#pragma once
#include "core/alignment.hpp"
#include <QString>
#include <QFileDialog>
#include <functional>
#include <ostream>
class QIODevice;
class QWidget;
namespace kleiotide {
enum class OutputFormat { Session, Fasta, Abif, Table };
enum class OutputNameRules { Platform, Windows };
QString output_filename_error(const QString& path,OutputNameRules rules=OutputNameRules::Platform);
// Validate before QFileDialog can interpret ?/* as a directory listing filter.
QString choose_output_filename(QWidget*,const QString& caption,const QString& directory,
    const QString& filters,QString* selectedFilter=nullptr,QFileDialog::Options options={},OutputNameRules rules=OutputNameRules::Platform);
bool requires_session(const Document&);
QString output_path(QString path, OutputFormat);
QString output_path_error(const QString& path, OutputFormat, const Document* source = nullptr);
// Resolve the final filename before checking protection or confirming replacement.
QString approve_output_path(QWidget*, QString path, OutputFormat,
                            const Document* source = nullptr, bool confirmReplacement = true);
// Shared chooser for exports: validate the final path and confirm replacement once.
QString choose_export_path(QWidget*,const QString& caption,const QString& initial,
                           const QString& filters,OutputFormat,const Document* source=nullptr);
using OutputWriter = std::function<void(std::ostream&)>;
QString write_export_atomic(const QString&,OutputFormat,const Document*,const OutputWriter&,
                            const std::function<bool()>& cancelled = {});
void write_output(QIODevice&, const OutputWriter&);
QString write_output_atomic(const QString& path, const OutputWriter&,
                            const std::function<bool()>& cancelled = {});
}
