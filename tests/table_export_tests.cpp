#include "core/version.hpp"
#include "gui/blast_data.hpp"
#include "gui/table_export.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardItemModel>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
void need(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
void save(const QString &path, const QByteArray &bytes) {
  QFile f(path);
  need(f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size(),
       "Write export fixture");
}
int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  try {
    const auto out =
        QCoreApplication::applicationDirPath() + "/qa/table_exports";
    QDir().mkpath(out);
    TableSheet settings{"Settings",
                        {{"Setting", "Value"}, {"Version", version}}};
    TableSheet values{
        "Results",
        {{"Name", "Value", "Accession"},
         {"=literal, name", "1.25", "AB123456.1"},
         {"001", "-2", "literal"}},
        {1},
        {{"C2", QUrl("https://www.ncbi.nlm.nih.gov/nuccore/AB123456.1")}}};
    auto bytes = report_xlsx({settings, values});
    save(out + "/styled.xlsx", bytes);
    QStringList names;
    read_xlsx_table(out + "/styled.xlsx", 0, &names);
    const auto table = read_xlsx_table(out + "/styled.xlsx", 1);
    need(names == QStringList{"Settings", "Results"} && table.size() == 3 &&
             table[1][0] == "=literal, name" && table[2][0] == "001" &&
             table[2][1] == "-2",
         "Multi-sheet roundtrip preserves text/numeric cells");
    const auto csv = report_delimited({settings, values}, ',');
    save(out + "/styled.csv", csv);
    need(csv.contains("# Settings") && csv.contains("# Results") &&
             csv.contains("\"'=literal, name\"") && csv.contains(",-2,"),
         "CSV settings first, comma quoting, literal formula names, signed "
         "numbers");
    Document d;
    d.load({{"=one", "AAAA"}, {"two", "AAAT"}, {"other", "NNNN"}});
    d.apply_groups({{1, "North", 0x112233}}, {{1, 1}, {2, 1}});
    DistanceOptions o;
    auto distances = calculate_distances(d, o, DistanceReport::Matrix);
    const auto matrix = distance_workbook(distances);
    need(matrix.size() == 7 && matrix[0].name == "Calculation methods" &&
             matrix[1].name == "Distance matrix",
         "All distance data blocks have separate sheets");
    save(out + "/distances.xlsx", report_xlsx(matrix));
    for (const auto kind :
         {DistanceReport::Overall, DistanceReport::WithinGroups,
          DistanceReport::BetweenGroups}) {
      const auto means = distance_workbook(calculate_distances(d, o, kind));
      need(means[1].rows.front().size() == 13,
           "Mean report has full summary shape");
    }
    auto groupPlan = plan_identity_groups(d, IdentityGroupOptions{});
    auto groups = grouping_workbook(d, groupPlan);
    need(groups.size() == 2 && groups[0].rows.size() > 8 &&
             groups[1].rows.size() == 4,
         "Grouping workbook includes rules/settings and every result row");
    save(out + "/groups.xlsx", report_xlsx(groups));
    BlastOptions options;
    BlastQuery query{"q1", "ACGTACGTACGT", {"=query"}, {1}};
    BlastHit hit;
    hit.queryId = "q1";
    hit.accession = "AB123456.1";
    hit.species = "Species";
    hit.identifier = "Identifier";
    hit.identity = 100;
    hit.coverage = 100;
    hit.hsps = 2;
    hit.length = 12;
    hit.qstart = hit.hstart = 1;
    hit.qend = hit.hend = 12;
    hit.evalue = 1e-30;
    hit.bits = 40;
    hit.title = "Title";
    hit.taxonomy = "Taxonomy";
    hit.authors = "Authors";
    hit.date = "2020-01-01";
    const auto hits = blast_table({query}, {hit}, options);
    auto blast =
        blast_report(hits, options, "2026-10-31T12:00:00Z", "BLASTN test");
    need(blast.size() == 2 && blast[1].links.contains("E2") &&
             blast[0].rows.size() > 10,
         "BLAST settings and styled links in second worksheet");
    save(out + "/blast.xlsx", report_xlsx(blast));
    save(out + "/blast.csv", report_delimited(blast, ','));
    QStandardItemModel model;
    model.setHorizontalHeaderLabels({"Name", "Value"});
    model.appendRow({new QStandardItem("sample"), new QStandardItem("3")});
    const auto copied = parse_text_table(copy_model_table(&model));
    need(copied.size() == 2 && copied[0] == QStringList{"Name", "Value"} &&
             copied[1] == QStringList{"sample", "3"},
         "Copy only model cells and horizontal headers without row indices or "
         "settings");
    bool stopped = false;
    try {
      report_xlsx(matrix, [] { return true; });
    } catch (const std::exception &) {
      stopped = true;
    }
    need(stopped, "Workbook supports cooperative cancellation");
    std::cout << "PASS table formats, workbook blocks, literal names, numeric "
                 "schemas, metadata and hyperlinks\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
  return 0;
}
