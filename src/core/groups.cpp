#include "core/alignment.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace kleiotide {
const SequenceGroup* Document::group(GroupId id)const noexcept {for(const auto& g:groups_)if(g.id==id)return &g;return nullptr;}
Document::GroupPatch Document::group_patch(std::vector<SequenceGroup> groups,const std::vector<std::pair<SequenceId,GroupId>>& assignments)const{
    if(groups.size()>100000)throw std::invalid_argument("Too many sequence groups.");
    std::unordered_set<GroupId> existing;for(const auto& g:groups_)existing.insert(g.id);
    std::unordered_set<GroupId> ids;std::unordered_set<std::string> names;auto next=nextGroupId_;
    for(const auto& g:groups){if(!g.id||g.id==std::numeric_limits<GroupId>::max()||!ids.insert(g.id).second||g.color>0xffffff||g.name.empty()||g.name.find_first_of("\t\r\n")!=std::string::npos||g.name.find('\0')!=std::string::npos||g.name.find_first_not_of(' ')==std::string::npos||!names.insert(g.name).second)throw std::invalid_argument("Groups need unique IDs/names, nonblank single-line names and RGB colors.");
        if(!existing.contains(g.id)&&g.id<nextGroupId_)throw std::invalid_argument("A retired group ID cannot be reused.");
        next=std::max(next,g.id+1);}
    std::unordered_map<SequenceId,GroupId> requested;
    for(const auto& [id,target]:assignments){if((target&&!ids.contains(target))||!requested.emplace(id,target).second)throw std::invalid_argument("Invalid or repeated group assignment.");}
    GroupPatch patch{std::move(groups),{},next};
    for(const auto& row:rows_){const auto it=requested.find(row.id);const auto target=it!=requested.end()?it->second:ids.contains(row.groupId)?row.groupId:0;if(target!=row.groupId)patch.assignments.push_back({row.id,row.groupId,target});if(it!=requested.end())requested.erase(it);}
    if(!requested.empty())throw std::invalid_argument("Unknown sequence ID in group assignment.");
    return patch;
}
void Document::apply_groups(std::vector<SequenceGroup> groups,const std::vector<std::pair<SequenceId,GroupId>>& assignments){
    auto patch=group_patch(std::move(groups),assignments);if(patch.other==groups_&&patch.assignments.empty())return;
    Command command;command.cost=sizeof(Command)+sizeof(GroupPatch)+patch.assignments.size()*sizeof(GroupAssignment);
    for(const auto& g:groups_)command.cost+=sizeof(SequenceGroup)+g.name.size();
    for(const auto& g:patch.other)command.cost+=sizeof(SequenceGroup)+g.name.size();
    command.edits.emplace_back(std::move(patch));execute(std::move(command));
}
Document Document::subset(const std::vector<SequenceId>& ids)const {
    std::unordered_set<SequenceId> chosen;for(auto id:ids){(void)sequence(id);if(!chosen.insert(id).second)throw std::invalid_argument("Repeated clipboard row.");}
    std::vector<Record> records;std::vector<SequenceId> stable;std::unordered_set<GroupId> used;
    for(const auto& row:rows_)if(chosen.contains(row.id)){records.push_back({row.name,row.symbols.text(),row.kind,row.trace,row.representation,row.groupId,row.protein});stable.push_back(row.id);used.insert(row.groupId);}
    std::vector<SequenceGroup> groups;for(const auto& g:groups_)if(used.contains(g.id))groups.push_back(g);
    Document result;result.restore_session(std::move(records),stable,nextSequenceId_,std::move(groups),nextGroupId_,translation_);return result;
}
void Document::insert_document(const Document& incoming,std::size_t position){
    auto catalog=groups_;auto next=nextGroupId_;std::unordered_map<GroupId,GroupId> mapping;
    for(const auto& source:incoming.groups_){auto name=source.name;unsigned suffix=2;
        for(;;){const auto match=std::find_if(catalog.begin(),catalog.end(),[&](const auto& g){return g.name==name;});
            if(match!=catalog.end()&&match->color==source.color){mapping[source.id]=match->id;break;}
            if(match!=catalog.end()){name=source.name+" ("+std::to_string(suffix++)+")";continue;}
            if(next==std::numeric_limits<GroupId>::max())throw std::overflow_error("Group ID space exhausted.");
            mapping[source.id]=next;catalog.push_back({next++,std::move(name),source.color});break;
        }
    }
    std::vector<Record> records;for(const auto& row:incoming.rows_)records.push_back({row.name,row.symbols.text(),row.kind,row.trace,row.representation,row.groupId?mapping.at(row.groupId):0,row.protein});
    insert_records(std::move(records),position,std::move(catalog));
}
}
