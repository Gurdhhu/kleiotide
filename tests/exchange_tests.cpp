#include "gui/blast_data.hpp"
#include "gui/local_blast.hpp"
#include "gui/rename_rules.hpp"
#include "gui/spart.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;
void need(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
template <class F> void rejects(F f) {
  bool caught = false;
  try {
    f();
  } catch (...) {
    caught = true;
  }
  need(caught, "Invalid input accepted");
}
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  try {
    const auto table =
        parse_text_table("name,other,group\r\n\"(sample_1)\",\"with, "
                         "comma\",\"Red \"\"group\"\"\"\r\nsample_2,x,\r\n",
                         ',');
    need(table.size() == 3 && table[1][1] == "with, comma" &&
             table[1][2] == "Red \"group\"" && table[2][2].isEmpty(),
         "Quoted CSV, empty trailing field, CRLF");
    need(parse_text_table(table_text(table)) == table,
         "Quote-safe TSV roundtrip");
    rejects([] { parse_text_table("a,\"unfinished", ','); });
    rejects([] { parse_text_table("\"a\nx\",b", ','); });
    Document d;
    d.load({{"sample_1", "ACGT"}, {"sample_2", "ACGT"}, {"sample_3", "ACGT"}});
    d.apply_groups({{1, "Taxon α", 0x0099bb}, {2, "=literal", 0xcc8800}},
                   {{1, 1}, {2, 2}, {3, 0}});
    for (bool xml : {false, true}) {
      auto bytes = export_spart(d, xml, "project", "partition A");
      auto partitions = parse_spart(bytes);
      need(partitions.size() == 1 && partitions[0].name == "partition A" &&
               partitions[0].assignments == TextTable{{"sample_1", "Taxon α"},
                                                      {"sample_2", "=literal"},
                                                      {"sample_3", ""}},
           "SPART grouped/ungrouped Unicode name roundtrip");
    }
    auto multi =
        QByteArray("begin "
                   "spart;\rProject_name=p;Date=2026;N_spartitions=2:One,0.9/"
                   "Two;N_individuals=2/1;N_subsets=2/1;[comment\rsecond "
                   "line]\rIndividual_assignment=\rsample_1:0/7\rsample_2:2/"
                   "?;\rOptional_thing=ignored;end;");
    auto partitions = parse_spart(multi);
    auto padded = multi;
    padded.replace("sample_1:0/7", "sample_1:000/007");
    need(parse_spart(padded)[0].assignments[0][1] == "0",
         "SPART integer subset labels accept leading zeros");
    need(partitions.size() == 2 && partitions[0].assignments[0][1] == "0" &&
             partitions[1].assignments[1][1].isEmpty(),
         "Multiple partitions, zero subset, scores, comments, CR newlines, "
         "unknown optional command");
    rejects([&] {
      auto bad = multi;
      bad.replace("N_individuals=2/1", "N_individuals=2/2");
      parse_spart(bad);
    });
    rejects([&] {
      auto bad = multi;
      bad.replace("sample_2:2/?", "sample_1:2/?");
      parse_spart(bad);
    });
    d.rename({{1, "invalid name"}});
    rejects([&] { export_spart(d, false, "p", "part"); });
    d.rename({{1, "sample_1"}});
    RenameOptions rename;
    rename.operation = RenameOperation::Table;
    rename.table = "\"(sample_1)\"\t\"New name\"\n";
    need(make_rename_plan(d, rename).rows[0].after == "New name",
         "Rename shares quote and parenthesis rules");
    const auto independent =
        QFileInfo(QString::fromUtf8(__FILE__))
            .dir()
            .filePath("../examples/table_import_demo.xlsx");
    auto shared = read_xlsx_table(independent, 1);
    need(shared.size() == 3 && shared[1][1] == "(alpha)" &&
             shared[1][2] == "renamed alpha",
         "Independent DEFLATE XLSX shared strings and second worksheet");
    QTemporaryDir temp;
    auto path = temp.filePath("hyperlinks.xlsx");
    BlastTable hits{{"1", "=literal", "1", "99.2",  "ABC123.1", "Species",
                     "",  "100",      "1", "1e-20", "100",      "0",
                     "1", "100",      "3", "102",   "99",       "Title",
                     "",  "",         ""}};
    auto bytes = blast_xlsx(hits);
    QFile file(path);
    need(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(),
         "Write XLSX fixture");
    file.close();
    auto excel = read_xlsx_table(path);
    need(excel.size() == 2 && excel[1][1] == "=literal" &&
             excel[1][4] == "ABC123.1",
         "XLSX literal cell import");
    need(bytes.contains("hyperlink") && bytes.contains("nuccore/ABC123.1"),
         "XLSX real accession hyperlink");
    for (const auto &p :
         QStringList{"blastn", "blastp", "blastx", "tblastn", "tblastx"}) {
      BlastOptions o;
      o.local = true;
      o.program = p;
      o.launcher = p;
      o.database = "path with spaces/db";
      o.word = 0;
      auto args = blast_arguments(o, "query file.fa");
      need(!args.contains("-remote") && !args.contains("-entrez_query") &&
               args.contains("path with spaces/db") &&
               args.contains("query file.fa"),
           "Local arguments preserve paths and never contact remote server");
      need(args.contains("-task") == bool(p == "blastn" || p == "blastp"),
           "Only blastn/blastp accept task");
      need(args.contains("-comp_based_stats") ==
               bool(p != "blastn" && p != "tblastx"),
           "Translated composition arguments");
      need(args.contains("-query_gencode") ==
               bool(p == "blastx" || p == "tblastx"),
           "Query genetic code");
      need(args.contains("-db_gencode") ==
               bool(p == "tblastn" || p == "tblastx"),
           "Database genetic code");
    }
    if (argc > 1) {
      QFile output(QString::fromLocal8Bit(argv[1]));
      need(output.open(QIODevice::WriteOnly) &&
               output.write(bytes) == bytes.size(),
           "Save independent XLSX fixture");
    }
    std::cout << "PASS CSV/TSV decoding, shared rename rules, SPART matrix/XML "
                 "roundtrips, partitions/count validation, XLSX import and "
                 "hyperlinks, local arguments for all five BLAST programs\n";
  } catch (const std::exception &e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
  }
}
