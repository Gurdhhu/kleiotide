#include "core/group_import.hpp"
#include <limits>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
namespace kleiotide {
namespace {
// Strip only a balanced outer pair spanning the entire cell. Embedded or
// separate parenthesized parts of an identifier remain literal.
void unwrap_parentheses(std::string& cell){
    if(cell.size()<2||cell.front()!='('||cell.back()!=')')return;
    std::size_t depth=0;
    for(std::size_t i=0;i<cell.size();++i){
        if(cell[i]=='(')++depth;
        else if(cell[i]==')'){if(!depth)return;--depth;if(!depth&&i+1!=cell.size())return;}
    }
    if(!depth)cell=cell.substr(1,cell.size()-2);
}
// Quote-aware TSV decoding is shared by headers and data. Only the chosen
// two fields are used for assignments; all columns are checked for valid syntax.
bool import_fields(std::string_view line,std::vector<std::string>& cells,std::string& error,const std::function<bool()>& cancelled){
    std::size_t at=0,polls=0;
    auto poll=[&]{if((polls++&4095)==0&&cancelled&&cancelled())throw std::runtime_error("Import preview cancelled.");};
    while(true){
        if(cells.size()>=group_import_max_columns)throw std::length_error("The group table exceeds 4,096 columns. Remove unused columns.");
        std::string cell;
        if(at<line.size()&&line[at]=='"'){
            ++at;bool closed=false;
            while(at<line.size()){
                poll();const auto c=line[at++];
                if(c!='"'){cell+=c;continue;}
                if(at<line.size()&&line[at]=='"'){cell+='"';++at;}
                else {closed=true;break;}
            }
            if(!closed){error="Unclosed double-quoted field";return false;}
            if(at<line.size()&&line[at]!='\t'){error="Unexpected text after a quoted field";return false;}
        }else{
            while(at<line.size()&&line[at]!='\t'){poll();cell+=line[at++];}
        }
        unwrap_parentheses(cell);cells.push_back(std::move(cell));
        if(at==line.size())return true;
        ++at; // Also retain an empty final field after a trailing tab.
    }
}

}

std::uint32_t default_group_color(std::size_t i)noexcept {constexpr std::uint32_t colors[]={0x0072b2,0xe69f00,0x009e73,0xcc79a7,0x56b4e9,0xd55e00,0x777777,0x332288};return colors[i%8];}
GroupImportSource group_import_source(const Document& d){GroupImportSource source;source.revision=d.state_id();source.nextId=d.next_group_id();source.groups=d.groups();source.rows.reserve(d.rows().size());for(const auto& row:d.rows())source.rows.push_back({row.id,row.name});return source;}
GroupImportPlan plan_group_import(const Document& d,std::string_view text,bool header,bool duplicates,GroupImportColumns columns){return plan_group_import(group_import_source(d),text,header,duplicates,{},columns);}
GroupImportPlan plan_group_import(const GroupImportSource& d,std::string_view text,bool header,bool duplicates,std::function<bool()> cancelled,GroupImportColumns columns){
    auto check=[&]{if(cancelled&&cancelled())throw std::runtime_error("Import preview cancelled.");};check();
    if(text.size()>group_import_max_bytes)throw std::length_error("The group table exceeds 16 MiB. Split it into smaller tables.");
    GroupImportPlan plan;plan.revision=d.revision;plan.groups=d.groups;auto next=d.nextId;
    std::unordered_map<std::string,std::vector<SequenceId>> titles;for(const auto& row:d.rows){check();titles[row.name].push_back(row.id);}
    std::unordered_map<std::string,GroupId> groups;for(const auto& g:plan.groups)groups[g.name]=g.id;
    std::unordered_map<SequenceId,GroupId> assigned;std::istringstream input{std::string(text)};std::string line;std::size_t n=0;
    while(std::getline(input,line)){check();++n;if(n==1&&line.starts_with("\xef\xbb\xbf"))line.erase(0,3);if(!line.empty()&&line.back()=='\r')line.pop_back();if(n==1&&header){
            std::string error;if(!import_fields(line,plan.columnNames,error,cancelled))throw std::invalid_argument("Invalid header: "+error);
            plan.columnCount=plan.columnNames.size();continue;
        }if(line.empty())continue;
        if(plan.rows.size()>=group_import_max_rows)throw std::length_error("The group table exceeds 100,000 data rows. Split it into smaller tables.");
        GroupImportRow row;row.line=n;
        std::vector<std::string> cells;
        if(!import_fields(line,cells,row.status,cancelled)){row.title=line;row.error=true;}
        else if(columns.sequence==columns.group){row.title=line;row.status="Choose different columns for sequence name and group name";row.error=true;}
        else if(columns.sequence>=cells.size()||columns.group>=cells.size()){row.title=line;row.status="A selected column is missing from this row";row.error=true;}
        else{row.title=std::move(cells[columns.sequence]);row.name=std::move(cells[columns.group]);const auto found=titles.find(row.title);
            if(row.title.empty()||row.name.find('\0')!=std::string::npos||row.name.find_first_of("\t\r\n")!=std::string::npos||(!row.name.empty()&&row.name.find_first_not_of(' ')==std::string::npos)){row.status="Blank title or invalid group name";row.error=true;}
            else if(found==titles.end()){row.status="Unmatched title; skipped";++plan.warnings;}
            else if(found->second.size()>1&&!duplicates){row.status="Duplicate alignment title; enable Apply to all matching titles";row.error=true;}
            else{GroupId target=0;if(!row.name.empty()){auto it=groups.find(row.name);if(it!=groups.end())target=it->second;else{
                        if(next==std::numeric_limits<GroupId>::max())throw std::overflow_error("Group ID space exhausted.");
                        if(plan.groups.size()>=100000)throw std::length_error("The resulting group catalog would exceed 100,000 groups.");
                        target=next++;groups[row.name]=target;plan.groups.push_back({target,row.name,default_group_color(plan.groups.size())});}}
                bool conflict=false;for(auto id:found->second){check();const auto prior=assigned.find(id);if(prior!=assigned.end()&&prior->second!=target)conflict=true;}
                if(conflict){row.status="Conflicting assignments to the same sequence";row.error=true;}
                else{for(auto id:found->second){check();assigned[id]=target;}row.status="Assign "+std::to_string(found->second.size())+" matching row(s)"+(row.name.empty()?" to Ungrouped":"");}
            }
        }
        plan.columnCount=std::max(plan.columnCount,cells.size());
        if(row.error)++plan.errors;
        plan.rows.push_back(std::move(row));
    }
    for(const auto& row:d.rows){check();if(auto it=assigned.find(row.id);it!=assigned.end())plan.assignments.emplace_back(row.id,it->second);}
    return plan;
}
void apply_group_import(Document& d,const GroupImportPlan& p){if(p.revision!=d.state_id())throw std::invalid_argument("Alignment changed; refresh the import preview.");if(p.errors||p.assignments.empty())throw std::invalid_argument("Resolve import errors and provide at least one matching title.");d.apply_groups(p.groups,p.assignments);}
GroupImportPlan plan_identical_groups(const Document& d){
    GroupImportPlan plan;plan.revision=d.state_id();
    std::unordered_map<std::string,std::size_t> lookup;
    std::vector<std::vector<SequenceId>> clusters;
    for(const auto& row:d.rows()){
        std::string key;key.reserve(row.symbols.size()+2);
        key.push_back(static_cast<char>(row.kind));key.push_back(static_cast<char>(row.representation));
        for(char c:row.symbols.text())if(!is_gap(c))key.push_back(c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c);
        const auto [it,inserted]=lookup.emplace(std::move(key),clusters.size());
        if(inserted)clusters.emplace_back();
        clusters[it->second].push_back(row.id);
    }
    auto next=d.next_group_id();
    for(const auto& cluster:clusters){GroupId id=0;
        if(cluster.size()>1){if(next==std::numeric_limits<GroupId>::max())throw std::overflow_error("Group ID space exhausted.");
            id=next++;const auto n=plan.groups.size();plan.groups.push_back({id,"Identical "+std::to_string(n+1),default_group_color(n)});}
        for(auto row:cluster)plan.assignments.emplace_back(row,id);
    }
    return plan;
}
}
