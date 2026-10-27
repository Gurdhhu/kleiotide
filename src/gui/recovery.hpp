#pragma once
#include "core/alignment.hpp"
#include <QDateTime>
#include <QLockFile>
#include <QString>
#include <atomic>
#include <functional>
#include <memory>
namespace kleiotide {
struct RecoveryEntry {QString directory,id,path,title;QDateTime modified;QString previousPath;};
class RecoveryClaim {
public:
    explicit RecoveryClaim(RecoveryEntry);
    bool valid()const{return locked_;}
    const RecoveryEntry& entry()const{return entry_;}
    QString discard();
private:
    RecoveryEntry entry_;QLockFile lock_;bool locked_=false;
};
class RecoveryStore {
public:
    explicit RecoveryStore(QString directory=default_directory());
    ~RecoveryStore()=default; // An unclean destruction leaves valid recovery files.
    static QString default_directory();
    static std::vector<RecoveryEntry> available(const QString& directory=default_directory());
    QString write(const Document&,const std::string& view,const std::string& report,
                  const QString& title,const QString& source,std::size_t quotaBytes,const std::function<bool()>& cancelled={});
    QString discard();
    QString directory()const{return directory_;}
private:
    QString directory_,id_;std::unique_ptr<QLockFile> lock_;std::uint64_t generation_=0;
};
}
