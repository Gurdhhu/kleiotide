#pragma once
#include "core/alignment.hpp"
#include <istream>
#include <functional>
#include <ostream>
namespace kleiotide {
inline constexpr std::string_view session_magic="KLEIO\r\n\x1a";
inline constexpr unsigned session_version=5;
struct Session { Document document;std::string viewState,alignmentReport;unsigned formatVersion=session_version; };
// Versioned portable native session. Signals are embedded; paths are descriptive.
Session read_session(std::istream& input);
void write_session(std::ostream& output,const Document& document,
                   std::string_view viewState={},std::string_view alignmentReport={},
                   const std::function<bool()>& cancelled={});
}
