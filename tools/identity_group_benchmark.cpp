#include "core/fasta.hpp"
#include "core/identity_groups.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
int main(int argc,char** argv){
    try{
        if(argc<2||argc>5){std::cerr<<"Usage: kleiotide_identity_group_benchmark FASTA [maximum_candidates=16; 0=exhaustive] [row_limit=0; 0=all] [preview.tsv]\n";return 2;}
        std::ifstream input(argv[1]);if(!input)throw std::runtime_error("Cannot open input FASTA.");
        auto rows=read_fasta(input);const auto limit=argc>3?std::stoull(argv[3]):0;if(limit&&rows.size()>limit)rows.resize(static_cast<std::size_t>(limit));
        Document document;document.load(std::move(rows));IdentityGroupOptions options;options.alignment=IdentityAlignment::Pairwise;if(argc>2)options.maximumCandidates=static_cast<std::size_t>(std::stoull(argv[2]));
        const auto start=std::chrono::steady_clock::now();const auto cpu=std::clock();
        auto plan=plan_identity_groups(document,options);
        const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();const auto cpuSeconds=double(std::clock()-cpu)/CLOCKS_PER_SEC;
        std::cout<<document.rows().size()<<" sequences; "<<plan.clusters<<" clusters; "<<plan.comparisons<<" centroid comparisons; "<<plan.shortlistedQueries<<" queries shortlisted; "<<seconds<<" wall seconds; "<<cpuSeconds<<" CPU seconds\n";
        if(argc>4){std::ofstream output(argv[4],std::ios::binary);if(!output)throw std::runtime_error("Cannot open preview TSV.");output<<identity_groups_tsv(document,plan);if(!output)throw std::runtime_error("Cannot write preview TSV.");}
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
