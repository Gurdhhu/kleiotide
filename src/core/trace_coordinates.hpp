#pragma once
#include "core/alignment.hpp"
namespace kleiotide {
// Display-only monotone transform. Original samples are never modified.
class TraceCoordinates {
public:
    explicit TraceCoordinates(const Sequence& row);
    double column_for_sample(double originalSample) const;
    double sample_for_column(double column) const;
private:
    double interpolate(double value,bool inverse) const;
    std::vector<std::pair<double,double>> knots_; // oriented sample, column center
    double lastSample_=0;
    bool reversed_=false;
};
}
