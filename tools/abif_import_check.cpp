#include "gui/main_window.hpp"
#include "core/abif.hpp"
#include "../tests/abif_fixture.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void check(QApplication& app,const QString& path){
    QFile input(path);need(input.open(QIODevice::ReadOnly),"Open input");const auto bytes=input.readAll();need(input.error()==QFileDevice::NoError,"Read input");std::istringstream in(bytes.toStdString(),std::ios::binary);
    std::string expectedNote;const auto expected=read_abif(in,"expected",path.toStdString(),&expectedNote);
    need(!expectedNote.empty(),"Input produces an import notice");
    MainWindow window;window.show();bool acknowledged=false;QString shown;
    QTimer timer;timer.setInterval(2);
    QObject::connect(&timer,&QTimer::timeout,[&]{if(auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())){shown=box->text();acknowledged=box->windowTitle()=="AB1 import notes"&&shown.contains(QString::fromStdString(expectedNote));box->accept();}});
    timer.start();window.open_path(path);QElapsedTimer elapsed;elapsed.start();
    while((!acknowledged||window.current_document().rows().size()!=1)&&elapsed.elapsed()<15000){app.processEvents();QThread::msleep(1);}
    timer.stop();need(acknowledged,"GUI displays and acknowledges the import notice");need(window.current_document().rows().size()==1,"Import completed");
    const auto& row=window.current_document().rows().front();const auto& a=*row.trace->original;const auto& b=*expected.trace->original;
    need(row.symbols.text()==expected.symbols&&a.calls==b.calls&&a.callSet==b.callSet&&a.peaks==b.peaks&&a.quality==b.quality&&a.channels==b.channels,"GUI retains normalized calls and complete evidence");
    std::cout<<"PASS GUI import: "<<a.calls.size()<<" calls, "<<a.channels[0].size()<<" samples/channel; "<<expectedNote<<'\n';
}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir temporary;need(temporary.isValid(),"Temporary directory");const auto path=temporary.filePath("unknown.ab1");
    {QFile out(path);need(out.open(QIODevice::WriteOnly),"Open synthetic fixture");const auto bytes=test::synthetic_abif("ACXxTG");need(out.write(bytes.data(),static_cast<qint64>(bytes.size()))==static_cast<qint64>(bytes.size())&&out.flush(),"Write synthetic fixture");}
    check(app,path);
    // argv[1] is the common workflow output directory. Additional paths allow
    // local acceptance with research files without distributing them in fixtures.
    for(int i=2;i<argc;++i)check(app,QString::fromLocal8Bit(argv[i]));
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
