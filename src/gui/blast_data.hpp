#pragma once
#include "core/alignment.hpp"
#include <QByteArray>
#include <QJsonObject>
#include <QStringList>
#include <QUrlQuery>
namespace kleiotide {
struct BlastQuery {QString id,sequence;QStringList names;};
struct BlastOptions {
 bool protein=false,excludeEnvironmental=false,mask=true,softMask=true,shortQuery=false;QString database="core_nt",task="megablast",email,entrez,launcher,strand="both",matrix="BLOSUM62";
 int maxHits=10,word=28,reward=1,penalty=-2,gapOpen=0,gapExtend=0,composition=2;double expect=.05,identity=0,coverage=0;
 QString error()const;bool toolkit()const;QJsonObject json()const;static BlastOptions from_json(const QJsonObject&);
};
struct BlastHit {QString queryId,accession,species,identifier,title,taxonomy,authors,date;double identity=0,coverage=0,evalue=0,bits=0;int length=0,mismatches=0,qstart=0,qend=0,hstart=0,hend=0;};
std::vector<BlastQuery> blast_queries(const Document&,const std::vector<SequenceId>&,bool protein);
std::vector<std::vector<BlastQuery>> blast_batches(const std::vector<BlastQuery>&,std::size_t maxResidues=100000,std::size_t maxQueries=50);
QString blast_fasta(const std::vector<BlastQuery>&);
QUrlQuery blast_parameters(const BlastOptions&,const std::vector<BlastQuery>&);
QStringList blast_arguments(const BlastOptions&,const QString& input);
std::vector<BlastHit> parse_blast(const QByteArray&,const std::vector<BlastQuery>&);
void enrich_blast(std::vector<BlastHit>&,const QByteArray& xml);
QStringList blast_columns();
using BlastTable=std::vector<QStringList>;
BlastTable blast_table(const std::vector<BlastQuery>&,const std::vector<BlastHit>&,const BlastOptions&);
QByteArray blast_tsv(const BlastTable&);
QByteArray blast_xlsx(const BlastTable&);
}
