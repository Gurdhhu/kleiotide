#pragma once
#include "core/alignment.hpp"
#include <QStringList>
namespace kleiotide {
enum class TitleMatch { Exact,Prefix,Contains };
enum class DuplicateTitle { Full,BeforeParenthesis,BeforePipe };
std::vector<SequenceId> select_titles(const Document&,const QStringList&,TitleMatch,bool matchCase);
std::vector<SequenceId> select_duplicate_titles(const Document&,DuplicateTitle,bool matchCase,bool additionalOnly);
}
