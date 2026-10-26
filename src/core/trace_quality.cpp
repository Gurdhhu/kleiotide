#include "core/trace_quality.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>
namespace kleiotide {
namespace {
void check(const QualityCancel& cancel){if(cancel&&cancel())throw std::runtime_error("Quality preview cancelled.");}
char upper(char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;}
struct Call {std::size_t column;std::optional<unsigned> q;};
}
QualityPlan plan_trace_quality(const Document& document,QualitySettings settings,const std::vector<SequenceId>& selected,QualityCancel cancel){
    if(settings.threshold>255||!settings.window||settings.window>10000||
       (settings.operation!=QualityOperation::TrimEnds&&settings.operation!=QualityOperation::CallsToN)||
       (settings.scope!=QualityScope::Selected&&settings.scope!=QualityScope::AllTraces))throw std::invalid_argument("Invalid trace-quality settings.");
    QualityPlan plan;plan.source=&document;plan.revision=document.state_id();plan.settings=settings;
    std::unordered_set<SequenceId> chosen;
    if(settings.scope==QualityScope::Selected)for(auto id:selected){(void)document.sequence(id);if(!chosen.insert(id).second)throw std::invalid_argument("Repeated selected sequence.");}
    for(std::size_t rowIndex=0;rowIndex<document.rows().size();++rowIndex){
        check(cancel);const auto& row=document.rows()[rowIndex];if(settings.scope==QualityScope::Selected&&!chosen.contains(row.id))continue;
        QualityRowPreview info;info.id=row.id;info.name=row.name;info.row=rowIndex+1;
        for(std::size_t c=0;c<row.symbols.size();++c){if((c&4095)==0)check(cancel);if(!is_gap(row.symbols.text()[c]))++info.calls;}info.keepEnd=info.calls;
        if(row.kind==RowKind::Protein)info.status="Protein; ignored";
        else if(row.kind==RowKind::Mask)info.status="Mask; ignored";
        else if(!row.trace)info.status="No linked trace; ignored";
        else if(!info.calls)info.status="No current calls; ignored";
        if(!info.status.empty()){++plan.ignoredRows;plan.rows.push_back(std::move(info));continue;}
        info.eligible=true;++plan.eligibleRows;
        const auto& original=*row.trace->original;const auto& text=row.symbols.text();std::vector<Call> calls;calls.reserve(info.calls);std::size_t ordinal=0;
        for(std::size_t column=0;column<text.size();++column){if((column&4095)==0)check(cancel);if(is_gap(text[column]))continue;
            const auto mapped=row.trace->originalCalls[ordinal++];std::optional<unsigned> q;
            if(mapped<0||original.quality.empty())++info.missingQ;
            else{const auto index=static_cast<std::size_t>(mapped);const auto imported=row.trace->reversed?trace_complement(original.calls[index]):original.calls[index];
                const bool edited=upper(text[column])!=upper(imported);
                if(edited&&!settings.useOriginalQForEdited)++info.protectedEdited;
                else{q=original.quality[index];++info.scored;}
            }
            calls.push_back({column,q});
        }
        std::string after;
        if(settings.operation==QualityOperation::CallsToN){
            for(std::size_t i=0;i<calls.size();++i){if((i&4095)==0)check(cancel);const auto& call=calls[i];if(!call.q||*call.q>=settings.threshold||upper(text[call.column])=='N')continue;
                if(after.empty())after=text;
                after[call.column]=text[call.column]>='a'&&text[call.column]<='z'?'n':'N';++info.changed;
            }
            info.status=info.changed?"Low-Q calls replaced":"No eligible low-Q calls";
        }else if(calls.size()<settings.window)info.status="Shorter than one complete window; unchanged";
        else{
            std::size_t sum=0,unknown=0;std::optional<std::size_t> first,last;
            for(std::size_t i=0;i<calls.size();++i){if((i&4095)==0)check(cancel);
                if(calls[i].q)sum+=*calls[i].q;else ++unknown;
                if(i>=settings.window){const auto& previous=calls[i-settings.window];if(previous.q)sum-=*previous.q;else --unknown;}
                if(i+1>=settings.window&&!unknown&&sum>=static_cast<std::size_t>(settings.threshold)*settings.window){const auto start=i+1-settings.window;if(!first)first=start;last=i+1;}
            }
            if(!first)info.status="No complete passing window; unchanged";
            else{
                auto begin=*first,end=*last;
                // Unknown/edited calls are protected anchors; retain a single
                // interval containing all of them as well as passing windows.
                for(std::size_t i=0;i<calls.size();++i){if((i&4095)==0)check(cancel);if(!calls[i].q){begin=std::min(begin,i);end=std::max(end,i+1);}}
                info.keepBegin=begin;info.keepEnd=end;info.trimLeft=begin;info.trimRight=calls.size()-end;info.changed=info.trimLeft+info.trimRight;
                if(info.changed){after=text;for(std::size_t i=0;i<calls.size();++i){if((i&4095)==0)check(cancel);if(i<begin||i>=end)after[calls[i].column]='~';}}
                info.status=info.changed?"Terminal calls replaced by open gaps":"All calls retained";
            }
        }
        if(!info.scored)info.status="No usable Q; unchanged";
        if(info.changed){++plan.changedRows;plan.changedSymbols+=info.changed;plan.edits.push_back({row.id,0,text.size(),std::move(after)});}
        plan.rows.push_back(std::move(info));
    }
    check(cancel);return plan;
}
void apply_trace_quality(Document& document,const QualityPlan& plan){
    if(plan.source!=&document||plan.revision!=document.state_id())throw std::invalid_argument("Alignment changed; create a new quality preview.");
    document.replace_many(plan.edits);
}
}
