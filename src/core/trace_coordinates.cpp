#include "core/trace_coordinates.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace kleiotide {
TraceCoordinates::TraceCoordinates(const Sequence& row){
    if(!row.trace)throw std::invalid_argument("Trace coordinates need linked evidence.");
    validate_trace(row);const auto& link=*row.trace;reversed_=link.reversed;lastSample_=static_cast<double>(link.original->channels[0].size()-1);
    const auto positions=trace_sample_positions(link);
    std::size_t ordinal=0;std::vector<std::pair<double,double>> anchors;
    for(std::size_t c=0;c<row.symbols.size();++c)if(!is_gap(row.symbols.text()[c])){
        const auto raw=positions[ordinal++];
        anchors.emplace_back(reversed_?lastSample_-raw:raw,static_cast<double>(c)+.5);
    }
    // Coincident imported peaks share the mean column of their surviving calls.
    for(std::size_t i=0;i<anchors.size();){auto j=i+1;double sum=anchors[i].second;while(j<anchors.size()&&anchors[j].first==anchors[i].first)sum+=anchors[j++].second;
        knots_.emplace_back(anchors[i].first,sum/static_cast<double>(j-i));i=j;
    }
    if(knots_.empty()){knots_={{0.,-.5},{std::max(1.,lastSample_),static_cast<double>(row.symbols.size())+.5}};}
    else{
        if(knots_.front().first>0.)knots_.insert(knots_.begin(),{0.,-.5});
        if(knots_.back().first<lastSample_)knots_.emplace_back(lastSample_,static_cast<double>(row.symbols.size())+.5);
        if(knots_.size()==1)knots_.emplace_back(knots_[0].first+1.,knots_[0].second+1.);
    }
}
double TraceCoordinates::interpolate(double value,bool inverse) const {
    auto coordinate=[&](const auto& point){return inverse?point.second:point.first;};
    const auto it=std::upper_bound(knots_.begin(),knots_.end(),value,[&](double v,const auto& point){return v<coordinate(point);});
    const auto end=it==knots_.begin()?knots_.begin()+1:it==knots_.end()?knots_.end()-1:it;
    const auto& a=*(end-1);const auto& b=*end;const double t=(value-coordinate(a))/(coordinate(b)-coordinate(a));
    return inverse?a.first+(b.first-a.first)*t:a.second+(b.second-a.second)*t;
}
double TraceCoordinates::column_for_sample(double sample)const {return interpolate(reversed_?lastSample_-sample:sample,false);}
double TraceCoordinates::sample_for_column(double column)const {const auto sample=interpolate(column,true);return reversed_?lastSample_-sample:sample;}
}
