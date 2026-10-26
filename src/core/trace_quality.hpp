#pragma once
#include "core/alignment.hpp"
#include <functional>
namespace kleiotide {
enum class QualityOperation { TrimEnds, CallsToN };
enum class QualityScope { Selected, AllTraces };
struct QualitySettings {
    QualityOperation operation=QualityOperation::TrimEnds;
    QualityScope scope=QualityScope::Selected;
    unsigned threshold=20,window=10;
    bool useOriginalQForEdited=false;
};
struct QualityRowPreview {
    SequenceId id=0;
    std::string name,status;
    std::size_t row=0,calls=0,scored=0,missingQ=0,protectedEdited=0;
    std::size_t changed=0,trimLeft=0,trimRight=0,keepBegin=0,keepEnd=0;
    bool eligible=false;
};
struct QualityPlan {
    const Document* source=nullptr;
    std::uint64_t revision=0;
    QualitySettings settings;
    std::vector<QualityRowPreview> rows;
    std::vector<Document::Replacement> edits;
    std::size_t eligibleRows=0,ignoredRows=0,changedRows=0,changedSymbols=0;
};
using QualityCancel=std::function<bool()>;
QualityPlan plan_trace_quality(const Document&,QualitySettings,const std::vector<SequenceId>& selected={},QualityCancel={});
void apply_trace_quality(Document&,const QualityPlan&);
}
