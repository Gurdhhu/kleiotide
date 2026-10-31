#include "gui/rename_rules.hpp"
#include "gui/spart.hpp"
#include "gui/table_io.hpp"
#include <QHash>
#include <QSet>
#include <QTextBoundaryFinder>
#include <QRegularExpression>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
namespace kleiotide {
namespace {
QString name_of(const Sequence& row){return QString::fromUtf8(row.name.data(),static_cast<qsizetype>(row.name.size()));}
void valid(const QString& name){if(name.trimmed().isEmpty()||name.contains('\n')||name.contains('\r')||name.contains(QChar(0)))throw std::invalid_argument("Every resulting name must be nonblank and contain no newline or null character.");}
QString reverse_name(const QString& text){QStringList parts;QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme,text);int start=0;for(int end=finder.toNextBoundary();end>=0;end=finder.toNextBoundary()){parts.append(text.mid(start,end-start));start=end;}std::reverse(parts.begin(),parts.end());return parts.join(QString{});}
QString sanitized_name(QString name){
    static const QRegularExpression unsafe("[^A-Za-z0-9_]+");
    if(!name.contains(unsafe))return name;
    name.replace(unsafe,"_");
    qsizetype begin=0,end=name.size();while(begin<end&&name[begin]=='_')++begin;while(end>begin&&name[end-1]=='_')--end;
    name=name.mid(begin,end-begin);return name.isEmpty()?QString("sequence"):name;
}
}
std::vector<std::pair<SequenceId,std::string>> RenamePlan::patches()const{std::vector<std::pair<SequenceId,std::string>> result;result.reserve(changed);for(const auto& row:rows)if(row.before!=row.after)result.emplace_back(row.id,row.after.toUtf8().toStdString());return result;}
RenamePlan make_rename_plan(const Document& document,const RenameOptions& options,const std::vector<SequenceId>& scope){
    RenamePlan plan;plan.state=document.state_id();std::unordered_set<SequenceId> selected;
    for(auto id:scope){(void)document.sequence(id);if(!selected.insert(id).second)throw std::invalid_argument("A sequence occurs twice in the rename scope.");}
    const auto included=[&](SequenceId id){return selected.empty()||selected.contains(id);};
    const bool table=options.operation==RenameOperation::Table||options.operation==RenameOperation::AppendTable;
    if((options.operation==RenameOperation::Prepend||options.operation==RenameOperation::Append||options.operation==RenameOperation::Filename||options.operation==RenameOperation::Replace)&&options.text.isEmpty())throw std::invalid_argument("Enter the text to add or find.");
    if((options.operation==RenameOperation::StripSuffix||options.operation==RenameOperation::Unique)&&options.separator.isEmpty())throw std::invalid_argument("Enter a nonempty suffix separator.");
    QHash<QString,QString> mappings;QHash<QString,std::size_t> matches;
    if(table){int number=0;for(const auto& cells:parse_text_table(options.table)){++number;if(cells.size()!=2)throw std::invalid_argument((QString("Line %1: expected two selected columns.").arg(number)).toStdString());
        const auto old=normalized_table_cell(cells[0]),replacement=normalized_table_cell(cells[1]);valid(old);valid(replacement);if(mappings.contains(old)&&mappings[old]!=replacement)throw std::invalid_argument((QString("Line %1: conflicting mappings for %2.").arg(number).arg(old)).toStdString());mappings[old]=replacement;}
        if(mappings.isEmpty())throw std::invalid_argument("The rename table is empty.");}
    QSet<QString> reserved,assigned;
    for(const auto& row:document.rows()){reserved.insert(name_of(row));if(!included(row.id))assigned.insert(name_of(row));}
    QHash<QString,qulonglong> suffixes;
    QSet<QString> sanitizedReserved,sanitizedAssigned;QHash<QString,SequenceId> safeOwner;
    if(options.operation==RenameOperation::Sanitize)for(const auto& row:document.rows()){
        const auto original=name_of(row),base=sanitized_name(original),key=base.toCaseFolded();
        sanitizedReserved.insert(key);sanitizedReserved.insert(original.toCaseFolded());
        if(!included(row.id))sanitizedAssigned.insert(original.toCaseFolded());
        else if(original==base&&!safeOwner.contains(key))safeOwner.insert(key,row.id);
    }
    const QRegularExpression suffix(QRegularExpression::escape(options.separator)+"[0-9]+$");
    std::size_t ordinal=0;
    for(const auto& row:document.rows()){
        ++ordinal;if(!included(row.id))continue;auto before=name_of(row),after=before;
        switch(options.operation){
        case RenameOperation::SpacesToUnderscores:after.replace(' ','_');break;
        case RenameOperation::UnderscoresToSpaces:after.replace('_',' ');break;
        case RenameOperation::Prepend:after=options.text+before;break;
        case RenameOperation::Append:after=before+options.text;break;
        case RenameOperation::AppendGroup:case RenameOperation::PrependGroup:
            if(row.groupId){const auto& groups=document.groups();auto group=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return g.id==row.groupId;});if(group!=groups.end()){const auto name=QString::fromStdString(group->name);after=options.operation==RenameOperation::AppendGroup?before+options.separator+name:name+options.separator+before;}}break;
        case RenameOperation::Replace:after.replace(options.text,options.replacement,Qt::CaseSensitive);break;
        case RenameOperation::Filename:after=before+options.separator+options.text;break;
        case RenameOperation::Reverse:after=reverse_name(before);break;
        case RenameOperation::StripSuffix:after.remove(suffix);break;
        case RenameOperation::Unique:{if(assigned.contains(before)){auto& next=suffixes[before];if(!next)next=2;do{after=before+options.separator+QString::number(next++);}while(reserved.contains(after)||assigned.contains(after));}assigned.insert(after);reserved.insert(after);break;}
        case RenameOperation::RowNumber:after=options.text+QString::number(static_cast<qulonglong>(ordinal));break;
        case RenameOperation::Length:after=options.text+QString::number(static_cast<qulonglong>(row.symbols.size()));break;
        case RenameOperation::TrueLength:after=options.text+QString::number(static_cast<qulonglong>(row.symbols.residue_count()));break;
        case RenameOperation::SanitizeSpart:after=spart_sanitize(before);break;
        case RenameOperation::Sanitize:{const auto base=sanitized_name(before),key=base.toCaseFolded();after=base;
            if(sanitizedAssigned.contains(key)||(safeOwner.contains(key)&&safeOwner.value(key)!=row.id)){
                auto& next=suffixes[key];if(!next)next=2;
                do{after=base+"_"+QString::number(next++);}while(sanitizedReserved.contains(after.toCaseFolded())||sanitizedAssigned.contains(after.toCaseFolded()));
            }
            sanitizedAssigned.insert(after.toCaseFolded());sanitizedReserved.insert(after.toCaseFolded());break;}
        case RenameOperation::Table:case RenameOperation::AppendTable:if(mappings.contains(before)){++matches[before];after=options.operation==RenameOperation::Table?mappings[before]:before+options.separator+mappings[before];}break;
        }
        valid(after);if(before!=after)++plan.changed;plan.rows.push_back({row.id,before,after,false});
    }
    if(table){for(auto it=mappings.cbegin();it!=mappings.cend();++it){if(!matches.value(it.key()))plan.unmatched.append(it.key());else if(matches.value(it.key())>1)++plan.multipleMatches;}std::sort(plan.unmatched.begin(),plan.unmatched.end());}
    QHash<QString,std::size_t> finalCounts;for(const auto& row:document.rows())if(!included(row.id))++finalCounts[name_of(row)];for(const auto& row:plan.rows)++finalCounts[row.after];
    for(auto& row:plan.rows){row.duplicate=finalCounts.value(row.after)>1;if(row.duplicate)++plan.duplicateRows;}
    return plan;
}
bool rename_plan_current(const Document& document,const RenamePlan& plan){if(document.state_id()!=plan.state)return false;for(const auto& row:plan.rows)if(name_of(document.sequence(row.id))!=row.before)return false;return true;}
}
