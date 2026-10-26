#include "core/alignment.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace kleiotide;
using Clock=std::chrono::steady_clock;
namespace {
double elapsed(Clock::time_point start){return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
std::uint64_t fingerprint(const Document& d){
    std::uint64_t hash=1469598103934665603ULL;
    for(const auto& row:d.rows()){hash=(hash^row.id)*1099511628211ULL;for(unsigned char c:row.symbols.text())hash=(hash^c)*1099511628211ULL;}
    return hash;
}
}
int main(int argc,char** argv){try{
    const auto maximum=argc>1?std::stoull(argv[1]):80000ULL;
    if(maximum>1000000)throw std::invalid_argument("Maximum is one million short rows.");
    std::cout<<"rows\terase_column_ms\tundo_ms\tinsert_gap_ms\terase_half_ms\n";
    for(const std::size_t count:{20000,40000,80000}){
        if(count>maximum)continue;
        std::vector<Record> records;records.reserve(count);
        for(std::size_t i=0;i<count;++i)records.push_back({"s"+std::to_string(i),std::string(200,'A'),RowKind::Nucleotide});
        Document d;d.load(std::move(records));std::vector<SequenceId> all;all.reserve(count);for(const auto& r:d.rows())all.push_back(r.id);
        const auto original=fingerprint(d);auto t=Clock::now();d.erase_columns(10,1);const auto erase=elapsed(t);
        t=Clock::now();d.undo();const auto undo=elapsed(t);if(fingerprint(d)!=original)throw std::runtime_error("Column Undo mismatch");
        t=Clock::now();d.edit_gaps(all,5,true);const auto gap=elapsed(t);
        std::vector<SequenceId> half(all.begin(),all.begin()+all.size()/2);
        t=Clock::now();d.erase_rows(half);const auto rows=elapsed(t);d.undo();d.undo();
        if(fingerprint(d)!=original)throw std::runtime_error("Structural/gap Undo mismatch");
        std::cout<<count<<'\t'<<erase<<'\t'<<undo<<'\t'<<gap<<'\t'<<rows<<std::endl;
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
