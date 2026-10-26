#pragma once
#include "core/alignment.hpp"
#include <istream>
namespace kleiotide {
// ABIF 1.x reader, including uniquely validated stale name-offset recovery.
// X/x calls normalize to N/n; recoveryNote reports both kinds of import notice.
Record read_abif(std::istream& input,std::string title,std::string sourcePath = {},std::string* recoveryNote=nullptr);
}
