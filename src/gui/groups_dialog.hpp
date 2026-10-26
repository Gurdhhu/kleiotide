#pragma once
#include "core/alignment.hpp"
class QWidget;
namespace kleiotide {
bool manage_groups(Document&,const std::vector<SequenceId>&,QWidget*);
bool assign_group(Document&,const std::vector<SequenceId>&,QWidget*);
bool import_groups(Document&,QWidget*);
std::optional<std::vector<SequenceId>> select_groups(const Document&,QWidget*);
}
