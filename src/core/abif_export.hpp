#pragma once
#include "core/alignment.hpp"
#include <functional>
#include <ostream>
namespace kleiotide {
using AbifCancel=std::function<bool()>;
struct AbifExport {
    std::string calls,sampleName;
    std::vector<std::uint16_t> peaks;
    std::vector<std::uint8_t> quality;
    std::shared_ptr<const Chromatogram> signal;
    std::array<unsigned,4> channelTypes{};
    bool reversed=false;
    std::size_t inserted=0,substituted=0,measuredQuality=0,missingQuality=0,omittedGaps=0,coincident=0;
};
// Standalone ABIF 1.01 compatibility export from the current row and embedded evidence.
// Call sets 1 and 2 both contain the edited sequence; measured signal is never resampled.
AbifExport prepare_abif_export(const Sequence&,AbifCancel={});
void write_abif_export(std::ostream&,const AbifExport&,AbifCancel={});
}
