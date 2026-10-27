#include "core/overview.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace kleiotide;
namespace {void need(bool v,const char* m){if(!v)throw std::runtime_error(m);}}
int main(){try{
    std::mt19937 rng(20261026);
    for(int trial=0;trial<60;++trial){
        std::vector<Record> rows;
        for(int r=0;r<5;++r){std::string text;for(int i=0,n=int(rng()%500);i<n;++i)text+="ACGTURYSWKMBDHVN?~-"[rng()%19];rows.push_back({std::to_string(r),text});}
        Document d;d.load(rows);OverviewScan scan(d,4096,7);
        while(!scan.step(d,1+rng()%40)){}
        need(scan.workspace_bytes()<=4096,"Workspace bound");
        for(int n=1;n<10;++n)for(const auto& bin:scan.bins(n)){
            double coverage=0,conservation=0;std::size_t informative=0;
            for(auto col=bin.begin;col<bin.end;++col){NucleotideEvidence evidence;for(const auto& row:rows)if(col<row.symbols.size())evidence.add(row.symbols[col]);coverage+=double(evidence.residues)/rows.size();
                std::uint64_t calls=0;for(auto v:evidence.bases)calls+=v;
                if(calls){++informative;conservation+=double(*std::max_element(evidence.bases.begin(),evidence.bases.end()))/calls;}}
            need(std::abs(bin.residues-coverage/(bin.end-bin.begin))<1e-12,"Tiled coverage equals exact column model");
            need(bin.hasInformation==(informative!=0)&&std::abs(bin.conservation-(informative?conservation/informative:0))<1e-12,"Tiled conservation equals exact column model");
        }
    }
    std::string text;for(int i=0;i<1000000;++i)text+=(i%2?'C':'A');
    Document d;d.load({{"one",text},{"two",text},{"mask","1111",RowKind::Mask}});
    OverviewScan large(d,256*1024,1024);need(large.workspace_bytes()<=256*1024,"Million-column scan remains bounded");
    while(!large.step(d,4096)){}
    for(const auto& bin:large.bins(600))need(bin.conservation==1&&bin.residues==1,"Different conserved bases never pool into false variation");
    const auto original=d.content_id();d.rename({{d.rows()[0].id,"renamed"}});need(d.content_id()==original&&large.step(d,0),"Metadata rename does not invalidate overview");
    d.replace(d.rows()[0].id,0,1,"T");bool refused=false;try{large.step(d);}catch(const std::logic_error&){refused=true;}need(refused,"Changed content invalidates result");
    refused=false;try{OverviewScan tooSmall(d,1);}catch(const std::length_error&){refused=true;}need(refused,"Insufficient workspace is rejected before full-length allocation");
    std::cout<<"PASS bounded overview: independent randomized column model, IUPAC, million-column conservation, memory bound and revision checks\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
