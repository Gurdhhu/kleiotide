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
    need(large.update(d)&&large.scan_end()-large.scan_begin()<1000,"Single overwrite invalidates only one bounded summary");
    auto compare=[&]{while(!large.step(d,127)){}OverviewScan fresh(d,256*1024,1024);while(!fresh.step(d,4096)){}const auto a=large.bins(600),b=fresh.bins(600);need(a.size()==b.size(),"Incremental bin count");for(std::size_t i=0;i<a.size();++i)need(a[i].begin==b[i].begin&&a[i].end==b[i].end&&std::abs(a[i].residues-b[i].residues)<1e-12&&std::abs(a[i].conservation-b[i].conservation)<1e-12&&a[i].hasInformation==b[i].hasInformation,"Incremental equals independent full rescan");};
    compare();
    for(int n=0;n<30;++n){const auto c=std::size_t(rng()%1000000);d.replace(d.rows()[rng()%2].id,c,1,std::string(1,"ACGTNR~"[rng()%7]));need(large.update(d),"Column-preserving overwrite accepted");if(n%3==0){large.step(d,3);d.replace(1,c,1,"Y");need(large.update(d),"Rapid edits coalesce during partial scan");}compare();d.undo();need(large.update(d),"Undo invalidates only touched bins");compare();d.redo();need(large.update(d),"Redo invalidates only touched bins");compare();}
    d.replace(1,0,0,"A");need(!large.update(d),"Length changes require full rescan");d.undo();
    OverviewScan stale(d,256*1024,1024);while(!stale.step(d,65536)){}for(int n=0;n<65;++n)d.replace(1,n,1,n%2?"T":"G");need(!stale.update(d),"Expired change log requires full rescan");
    Document single;single.load({{"one","ACGTRYSWKMBDHVN?~-"}});OverviewScan singleScan(single,4096,7);while(!singleScan.step(single,2)){}
    for(const auto& b:singleScan.bins(5)){double votes=0,cover=0;std::size_t info=0;for(auto c=b.begin;c<b.end;++c){NucleotideEvidence e;e.add(single.rows()[0].symbols.text()[c]);cover+=e.residues;std::uint64_t sum=0;for(auto v:e.bases)sum+=v;if(sum){++info;votes+=double(*std::max_element(e.bases.begin(),e.bases.end()))/sum;}}need(std::abs(b.residues-cover/(b.end-b.begin))<1e-12&&std::abs(b.conservation-(info?votes/info:0))<1e-12,"Single-row fast path preserves IUPAC semantics");}
    refused=false;try{OverviewScan tooSmall(d,1);}catch(const std::length_error&){refused=true;}need(refused,"Insufficient workspace is rejected before full-length allocation");
    std::cout<<"PASS bounded overview: independent randomized column model, IUPAC, million-column conservation, memory bound and revision checks\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
