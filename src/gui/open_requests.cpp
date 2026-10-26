#include "gui/open_requests.hpp"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
namespace kleiotide {
QString OpenRequests::directory(const QString& key){return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/open-requests/"+key;}
OpenRequests::OpenRequests(QObject* parent):QObject(parent){
    timer_.setSingleShot(true);timer_.setInterval(500);poll_.setInterval(50);
    connect(&timer_,&QTimer::timeout,this,[this]{if(pending_.isEmpty())return;const auto paths=std::move(pending_);pending_.clear();if(opened)opened(paths);});
    connect(&poll_,&QTimer::timeout,this,[this]{for(const auto& name:QDir(directory_).entryList({"*.request"},QDir::Files,QDir::Name)){
        const auto path=directory_+'/'+name;QFile file(path);QByteArray bytes;
        if(file.open(QIODevice::ReadOnly)&&file.size()<=1024*1024)bytes=file.readAll();
        file.close();QFile::remove(path);
        QJsonParseError error;const auto request=QJsonDocument::fromJson(bytes,&error);if(error.error!=QJsonParseError::NoError||!request.isArray())continue;
        const auto array=request.array();if(array.size()>4096)continue;QStringList paths;for(const auto& value:array)if(value.isString()&&!value.toString().isEmpty())paths.append(value.toString());
        if(paths.isEmpty()){if(activate)activate();}else queue(paths);
    }});
}
bool OpenRequests::listen(const QString& key){
    directory_=directory(key);if(!QDir().mkpath(directory_))return false;
    QFile::setPermissions(directory_,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
    auto candidate=std::make_unique<QLockFile>(directory_+"/instance.lock");candidate->setStaleLockTime(0);
    if(!candidate->tryLock(0))return false;
    lock_=std::move(candidate);poll_.start();return true;
}
bool OpenRequests::forward(const QString& key,const QStringList& paths){
    const auto root=directory(key);if(!QFile::exists(root+"/instance.lock")||paths.size()>4096)return false;
    QJsonArray array;for(const auto& path:paths)array.append(path);const auto bytes=QJsonDocument(array).toJson(QJsonDocument::Compact);if(bytes.size()>1024*1024)return false;
    const auto name=QString::number(QDateTime::currentMSecsSinceEpoch())+'-'+QUuid::createUuid().toString(QUuid::Id128)+".request";
    QSaveFile file(root+'/'+name);if(!file.open(QIODevice::WriteOnly))return false;file.setPermissions(QFile::ReadOwner|QFile::WriteOwner);
    return file.write(bytes)==bytes.size()&&file.commit();
}
void OpenRequests::queue(const QStringList& paths){for(const auto& path:paths)if(!pending_.contains(path))pending_.append(path);if(!pending_.isEmpty())timer_.start();}
}
