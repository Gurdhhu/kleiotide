#include "core/merge.hpp"
#include "core/site_summary.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>
namespace kleiotide {
namespace {
char upper(char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;}
char canonical(char c){c=upper(c);return c=='U'?'T':c;}
char normalized(char c){return c=='U'?'T':c=='u'?'t':c;}
bool lower(char c){return c>='a'&&c<='z';}
bool missing(char c){return upper(c)=='N'||c=='?';}
void check(const MergeCancel& cancel){if(cancel&&cancel())throw std::runtime_error("Merge preview cancelled.");}
char iupac(unsigned state){constexpr char codes[]="~ACMGRSVTWYHKDBN";return codes[state&15];}
struct Input {
    const Sequence* row=nullptr;
    std::string text;
    std::vector<int> q;
    std::size_t first=0,last=0,informationFirst=0,informationLast=0;
};
void span(Input& in){
    in.first=0;while(in.first<in.text.size()&&is_gap(in.text[in.first]))++in.first;
    in.last=in.text.size();while(in.last>in.first&&is_gap(in.text[in.last-1]))--in.last;
    const auto absent=[](char c){return is_gap(c)||missing(c);};
    in.informationFirst=0;while(in.informationFirst<in.text.size()&&absent(in.text[in.informationFirst]))++in.informationFirst;
    in.informationLast=in.text.size();while(in.informationLast>in.informationFirst&&absent(in.text[in.informationLast-1]))--in.informationLast;
}
std::vector<Input> inputs(const Document& d,const std::vector<SequenceId>& ids,bool degap,const MergeCancel& cancel){
    if(ids.size()<2)throw std::invalid_argument("Select at least two nucleotide sequence names.");
    std::unordered_set<SequenceId> chosen;
    for(auto id:ids){(void)d.sequence(id);if(!chosen.insert(id).second)throw std::invalid_argument("Repeated selected sequence.");}
    std::vector<Input> result;
    for(const auto& row:d.rows())if(chosen.contains(row.id)){
        check(cancel);
        if(row.kind!=RowKind::Nucleotide)throw std::invalid_argument("Merge accepts nucleotide sequences only. Deselect proteins and masks.");
        Input in;in.row=&row;const auto& text=row.symbols.text();std::size_t ordinal=0;
        for(std::size_t c=0;c<text.size();++c){
            if((c&4095)==0)check(cancel);
            const auto symbol=normalized(text[c]);int q=-1;
            if(!is_gap(symbol)){
                if(row.trace&&row.trace->original&&ordinal<row.trace->originalCalls.size()){
                    const auto mapped=row.trace->originalCalls[ordinal];const auto& original=*row.trace->original;
                    if(mapped>=0&&static_cast<std::size_t>(mapped)<original.quality.size()&&static_cast<std::size_t>(mapped)<original.calls.size()){
                        const auto at=static_cast<std::size_t>(mapped);
                        const auto imported=row.trace->reversed?trace_complement(original.calls[at]):original.calls[at];
                        if(upper(text[c])==upper(imported))q=original.quality[at];
                    }
                }
                ++ordinal;
            }
            if(!degap||!is_gap(symbol)){in.text.push_back(is_gap(symbol)?'~':symbol);in.q.push_back(q);}
        }
        span(in);result.push_back(std::move(in));
    }
    return result;
}
MergeContig consensus(const std::vector<Input>& rows,const MergeSettings& settings,const MergeCancel& cancel){
    MergeContig result;std::size_t width=0;for(const auto& in:rows)width=std::max(width,in.text.size());
    result.symbols.reserve(width);
    for(std::size_t c=0;c<width;++c){
        if((c&1023)==0)check(cancel);
        unsigned seen=0,common=15;std::size_t calls=0,gaps=0,unknown=0;char lone='N',loneUnknown='N';
        for(const auto& in:rows){
            if(c>=in.text.size())continue;
            const auto symbol=in.text[c];
            if(is_gap(symbol)){if(c>=in.informationFirst&&c<in.informationLast)++gaps;continue;}
            if(missing(symbol)){++unknown;loneUnknown=lower(symbol)?'n':'N';continue;}
            const auto state=nucleotide_state(symbol);seen|=state;common&=state;++calls;lone=symbol;
        }
        const bool baseConflict=calls>1&&!common,gapConflict=calls&&gaps;
        if(calls>1)++result.overlappingPositions;
        if(baseConflict)++result.residueConflicts;
        if(gapConflict)++result.gapConflicts;
        if(baseConflict||gapConflict)++result.conflictingPositions;
        char symbol=calls==1?lone:seen?iupac(seen):unknown==1?loneUnknown:unknown?'N':'~';
        bool resolved=false;
        if(baseConflict&&settings.resolveQuality&&!gapConflict){
            int best=-1;unsigned winner=0;bool complete=true;
            for(const auto& in:rows)if(c<in.text.size()&&!is_gap(in.text[c])&&!missing(in.text[c])){
                if(in.q[c]<0)complete=false;
                if(in.q[c]>best){best=in.q[c];winner=nucleotide_state(in.text[c]);}
            }
            bool sufficient=complete&&(winner==1||winner==2||winner==4||winner==8);
            for(const auto& in:rows)if(c<in.text.size()&&!is_gap(in.text[c])&&!missing(in.text[c])&&nucleotide_state(in.text[c])!=winner){
                if(in.q[c]>=best||best-in.q[c]<static_cast<int>(settings.qualityMargin))sufficient=false;
            }
            if(sufficient){symbol=iupac(winner);++result.qualityResolved;resolved=true;}
        }
        if(baseConflict||gapConflict)result.conflicts.push_back({c,resolved});
        result.symbols.push_back(symbol);
    }
    for(const auto& in:rows){result.ids.push_back(in.row->id);result.placements.push_back({in.row->id,in.first,in.last-in.first});}
    return result;
}
struct Reference {std::string text;std::vector<bool> information;};
Reference reference(const std::vector<Input>& rows,const MergeSettings& settings,const MergeCancel& cancel){
    Reference ref;ref.text=consensus(rows,settings,cancel).symbols;ref.information.resize(ref.text.size());
    for(const auto& in:rows)for(std::size_t c=0;c<in.text.size();++c){if((c&4095)==0)check(cancel);if(!is_gap(in.text[c])&&!missing(in.text[c]))ref.information[c]=true;}
    return ref;
}
struct Placement {
    std::size_t row=0,aBegin=0,bBegin=0,aEnd=0,bEnd=0,score=0;
    unsigned edits=0;
    std::string operations; // M: two calls, D: gap in new read, I: new contig column.
};
bool equal(const Reference& ref,std::size_t a,char b){return !ref.information[a]||missing(b)||canonical(ref.text[a])==canonical(b);}
std::vector<Placement> exact_candidates(const Reference& ref,const Input& in,std::size_t row,std::size_t minimum,const MergeCancel& cancel){
    std::vector<Placement> best;std::size_t score=0,work=0;
    if(in.text.size()<minimum||ref.text.size()<minimum)return best;
    const auto a=static_cast<std::int64_t>(ref.text.size()),b=static_cast<std::int64_t>(in.text.size());
    for(auto shift=static_cast<std::int64_t>(minimum)-b;shift<=a-static_cast<std::int64_t>(minimum);++shift){
        if((++work&1023)==0)check(cancel);
        const auto begin=std::max<std::int64_t>(0,shift),end=std::min(a,shift+b);
        std::size_t matched=0;bool exact=true;
        for(auto c=begin;c<end;++c){
            if((++work&4095)==0)check(cancel);
            const auto at=static_cast<std::size_t>(c);const auto y=in.text[static_cast<std::size_t>(c-shift)];
            if(!equal(ref,at,y)){exact=false;break;}
            if(ref.information[at]&&!missing(y))++matched;
        }
        if(exact&&matched>=minimum&&matched>=score){
            if(matched>score){score=matched;best.clear();}
            if(best.size()<2)best.push_back({row,static_cast<std::size_t>(begin),static_cast<std::size_t>(begin-shift),static_cast<std::size_t>(end),static_cast<std::size_t>(end-shift),matched,0,std::string(static_cast<std::size_t>(end-begin),'M')});
        }
    }
    return best;
}
// Semi-global edit-distance alignment. Free terminal overhangs; each interior
// substitution/insertion/deletion costs one. Keep maximum matching evidence at
// every exact edit count. Rolling score rows + byte traceback bound memory.
std::vector<Placement> edited_candidates(const Reference& ref,const Input& in,std::size_t row,const MergeSettings& settings,const MergeCancel& cancel){
    const auto m=ref.text.size(),n=in.text.size(),k=static_cast<std::size_t>(settings.maximumEdits),states=k+1;
    if(m<settings.minimumOverlap||n<settings.minimumOverlap)return {};
    constexpr std::size_t limit=64*1024*1024;
    if(m+1>limit/(n+1)||((m+1)*(n+1))>limit/states)throw std::length_error("Overlap search exceeds the 64-million-state limit. Reduce the edit allowance or use shorter reads.");
    const auto cells=(m+1)*(n+1)*states;
    std::vector<unsigned char> trace(cells);
    std::vector<int> previous((n+1)*states,-1),current(previous.size(),-1);
    for(std::size_t j=0;j<=n;++j)previous[j*states]=0;
    struct End {std::size_t i,j,e;};std::vector<End> ends;int bestScore=-1;std::size_t bestEdits=k+1,work=0;
    auto remember=[&](std::size_t i,std::size_t j,std::size_t e,int score){
        if(score<0||static_cast<std::size_t>(score)<settings.minimumOverlap)return;
        if(e<bestEdits||(e==bestEdits&&score>bestScore)){bestScore=score;bestEdits=e;ends.clear();}
        if(score==bestScore&&e==bestEdits)ends.push_back({i,j,e});
    };
    for(std::size_t i=1;i<=m;++i){
        std::fill(current.begin(),current.end(),-1);current[0]=0;
        for(std::size_t j=1;j<=n;++j){
            const bool same=equal(ref,i-1,in.text[j-1]);const std::size_t cost=same?0:1;
            const int vote=ref.information[i-1]&&!missing(in.text[j-1])&&same?1:0;
            for(std::size_t e=0;e<states;++e){
                if((++work&4095)==0)check(cancel);
                int best=-1;unsigned char op=0;
                if(e>=cost){const auto v=previous[(j-1)*states+e-cost];if(v>=0){best=v+vote;op=cost?2:1;}}
                if(e){const auto up=previous[j*states+e-1],left=current[(j-1)*states+e-1];if(up>best){best=up;op=3;}if(left>best){best=left;op=4;}}
                current[j*states+e]=best;trace[(i*(n+1)+j)*states+e]=op;
                if(i==m||j==n)remember(i,j,e,best);
            }
        }
        previous.swap(current);
    }
    std::vector<Placement> result;
    for(const auto& end:ends){
        check(cancel);auto i=end.i,j=end.j,e=end.e;std::string operations;
        while(i&&j){
            if((++work&4095)==0)check(cancel);
            const auto op=trace[(i*(n+1)+j)*states+e];
            if(op==1||op==2){operations.push_back('M');--i;--j;if(op==2)--e;}
            else if(op==3){operations.push_back('D');--i;--e;}
            else if(op==4){operations.push_back('I');--j;--e;}
            else throw std::logic_error("Invalid overlap traceback.");
        }
        std::reverse(operations.begin(),operations.end());
        Placement p{row,i,j,end.i,end.j,static_cast<std::size_t>(bestScore),static_cast<unsigned>(end.e),std::move(operations)};
        // Equivalent terminal unknown padding can reach several endpoints. Prefer
        // more aligned columns for the same start; true repeat placements remain.
        auto equivalent=std::find_if(result.begin(),result.end(),[&](const auto& old){return old.aBegin==p.aBegin&&old.bBegin==p.bBegin;});
        if(equivalent!=result.end()){if(p.operations.size()>equivalent->operations.size())*equivalent=std::move(p);}
        else if(result.size()<2)result.push_back(std::move(p));
    }
    return result;
}
std::vector<Placement> candidates(const Reference& ref,const Input& in,std::size_t row,const MergeSettings& settings,const MergeCancel& cancel){
    if(static_cast<std::size_t>(std::count_if(in.text.begin(),in.text.end(),[](char c){return !missing(c);}))<settings.minimumOverlap)return {};
    auto exact=exact_candidates(ref,in,row,settings.minimumOverlap,cancel);
    // A qualifying exact overlap always wins, even when the allowance permits
    // longer overlaps with substitutions/indels. Preserve exact-repeat ambiguity.
    if(!exact.empty()||!settings.maximumEdits)return exact;
    return edited_candidates(ref,in,row,settings,cancel);
}
void combine(std::vector<Input>& placed,const Input& incoming,const Placement& p,const MergeCancel& cancel){
    std::vector<Input> output(placed.size()+1);for(std::size_t r=0;r<placed.size();++r)output[r].row=placed[r].row;output.back().row=incoming.row;
    std::size_t work=0;
    auto column=[&](std::optional<std::size_t> a,std::optional<std::size_t> b){
        if((++work&4095)==0)check(cancel);
        for(std::size_t r=0;r<placed.size();++r){output[r].text.push_back(a?placed[r].text[*a]:'~');output[r].q.push_back(a?placed[r].q[*a]:-1);}
        output.back().text.push_back(b?incoming.text[*b]:'~');output.back().q.push_back(b?incoming.q[*b]:-1);
    };
    for(std::size_t c=0;c<p.aBegin;++c)column(c,{});
    for(std::size_t c=0;c<p.bBegin;++c)column({},c);
    auto a=p.aBegin,b=p.bBegin;
    for(const auto op:p.operations){if(op=='M')column(a++,b++);else if(op=='D')column(a++,{});else column({},b++);}
    for(;a<placed.front().text.size();++a)column(a,{});
    for(;b<incoming.text.size();++b)column({},b);
    for(auto& in:output)span(in);
    placed=std::move(output);
}
void overlap(MergePlan& plan,const std::vector<Input>& rows,const MergeCancel& cancel){
    std::vector<bool> available(rows.size(),true),orphan(rows.size(),false);
    for(std::size_t r=0;r<rows.size();++r){
        if(rows[r].text.size()>static_cast<std::size_t>(std::numeric_limits<int>::max()/2))throw std::length_error("Sequence too long for overlap coordinates.");
        if(std::none_of(rows[r].text.begin(),rows[r].text.end(),[](char c){return !missing(c);}))available[r]=false;
    }
    while(true){
        std::optional<std::size_t> seed;
        for(std::size_t r=0;r<rows.size();++r)if(available[r]&&(!seed||rows[r].text.size()>rows[*seed].text.size()))seed=r;
        if(!seed)break;
        available[*seed]=false;std::vector<Input> placed{rows[*seed]};std::size_t edits=0,substitutions=0,indels=0,compatible=0;
        while(true){
            check(cancel);const auto ref=reference(placed,plan.settings,cancel);std::optional<Placement> best;
            for(std::size_t r=0;r<rows.size();++r)if(available[r]||orphan[r]){
                const auto choices=candidates(ref,rows[r],r,plan.settings,cancel);
                if(choices.size()!=1)continue;
                const auto& choice=choices.front();
                if(!best||choice.edits<best->edits||(choice.edits==best->edits&&choice.score>best->score))best=choice;
            }
            if(!best)break;
            auto a=best->aBegin,b=best->bBegin;std::size_t added=0;
            for(const auto op:best->operations){
                if(op=='M'){
                    const auto symbol=rows[best->row].text[b];
                    if(!equal(ref,a,symbol)){++substitutions;++added;if(nucleotide_state(ref.text[a])&nucleotide_state(symbol))++compatible;}
                    ++a;++b;
                }else{++indels;++added;if(op=='D')++a;else ++b;}
            }
            if(added!=best->edits)throw std::logic_error("Overlap edit accounting differs from traceback.");
            combine(placed,rows[best->row],*best,cancel);edits+=best->edits;available[best->row]=false;orphan[best->row]=false;
        }
        if(placed.size()>1){
            auto result=consensus(placed,plan.settings,cancel);result.editsUsed=edits;result.substitutionsUsed=substitutions;result.indelBasesUsed=indels;result.compatibleDifferencesUsed=compatible;
            // Restore source order in the membership/placement table.
            std::sort(result.ids.begin(),result.ids.end(),[&](auto a,auto b){return std::find(plan.selectedIds.begin(),plan.selectedIds.end(),a)<std::find(plan.selectedIds.begin(),plan.selectedIds.end(),b);});
            std::sort(result.placements.begin(),result.placements.end(),[&](const auto& a,const auto& b){return std::find(plan.selectedIds.begin(),plan.selectedIds.end(),a.id)<std::find(plan.selectedIds.begin(),plan.selectedIds.end(),b.id);});
            plan.contigs.push_back(std::move(result));
        }else orphan[*seed]=true;
    }
    std::stable_sort(plan.contigs.begin(),plan.contigs.end(),[](const auto& a,const auto& b){return a.ids.size()!=b.ids.size()?a.ids.size()>b.ids.size():a.overlappingPositions>b.overlappingPositions;});
    if(!plan.contigs.empty())choose_merge_contig(plan,0);
    else for(auto id:plan.selectedIds)plan.omitted.push_back({id,"No unambiguous overlap within the selected limits"});
}
}
void choose_merge_contig(MergePlan& plan,std::size_t index){
    if(index>=plan.contigs.size())throw std::out_of_range("Unknown merge contig.");
    static_cast<MergeContig&>(plan)=plan.contigs[index];plan.chosenContig=index;plan.omitted.clear();
    for(auto id:plan.selectedIds)if(std::find(plan.ids.begin(),plan.ids.end(),id)==plan.ids.end()){
        std::string reason="No unambiguous overlap with this contig within the selected limits";
        for(std::size_t c=0;c<plan.contigs.size();++c)if(std::find(plan.contigs[c].ids.begin(),plan.contigs[c].ids.end(),id)!=plan.contigs[c].ids.end())reason="In separate contig "+std::to_string(c+1);
        const auto& row=plan.source->sequence(id);if(std::none_of(row.symbols.text().begin(),row.symbols.text().end(),[](char c){return !is_gap(c)&&!missing(c);}))reason="No informative nucleotide calls";
        plan.omitted.push_back({id,std::move(reason)});
    }
}
MergePlan plan_sequence_merge(const Document& d,const std::vector<SequenceId>& ids,MergeSettings settings,MergeCancel cancel){
    if((settings.mode!=MergeMode::Aligned&&settings.mode!=MergeMode::ExactOverlap)||!settings.minimumOverlap||settings.qualityMargin>255||settings.maximumEdits>100)throw std::invalid_argument("Invalid merge settings.");
    if(settings.mode==MergeMode::ExactOverlap&&settings.resolveQuality)throw std::invalid_argument("Quality resolution is available only for current-alignment merging.");
    const auto rows=inputs(d,ids,settings.mode==MergeMode::ExactOverlap,cancel);
    if(settings.resolveQuality&&std::any_of(rows.begin(),rows.end(),[](const auto& in){return !in.row->trace;}))throw std::invalid_argument("Quality resolution requires linked traces on every selected sequence.");
    MergePlan plan;plan.source=&d;plan.revision=d.state_id();plan.settings=settings;
    for(const auto& in:rows)plan.selectedIds.push_back(in.row->id);
    if(settings.mode==MergeMode::Aligned){
        static_cast<MergeContig&>(plan)=consensus(rows,settings,cancel);
        if(plan.symbols.empty()||std::none_of(plan.symbols.begin(),plan.symbols.end(),[](char c){return !is_gap(c);}))throw std::invalid_argument("Selected sequences contain no nucleotide calls to merge.");
    }else overlap(plan,rows,cancel);
    return plan;
}
SequenceId apply_sequence_merge(Document& d,const MergePlan& plan,std::string name){
    if(plan.source!=&d||plan.revision!=d.state_id())throw std::logic_error("The alignment changed after this merge preview. Preview again.");
    if(name.empty()||name.find_first_of("\r\n\0",0,3)!=std::string::npos)throw std::invalid_argument("Enter a nonempty single-line sequence name.");
    if(plan.ids.size()<2||plan.symbols.empty())throw std::invalid_argument("No merge preview is available.");
    const auto id=d.next_sequence_id();d.append({{std::move(name),plan.symbols,RowKind::Nucleotide}});return id;
}
}
