#include "gui/sequence_selection.hpp"
#include <QHash>
#include <QSet>
namespace kleiotide {
std::vector<SequenceId> select_titles(const Document& document,const QStringList& patterns,TitleMatch method,bool matchCase){
    const auto sensitivity=matchCase?Qt::CaseSensitive:Qt::CaseInsensitive;std::vector<SequenceId> result;
    QSet<QString> exact;
    if(method==TitleMatch::Exact)for(const auto& pattern:patterns)if(!pattern.trimmed().isEmpty())exact.insert(matchCase?pattern:pattern.toCaseFolded());
    for(const auto& row:document.rows()){
        const auto name=QString::fromUtf8(row.name.data(),static_cast<qsizetype>(row.name.size()));
        if(method==TitleMatch::Exact){if(exact.contains(matchCase?name:name.toCaseFolded()))result.push_back(row.id);continue;}
        for(const auto& pattern:patterns){if(pattern.trimmed().isEmpty())continue;
            const bool match=method==TitleMatch::Exact?name.compare(pattern,sensitivity)==0:method==TitleMatch::Prefix?name.startsWith(pattern,sensitivity):name.contains(pattern,sensitivity);
            if(match){result.push_back(row.id);break;}}
    }
    return result;
}
std::vector<SequenceId> select_duplicate_titles(const Document& document,DuplicateTitle part,bool matchCase,bool additionalOnly){
    QHash<QString,std::vector<SequenceId>> groups;
    for(const auto& row:document.rows()){
        auto name=QString::fromUtf8(row.name.data(),static_cast<qsizetype>(row.name.size()));
        if(part!=DuplicateTitle::Full){const auto position=name.indexOf(part==DuplicateTitle::BeforeParenthesis?'(':'|');if(position>=0)name.truncate(position);name=name.trimmed();}
        if(!matchCase)name=name.toCaseFolded();
        groups[name].push_back(row.id);
    }
    QSet<quint64> chosen;for(auto it=groups.cbegin();it!=groups.cend();++it)if(it.value().size()>1)for(std::size_t i=additionalOnly?1:0;i<it.value().size();++i)chosen.insert(it.value()[i]);
    std::vector<SequenceId> result;for(const auto& row:document.rows())if(chosen.contains(row.id))result.push_back(row.id);return result;
}
}
