#pragma once
#include "core/alignment.hpp"
#include <QString>
#include <QStringList>
namespace kleiotide {
enum class RenameOperation { SpacesToUnderscores,UnderscoresToSpaces,Prepend,Append,Replace,Filename,Reverse,StripSuffix,Unique,RowNumber,Length,TrueLength,AppendTable,Sanitize,SanitizeSpart,AppendGroup,PrependGroup,Table };
struct RenameOptions {RenameOperation operation=RenameOperation::SpacesToUnderscores;QString text,replacement,separator="_",table;};
struct RenameChange {SequenceId id=0;QString before,after;bool duplicate=false;};
struct RenamePlan {
    std::uint64_t state=0;
    std::vector<RenameChange> rows;
    QStringList unmatched;
    std::size_t multipleMatches=0,changed=0,duplicateRows=0;
    std::vector<std::pair<SequenceId,std::string>> patches()const;
};
RenamePlan make_rename_plan(const Document&,const RenameOptions&,const std::vector<SequenceId>& scope={});
bool rename_plan_current(const Document&,const RenamePlan&);
}
