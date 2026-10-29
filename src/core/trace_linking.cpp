#include "core/trace_linking.hpp"
#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <unordered_map>
namespace kleiotide {namespace {
char base(char c){c=char(std::toupper(static_cast<unsigned char>(c)));return c=='U'?'T':c;}
void stop(const std::function<bool()>& c){if(c&&c())throw std::runtime_error("Trace preview cancelled.");}
struct Hit {int score=0;std::size_t begin=0,end=0,tbegin=0,tend=0,matches=0,aligned=0,indels=0;bool reverse=false;std::vector<std::int32_t> map;};
std::vector<Hit> align(const std::string& query,const std::string& calls,bool reverse,std::size_t offset,const std::function<bool()>& cancel,std::size_t limit){
 const auto n=query.size(),m=calls.size();if(!m||!n)return {};if(n>limit/(m+1))throw std::length_error("Trace alignment exceeds workspace limit.");
 std::vector<unsigned char> trace((n+1)*(m+1));std::vector<int> prev(m+1),curr(m+1),vertical(m+1,-100000000);
 struct End{int score;std::size_t i,j;};std::vector<End> ends;
 for(std::size_t i=1;i<=n;++i){stop(cancel);int horizontal=-100000000;curr[0]=0;int rowBest=0;std::size_t rowJ=0,lastRowJ=0;
  for(std::size_t j=1;j<=m;++j){unsigned char bits=0;const auto vOpen=prev[j]-7,vExtend=vertical[j]-2;vertical[j]=std::max(vOpen,vExtend);if(vOpen>=vExtend)bits|=8;if(vExtend>=vOpen)bits|=16;
   const auto hOpen=curr[j-1]-7,hExtend=horizontal-2;horizontal=std::max(hOpen,hExtend);if(hOpen>=hExtend)bits|=32;if(hExtend>=hOpen)bits|=64;
   const char a=query[i-1],b=calls[j-1];const auto diag=prev[j-1]+(a==b&&a!='N'&&a!='?'?4:-4);const int score=std::max({0,diag,vertical[j],horizontal});
   if(score>0){if(diag==score)bits|=1;if(vertical[j]==score)bits|=2;if(horizontal==score)bits|=4;}
   curr[j]=score;trace[i*(m+1)+j]=bits;if(score>rowBest){rowBest=score;rowJ=lastRowJ=j;}else if(score==rowBest)lastRowJ=j;
  }
  if(rowBest>0){ends.push_back({rowBest,i,rowJ});if(lastRowJ!=rowJ)ends.push_back({rowBest,i,lastRowJ});}
  prev.swap(curr);
 }
 std::sort(ends.begin(),ends.end(),[](auto a,auto b){return a.score>b.score;});std::vector<Hit> hits;
 for(const auto e:ends){if(hits.size()>=8||(!hits.empty()&&e.score<hits[0].score*.95))break;stop(cancel);
  Hit h;h.score=e.score;h.reverse=reverse;h.end=e.i+offset;h.tend=e.j;h.map.assign(n,-1);auto i=e.i,j=e.j;int state=0;
  while(i&&j){const auto code=trace[i*(m+1)+j];if(!state)state=(code&1)?1:(code&2)?2:(code&4)?3:0;if(!state)break;
   if(state==1){h.map[i-1]=static_cast<std::int32_t>(reverse?m-j:j-1);h.matches+=query[i-1]==calls[j-1]&&(query[i-1]=='A'||query[i-1]=='C'||query[i-1]=='G'||query[i-1]=='T');++h.aligned;--i;--j;state=0;}
   else if(state==2){--i;++h.aligned;++h.indels;state=(code&8)?0:2;}
   else{--j;++h.aligned;++h.indels;state=(code&32)?0:3;}
  }
  h.begin=i+offset;h.tbegin=j;bool same=false;for(const auto& old:hits){const auto overlap=std::min(h.end,old.end)>std::max(h.begin,old.begin)?std::min(h.end,old.end)-std::max(h.begin,old.begin):0;
   if(overlap*2>std::min(h.end-h.begin,old.end-old.begin)&&std::int64_t(h.tbegin)-std::int64_t(h.begin)==std::int64_t(old.tbegin)-std::int64_t(old.begin)){same=true;break;}}
  if(!same){
   // Union all optimal affine traceback paths. A peak is retained only when
   // every path agrees; this avoids arbitrary left/right placement in repeats.
   struct Node{std::size_t i,j;unsigned state;};std::vector<Node> stack;std::vector<unsigned char> visited(trace.size());std::vector<std::int32_t> unique(n,-2);std::size_t prefix=0,work=0;
   auto push=[&](std::size_t a,std::size_t b,unsigned state){const auto bit=1U<<state;auto& seen=visited[a*(m+1)+b];if(!(seen&bit)){seen|=bit;stack.push_back({a,b,state});}};
   auto observe=[&](std::size_t q,std::int32_t value){auto& old=unique[q];if(old==-2)old=value;else if(old!=value)old=-1;};push(e.i,e.j,0);
   while(!stack.empty()){if((++work&4095)==0)stop(cancel);const auto node=stack.back();stack.pop_back();const auto a=node.i,b=node.j;if(!a||!b){prefix=std::max(prefix,a);continue;}const auto code=trace[a*(m+1)+b];
    if(node.state==0){if(!(code&7)){prefix=std::max(prefix,a);continue;}if(code&1){observe(a-1,static_cast<std::int32_t>(reverse?m-b:b-1));push(a-1,b-1,0);}if(code&2)push(a,b,1);if(code&4)push(a,b,2);}
    else if(node.state==1){observe(a-1,-1);if(code&8)push(a-1,b,0);if(code&16)push(a-1,b,1);}
    else{if(code&32)push(a,b-1,0);if(code&64)push(a,b-1,2);}
   }
   for(std::size_t q=0;q<n;++q)h.map[q]=q<prefix||unique[q]==-2?-1:unique[q];
   hits.push_back(std::move(h));
  }
 }
 return hits;
}
}
TraceMapping map_trace(std::string_view symbols,std::shared_ptr<const Chromatogram> original,const std::function<bool()>& cancel,std::size_t maxCells){
 TraceMapping result;if(!original||original->calls.empty()){result.reason="No original calls";return result;}
 if(original->calls.size()>std::size_t(std::numeric_limits<std::int32_t>::max()))throw std::length_error("Too many trace calls.");
 std::string query,calls;for(char c:symbols){stop(cancel);if(!is_gap(c)){if(query.size()>=2000000)throw std::length_error("Trace-link queries are limited to two million residues; trim terminal flanks first.");query+=base(c);}}for(char c:original->calls)calls+=base(c);
 if(query.size()>2000000)throw std::length_error("Trace-link queries are limited to two million residues; trim terminal flanks first.");
 if(query.empty()){result.reason="No nucleotide residues";return result;}
 if(query.size()==calls.size()){
  auto reverseCalls=calls;std::reverse(reverseCalls.begin(),reverseCalls.end());for(auto& c:reverseCalls)c=base(trace_complement(c));
  const bool forward=query==calls,reverse=query==reverseCalls;std::size_t concrete=0;for(char c:calls)concrete+=c=='A'||c=='C'||c=='G'||c=='T';
  if(forward!=reverse&&concrete>=std::min<std::size_t>(40,calls.size())){auto link=std::make_shared<TraceLink>();link->original=original;link->reversed=reverse;for(std::size_t i=0;i<query.size();++i)link->originalCalls.push_back(static_cast<std::int32_t>(reverse?calls.size()-1-i:i));result.link=link;result.orientation=reverse?"Reverse":"Forward";result.quality="Perfect";result.identity=1;result.matches=query.size();result.aligned=query.size();result.reason="Exact full-call match; original indices retained, including ambiguous calls";return result;}
 }
 std::vector<Hit> all;
 for(bool reverse:{false,true}){
  auto oriented=calls;if(reverse){std::reverse(oriented.begin(),oriented.end());for(auto& c:oriented)c=base(trace_complement(c));}
  std::vector<std::pair<std::size_t,std::size_t>> windows;
  if(query.size()<=maxCells/(calls.size()+1))windows.push_back({0,query.size()});
  else{
   constexpr std::size_t k=13;std::unordered_map<std::string,std::vector<std::size_t>> seeds;
   for(std::size_t j=0;j+k<=oriented.size();j+=k){auto key=oriented.substr(j,k);if(key.find_first_not_of("ACGT")==std::string::npos)seeds[key].push_back(j);}
   std::unordered_map<std::int64_t,int> diagonals;
   for(std::size_t i=0;i+k<=query.size();++i){if((i&4095)==0)stop(cancel);if(auto found=seeds.find(query.substr(i,k));found!=seeds.end())for(auto j:found->second){if(diagonals.size()>100000)break;++diagonals[(std::int64_t(i)-std::int64_t(j))/32];}}
   std::vector<std::pair<int,std::int64_t>> ranked;for(auto [d,count]:diagonals)ranked.push_back({count,d*32});std::sort(ranked.rbegin(),ranked.rend());
   for(auto [count,d]:ranked){(void)count;const auto margin=std::int64_t(std::max<std::size_t>(64,calls.size()/4));const auto a=std::size_t(std::max<std::int64_t>(0,d-margin)),b=std::min(query.size(),std::size_t(std::max<std::int64_t>(0,d+std::int64_t(calls.size())+margin)));
    if(a>=b)continue;
    bool overlap=false;for(auto w:windows)if(a<w.second&&b>w.first){overlap=true;break;}if(!overlap)windows.push_back({a,b});if(windows.size()==8)break;}
  }
  for(auto [a,b]:windows){auto hits=align(query.substr(a,b-a),oriented,reverse,a,cancel,maxCells);for(auto& h:hits){auto map=std::move(h.map);h.map.assign(query.size(),-1);std::copy(map.begin(),map.end(),h.map.begin()+a);all.push_back(std::move(h));}}
 }
 std::sort(all.begin(),all.end(),[](const auto& a,const auto& b){return a.score>b.score;});
 if(all.empty()){result.reason="No seeded overlap within bounded workspace";return result;}auto& h=all[0];result.matches=h.matches;result.aligned=h.aligned;result.identity=h.aligned?double(h.matches)/h.aligned:0;
 const auto minimum=std::min<std::size_t>(40,std::min(query.size(),calls.size()));
 if(h.matches<minimum||h.end-h.begin<std::min(query.size(),calls.size())*.3||result.identity<.65){result.reason="Insufficient unique matching evidence";return result;}
 const auto mapped=std::count_if(h.map.begin(),h.map.end(),[](auto index){return index>=0;});
 if(std::size_t(mapped)<minimum||double(mapped)<.5*(h.end-h.begin)){result.reason="Too many ambiguous original-call indices";return result;}
 if(all.size()>1&&all[1].score>=h.score*.95){result.reason="Ambiguous orientation or repeat placement";return result;}
 auto link=std::make_shared<TraceLink>();link->original=std::move(original);link->originalCalls=std::move(h.map);link->reversed=h.reverse;result.link=link;result.orientation=h.reverse?"Reverse":"Forward";
 result.quality=h.indels==0&&h.matches==query.size()?"Perfect":result.identity>=.9?"Good":"Terrible";result.reason="Heuristic affine overlap; terminal additions and ambiguous near-indel calls have no peak";return result;
}
int suggest_trace(std::string_view name,const std::vector<std::string>& stems,const std::vector<std::string>& internal){
 if(name.empty())return -1;
 for(int tier=0;tier<4;++tier){int found=-1;bool ambiguous=false;const auto& names=tier<2?stems:internal;for(std::size_t i=0;i<names.size();++i){const auto& s=names[i];if(s.empty())continue;const bool match=tier%2?name.find(s)!=std::string_view::npos||s.find(name)!=std::string::npos:name==s;if(match){if(found>=0)ambiguous=true;found=int(i);}}if(found>=0)return ambiguous?-1:found;}return -1;
}
void apply_trace_links(Document& d,const std::vector<std::pair<SequenceId,std::shared_ptr<const TraceLink>>>& links){
 std::vector<Document::Replacement> edits;for(const auto& [id,link]:links){if(d.sequence(id).kind!=RowKind::Nucleotide)throw std::invalid_argument("Traces require nucleotide rows.");edits.push_back({id,0,0,"",std::optional<std::shared_ptr<const TraceLink>>(link)});}d.replace_many(edits);
}
}
