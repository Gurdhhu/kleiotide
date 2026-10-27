#include "gui/recovery.hpp"
#include "gui/file_output.hpp"
#include "core/session.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>
#include <map>
#include <streambuf>
#include <stdexcept>
namespace kleiotide {
namespace {
bool valid_id(const QString& id){return QRegularExpression("\\A[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\\z").match(id).hasMatch();}
QString remove_group(const QString& directory,const QString& id){
    if(!valid_id(id))return "Invalid recovery identity.";
    for(int i=0;i<2;++i)for(const auto* suffix:{".kleio",".json"}){
        const auto file=QDir(directory).filePath(id+"_g"+QString::number(i)+suffix);
        if(QFileInfo::exists(file)&&!QFile::remove(file))return "Could not remove recovery file: "+file;
    }
    return {};
}
class LimitedBuffer:public std::streambuf {
public:LimitedBuffer(std::streambuf* out,std::size_t maximum,const std::function<bool()>& cancelled):out_(out),remaining_(maximum),cancelled_(cancelled){}
protected:
    std::streamsize xsputn(const char* bytes,std::streamsize count)override{
        if(cancelled_&&cancelled_())throw std::runtime_error("Recovery checkpoint cancelled.");
        if(count<0||static_cast<std::uint64_t>(count)>remaining_)throw std::length_error("Recovery checkpoint exceeds the retained disk quota. Previous checkpoints are unchanged.");
        const auto written=out_->sputn(bytes,count);remaining_-=std::size_t(written);return written;
    }
    int_type overflow(int_type ch)override{if(traits_type::eq_int_type(ch,traits_type::eof()))return traits_type::not_eof(ch);const char c=traits_type::to_char_type(ch);return xsputn(&c,1)==1?ch:traits_type::eof();}
    int sync()override{return out_->pubsync();}
private:std::streambuf* out_;std::size_t remaining_;const std::function<bool()>& cancelled_;
};
}
QString RecoveryStore::default_directory(){return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)+"/recovery";}
RecoveryStore::RecoveryStore(QString directory):directory_(std::move(directory)),id_(QUuid::createUuid().toString(QUuid::WithoutBraces)){
    if(!QDir().mkpath(directory_))throw std::runtime_error("Cannot create the recovery directory.");
    lock_=std::make_unique<QLockFile>(QDir(directory_).filePath(id_+".lock"));lock_->setStaleLockTime(0);
    if(!lock_->tryLock(0))throw std::runtime_error("Cannot lock the recovery identity.");
}
RecoveryClaim::RecoveryClaim(RecoveryEntry entry):entry_(std::move(entry)),lock_(QDir(entry_.directory).filePath(entry_.id+".lock")){
    lock_.setStaleLockTime(0);if(valid_id(entry_.id))locked_=lock_.tryLock(0);
}
QString RecoveryClaim::discard(){return locked_?remove_group(entry_.directory,entry_.id):"Recovery is in use by another window or process.";}
QString RecoveryStore::discard(){return remove_group(directory_,id_);}
std::vector<RecoveryEntry> RecoveryStore::available(const QString& directory){
    QDir dir(directory);std::map<QString,std::pair<RecoveryEntry,std::uint64_t>> grouped;
    for(const auto& info:dir.entryInfoList({"*_g0.kleio","*_g1.kleio"},QDir::Files|QDir::NoSymLinks,QDir::Time)){
        const auto id=info.fileName().left(36);
        if(!valid_id(id)||(info.fileName()!=id+"_g0.kleio"&&info.fileName()!=id+"_g1.kleio"))continue;
        QLockFile lock(dir.filePath(id+".lock"));lock.setStaleLockTime(0);if(!lock.tryLock(0))continue;
        RecoveryEntry entry{dir.absolutePath(),id,info.absoluteFilePath(),{},info.lastModified(),{}};
        std::uint64_t ordinal=0;QFile metadata(dir.filePath(info.completeBaseName()+".json"));
        const QFileInfo metadataInfo(metadata);
        if(metadataInfo.isFile()&&!metadataInfo.isSymLink()&&metadataInfo.size()<=16384&&metadata.open(QIODevice::ReadOnly)){
            const auto object=QJsonDocument::fromJson(metadata.readAll()).object();entry.title=object["source"].toString();if(entry.title.isEmpty())entry.title=object["title"].toString();ordinal=object["generation"].toString().toULongLong();
        }
        if(entry.title.isEmpty())entry.title="Unsaved session";
        const auto found=grouped.find(id);
        if(found==grouped.end())grouped.emplace(id,std::make_pair(std::move(entry),ordinal));
        else if(entry.modified>found->second.first.modified||(entry.modified==found->second.first.modified&&ordinal>found->second.second)){
            entry.previousPath=found->second.first.path;found->second={std::move(entry),ordinal};
        }else found->second.first.previousPath=entry.path;
    }
    std::vector<RecoveryEntry> entries;entries.reserve(grouped.size());for(auto& [id,pair]:grouped){(void)id;entries.push_back(std::move(pair.first));}
    std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.modified>b.modified;});return entries;
}
QString RecoveryStore::write(const Document& d,const std::string& view,const std::string& report,const QString& title,const QString& source,std::size_t quota,const std::function<bool()>& cancelled){
    QLockFile quotaLock(QDir(directory_).filePath("checkpoint-write.lock"));quotaLock.setStaleLockTime(0);
    if(!quotaLock.tryLock(0))return "Another recovery checkpoint is being written; retry after it finishes.";
    const auto next=generation_+1;const auto stem=id_+"_g"+QString::number(next%2);const auto path=QDir(directory_).filePath(stem+".kleio");
    std::uint64_t retained=0;
    for(const auto& file:QDir(directory_).entryInfoList({"*.kleio"},QDir::Files|QDir::NoSymLinks)){
        if(file.absoluteFilePath()==QFileInfo(path).absoluteFilePath())continue;
        const auto size=std::uint64_t(file.size());if(size>quota||retained>quota-size)return "Recovery disk quota is full; previous checkpoints are unchanged.";retained+=size;
    }
    if(retained>=quota)return "Recovery disk quota is full; previous checkpoints are unchanged.";
    const auto result=write_output_atomic(path,[&](std::ostream& out){LimitedBuffer buffer(out.rdbuf(),quota-std::size_t(retained),cancelled);std::ostream limited(&buffer);limited.exceptions(std::ios::badbit|std::ios::failbit);write_session(limited,d,view,report,cancelled);limited.flush();},cancelled);
    if(!result.isEmpty())return result;
    generation_=next;
    const auto metadata=QJsonDocument(QJsonObject{{"source",source},{"title",title},{"generation",QString::number(next)},{"created",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}}).toJson(QJsonDocument::Compact);
    const auto error=write_output_atomic(QDir(directory_).filePath(stem+".json"),[&](std::ostream& out){out.write(metadata.constData(),metadata.size());});
    return error.isEmpty()?QString{}:"Checkpoint data saved, but its label could not be saved: "+error;
}
}
