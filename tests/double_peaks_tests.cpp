#include "core/double_peaks.hpp"
#include "double_peak_fixture.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace kleiotide;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Action> void rejects(Action action, const char* message) {
    try {
        action();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}
}

int main() try {
    Document document;
    document.load({test::double_peak_read("forward", true),
        test::double_peak_read("without Q", false), {"unlinked", "AAAA"}});
    const auto evidence = document.sequence(1).trace;
    const auto state = document.state_id();

    const auto plan = plan_double_peaks(document, {}, {1, 3});
    require(plan.rows == 1 && plan.sites == 6 && plan.calls.size() == 2 && plan.thirdPeakExcluded == 1,
        "Selected scope, third peak rejection, displacement, height, prominence and deleted-call exclusion");
    const auto& supported = plan.calls[0];
    require(supported.column == 0 && supported.current == 'A' && supported.proposed == 'R' &&
        supported.secondaryPercent == 50 && supported.displacementSamples == 4 &&
        supported.displacementPercent == 20 && !supported.needsReview,
        "Relative peak height and displacement include their exact boundaries");
    const auto& review = plan.calls[1];
    require(review.column == 5 && review.current == 'g' && review.proposed == 'm' &&
        review.needsReview && review.secondaryPercent == 60,
        "A signal pair inconsistent with the current call needs review and keeps its case");
    require(document.state_id() == state && !document.can_undo(), "Preview leaves the source untouched");

    const auto all = plan_double_peaks(document, {true}, {});
    require(all.rows == 2 && all.calls.size() == 4 &&
        all.calls[0].secondaryPercent == all.calls[2].secondaryPercent &&
        all.calls[1].proposed == all.calls[3].proposed,
        "Rows with and without Phred Q produce identical calls");
    const auto relaxed = plan_double_peaks(document, {false, 50, 30, 35}, {1});
    require(relaxed.calls.size() == 3 && relaxed.calls[1].column == 3 &&
        relaxed.calls[1].proposed == 'W', "Relaxing displacement makes the shifted double peak eligible");
    const auto zero = plan_double_peaks(document, {false, 0, 100, 0}, {1});
    require(zero.calls.size() == 1 && zero.calls[0].column == 2 &&
        zero.calls[0].proposed == 'M', "Zero displacement requires coincident peak apices");
    rejects([&] { plan_double_peaks(document, {false, 101}, {1}); }, "Out-of-range threshold");
    rejects([&] { plan_double_peaks(document, {}, {1}, [] { return true; }); }, "Cancelled calculation");

    const std::array<unsigned char, 2> include{1, 0};
    apply_double_peaks(document, plan, include);
    require(document.sequence(1).symbols.text() == "R~CACgG" && document.sequence(1).trace == evidence &&
        !trace_position(document.sequence(1), 0).quality &&
        trace_position(document.sequence(1), 0).originalCall == 0 && evidence->original->quality.front() == 0,
        "Apply only checked calls and preserve original call, signal and Phred evidence");
    rejects([&] { apply_double_peaks(document, plan, include); }, "A stale preview cannot edit again");
    document.undo();
    require(document.state_id() == state && document.sequence(1).symbols.text() == "A~CACgG",
        "One Undo restores the source");
    document.redo();
    require(document.sequence(1).symbols.text() == "R~CACgG" &&
        plan_double_peaks(document, {}, {1}).calls.size() == 1, "Redo and identical IUPAC no-op");

    document.transform({2}, Document::Transform::ReverseComplement);
    const auto reverse = plan_double_peaks(document, {}, {2});
    require(reverse.calls.size() == 2 && reverse.calls[0].column == 1 && reverse.calls[0].proposed == 'k' &&
        reverse.calls[1].column == 6 && reverse.calls[1].proposed == 'Y' && reverse.calls[1].major == 'T',
        "Reverse-strand proposals use complemented bases and current alignment columns");

    Document anchored;
    auto manual = test::double_peak_read("manual and unanchored", false);
    manual.symbols += "NC";
    auto link = std::make_shared<TraceLink>(*manual.trace);
    link->originalCalls.insert(link->originalCalls.end(), {-1, -1});
    link->adjustedPeaks = {-1, -1, -1, -1, -1, -1, -1, 157};
    manual.trace = link;
    auto inferred = manual;
    inferred.name = "inferred";
    auto hints = std::make_shared<TraceLink>(*link);
    hints->adjustedPeaks.clear();
    hints->inferredPeaks = {-1, -1, -1, -1, -1, -1, -1, 157};
    inferred.trace = hints;
    anchored.load({std::move(manual), std::move(inferred)});
    const auto anchoredPlan = plan_double_peaks(anchored, {true}, {});
    const auto terminalUpdates = std::count_if(anchoredPlan.calls.begin(), anchoredPlan.calls.end(),
        [](const auto& row) { return row.column == 8 && row.proposed == 'Y' && !row.needsReview; });
    require(anchoredPlan.unanchored == 2 && terminalUpdates == 2,
        "Manual and inferred anchors are eligible, interpolated positions are ignored");

    std::cout << "PASS double peak thresholds, placement, review, trace evidence and history\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "FAIL " << error.what() << '\n';
    return 1;
}
