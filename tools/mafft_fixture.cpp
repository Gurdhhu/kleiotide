#include "core/fasta.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <algorithm>
#include <iostream>
#include <sstream>
// Deterministic process fixture, built only with optional developer checks.
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);const auto args=app.arguments();const auto mode=qEnvironmentVariable("KLEIOTIDE_FIXTURE_MODE");
    if(args.contains("--fixture-child")){for(int tick=0;;++tick){QFile marker(qEnvironmentVariable("KLEIOTIDE_FIXTURE_MARKER"));if(marker.open(QIODevice::WriteOnly))marker.write(QByteArray::number(tick));QThread::msleep(20);}}
    if(args.contains("--version")){std::cerr<<(mode=="badversion"?"Unrelated program 1.0":(QString("v")+qEnvironmentVariable("KLEIOTIDE_FIXTURE_VERSION","7.525")+" (process fixture)").toStdString())<<'\n';return 0;}
    if(mode=="hang"){
        QProcess child;child.start(QCoreApplication::applicationFilePath(),{"--fixture-child"});if(!child.waitForStarted(1000))return 8;
        std::cerr<<"fixture-child-pid="<<child.processId()<<"\nfixture-align-ready\n"<<std::flush;
        for(;;)QThread::msleep(50);
    }
    const auto threadOption=args.indexOf("--thread");
    const auto threads=threadOption>=0?args.value(threadOption+1).toInt():0;
    if(const auto marker=qEnvironmentVariable("KLEIOTIDE_FIXTURE_ATTEMPTS");!marker.isEmpty()){
        QFile file(marker);if(file.open(QIODevice::WriteOnly|QIODevice::Append))file.write(QByteArray::number(threads)+'\n');
    }
    if(mode=="corealways"||((mode=="corefailure"||mode=="coreempty")&&threads!=1)){
        std::cerr<<"Cannot count the number of physical cores.\n";return mode=="coreempty"?0:17;
    }
    if(mode=="failure"){std::cerr<<"Fixture failure diagnostic\n";return 17;}
    if(mode=="empty")return 0;
    if(mode=="corrupt"){std::cout<<">kleio_999\nAT\n";return 0;}
    QFile file(args.back());if(!file.open(QIODevice::ReadOnly))return 9;std::istringstream input(file.readAll().toStdString());auto rows=kleiotide::read_fasta(input);
    int addition=args.indexOf("--add");if(addition<0)addition=args.indexOf("--addfragments");
    if(addition>=0){
        QFile added(args[addition+1]);if(!added.open(QIODevice::ReadOnly))return 10;std::istringstream addedInput(added.readAll().toStdString());auto targets=kleiotide::read_fasta(addedInput);std::size_t targetWidth=0;for(const auto& row:targets)targetWidth=std::max(targetWidth,row.symbols.size());const auto templateWidth=rows.front().symbols.size();
        for(auto& row:rows){row.symbols='-'+row.symbols+std::string(targetWidth,'-');}
        if(mode=="template-shift"&&rows.size()>1){rows[1].symbols.insert(0,1,'-');rows[1].symbols.pop_back();}
        for(auto& row:targets){row.symbols=std::string(templateWidth+1,'-')+row.symbols;row.symbols.resize(templateWidth+targetWidth+1,'-');rows.push_back(std::move(row));}
        for(const auto& row:rows)std::cout<<'>'<<row.name<<'\n'<<row.symbols<<'\n';return 0;
    }
    std::size_t width=0;for(const auto& row:rows)width=std::max(width,row.symbols.size()+1);
    for(auto& row:rows){row.symbols.resize(width,'-');std::cout<<'>'<<row.name<<'\n'<<row.symbols<<'\n';}return 0;
}
