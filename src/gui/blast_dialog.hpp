#pragma once
#include "core/alignment.hpp"
#include <memory>
#include <QUrl>
class QWidget;
namespace kleiotide {
struct BlastServices {QUrl search{QStringLiteral("https://blast.ncbi.nlm.nih.gov/Blast.cgi")};QUrl records{QStringLiteral("https://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi")};int contactMs=10000,pollMs=60000;int testTimeoutMs=0; // Loopback fixture override only; ignored for NCBI.
QString jobDirectory;};
void blast_dialog(std::shared_ptr<Document>,const std::vector<SequenceId>&,bool protein,QWidget*,BlastServices services={});}
