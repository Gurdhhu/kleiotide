#pragma once
#include "core/alignment.hpp"
#include <functional>
namespace kleiotide {
struct GroupImportRow {std::size_t line=0;std::string title,name,status;bool error=false;};
struct GroupImportPlan {std::uint64_t revision=0;std::vector<SequenceGroup> groups;std::vector<std::pair<SequenceId,GroupId>> assignments;std::vector<GroupImportRow> rows;std::size_t errors=0,warnings=0,columnCount=0;std::vector<std::string> columnNames;};
std::uint32_t default_group_color(std::size_t index)noexcept;
// A worker snapshot contains titles/IDs/catalog only, never sequence data or
// references to the live document. Public parser limits apply to paste and load.
struct GroupImportSequence {SequenceId id=0;std::string name;};
struct GroupImportSource {std::uint64_t revision=0;GroupId nextId=1;std::vector<GroupImportSequence> rows;std::vector<SequenceGroup> groups;};
inline constexpr std::size_t group_import_max_bytes=16*1024*1024,group_import_max_rows=100000,group_import_max_columns=4096;
// Zero-based column indices. Extra fields are parsed but not assigned.
struct GroupImportColumns {std::size_t sequence=0,group=1;};
GroupImportSource group_import_source(const Document&);
GroupImportPlan plan_group_import(const GroupImportSource&,std::string_view,bool header,bool allowDuplicateTitles,std::function<bool()> cancelled={},GroupImportColumns columns={});
GroupImportPlan plan_group_import(const Document&,std::string_view,bool header,bool allowDuplicateTitles,GroupImportColumns columns={});
GroupImportPlan plan_identical_groups(const Document&);
void apply_group_import(Document&,const GroupImportPlan&);
}
