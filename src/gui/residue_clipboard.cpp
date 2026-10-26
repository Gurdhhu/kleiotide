#include "gui/alignment_view.hpp"
#include <algorithm>
namespace kleiotide {
std::optional<std::vector<std::string>> AlignmentView::selected_residue_rows()const {
    if(!document_||!fragment_||fragment_->ids.size()!=1)return {};
    std::vector<std::string> result;bool any=false;
    for(auto id:fragment_->ids){const auto& s=document_->sequence(id).symbols.text();auto part=fragment_->start<s.size()?s.substr(fragment_->start,fragment_->width):std::string{};any|=!part.empty();result.push_back(std::move(part));}
    return any?std::optional(std::move(result)):std::nullopt;
}
bool AlignmentView::cut_residues(){
    if((mode_!=Mode::Insert&&mode_!=Mode::Rewrite)||!selected_residue_rows())return false;
    const auto first=fragment_->ids.front(),start=fragment_->start;delete_residues(false);
    clear_fragment();caretId_=first;caretColumn_=start;notify_selection();ensure_caret_visible();return true;
}
bool AlignmentView::paste_residue_rows(const std::vector<std::string>& residueRows){
    if(!document_||residueRows.size()!=1||(fragment_&&fragment_->ids.size()!=1))return false;
    return write_residues(residueRows.front());
}
}
