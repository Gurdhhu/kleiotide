#include "core/abif_export.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace kleiotide;
int main(int argc,char** argv){try{
    if(argc<2)throw std::runtime_error("Pass an output directory, optionally followed by real AB1 paths.");const auto directory=std::filesystem::path(argv[1]);std::filesystem::create_directories(directory);std::ofstream manifest(directory/"manifest.tsv");manifest<<"file\toriginal\treversed\tcalls\tpeaks\tquality\n";int outputs=0;
    auto emit=[&](const Document& d,const std::string& name,const std::string& original){const auto p=prepare_abif_export(d.sequence(1));std::ofstream output(directory/name,std::ios::binary);write_abif_export(output,p);manifest<<name<<'\t'<<original<<'\t'<<p.reversed<<'\t'<<p.calls<<'\t';for(auto v:p.peaks)manifest<<v<<',';manifest<<'\t';for(auto q:p.quality)manifest<<static_cast<unsigned>(q)<<',';manifest<<'\n';++outputs;};
    std::vector<std::string> sources;{std::ofstream out(directory/"synthetic-source.ab1",std::ios::binary);out<<test::synthetic_abif("ACGTRNACGTTACGTA");}sources.push_back((directory/"synthetic-source.ab1").string());for(int i=2;i<argc;++i)sources.push_back(argv[i]);
    for(std::size_t i=0;i<sources.size();++i){const auto original="source"+std::to_string(i)+".ab1";std::filesystem::copy_file(sources[i],directory/original,std::filesystem::copy_options::overwrite_existing);std::ifstream input(directory/original,std::ios::binary);Document baseline;baseline.load({read_abif(input,"sample Я",sources[i])});emit(baseline,"s"+std::to_string(i)+"-unchanged.ab1",original);auto edited=baseline;const auto n=edited.sequence(1).symbols.size();if(n>4){edited.replace(1,2,1,edited.sequence(1).symbols.text()[2]=='A'?"C":"A");edited.replace(1,4,0,"nR");edited.replace(1,0,1,"");edited.replace(1,1,0,"-~~");}emit(edited,"s"+std::to_string(i)+"-edited.ab1",original);edited.transform({1},Document::Transform::ReverseComplement);emit(edited,"s"+std::to_string(i)+"-reverse.ab1",original);
        auto crowded=baseline;crowded.replace(1,1,0,std::string(80,'N'));emit(crowded,"s"+std::to_string(i)+"-crowded.ab1",original);auto newcalls=baseline;newcalls.replace(1,0,newcalls.sequence(1).symbols.size(),"");newcalls.replace(1,0,0,"ACGtNR?ACGT");emit(newcalls,"s"+std::to_string(i)+"-unanchored.ab1",original);
    }
    for(int n=1;n<=4;++n){const auto original="tiny-source"+std::to_string(n)+".ab1";{std::ofstream out(directory/original,std::ios::binary);out<<test::synthetic_abif(std::string("ACGT").substr(0,static_cast<std::size_t>(n)));}std::ifstream input(directory/original,std::ios::binary);Document d;d.load({read_abif(input,"tiny","")});emit(d,"tiny"+std::to_string(n)+".ab1",original);}
    std::cout<<"PASS wrote "<<outputs<<" AB1 compatibility fixtures and manifest\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
