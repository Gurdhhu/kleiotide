#include "gui/file_output.hpp"
#include "core/session.hpp"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QSaveFile>
#include <QRegularExpression>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <streambuf>
namespace kleiotide {
namespace {
std::filesystem::path native_path(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toUtf8().toStdString());
#endif
}
class DeviceBuffer final : public std::streambuf {
public:
    explicit DeviceBuffer(QIODevice& device) : device_(device) {}
protected:
    std::streamsize xsputn(const char* data, std::streamsize count) override {
        return static_cast<std::streamsize>(device_.write(data, static_cast<qint64>(count)));
    }
    int_type overflow(int_type value) override {
        if (traits_type::eq_int_type(value, traits_type::eof())) return traits_type::not_eof(value);
        const char c = traits_type::to_char_type(value);
        return device_.write(&c, 1) == 1 ? value : traits_type::eof();
    }
private:
    QIODevice& device_;
};
class OutputFileDialog final:public QFileDialog {
public:
    using QFileDialog::QFileDialog;
protected:
    void accept()override{
        if(const auto paths=selectedFiles();!paths.isEmpty()){
            const auto error=output_filename_error(paths.front(),OutputNameRules::Windows);
            if(!error.isEmpty()){QMessageBox::warning(this,"Invalid output filename",error);return;}
        }
        QFileDialog::accept();
    }
};
}
QString output_filename_error(const QString& path,OutputNameRules rules){
#ifdef _WIN32
    rules=OutputNameRules::Windows;
#endif
    if(rules!=OutputNameRules::Windows)return {};
    const auto name=QFileInfo(path).fileName();
    if(name.isEmpty()||name=="."||name=="..")return "Enter a filename.";
    if(name.contains(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1f]"))))
        return "Windows filenames cannot contain < > : \" / \\ | ? * or control characters. Enter a filename without wildcards.";
    if(name.endsWith('.')||name.endsWith(' '))return "Windows filenames cannot end with a dot or a space.";
    const auto stem=name.section('.',0,0).trimmed().toUpper();
    if(stem=="CON"||stem=="PRN"||stem=="AUX"||stem=="NUL"||stem=="CONIN$"||stem=="CONOUT$"||
       QRegularExpression(QStringLiteral("^(COM|LPT)[1-9¹²³]$")).match(stem).hasMatch())
        return "This filename is reserved by Windows. Choose another name.";
    return {};
}
QString choose_output_filename(QWidget* parent,const QString& caption,const QString& directory,
                              const QString& filters,QString* selectedFilter,QFileDialog::Options options,OutputNameRules rules){
#ifdef _WIN32
    rules=OutputNameRules::Windows;
#endif
    if(rules!=OutputNameRules::Windows)return QFileDialog::getSaveFileName(parent,caption,directory,filters,selectedFilter,options);
    OutputFileDialog dialog(parent,caption,directory,filters);
    dialog.setOptions(options|QFileDialog::DontUseNativeDialog);
    dialog.setAcceptMode(QFileDialog::AcceptSave);dialog.setFileMode(QFileDialog::AnyFile);
    if(selectedFilter&&!selectedFilter->isEmpty())dialog.selectNameFilter(*selectedFilter);
    if(dialog.exec()!=QDialog::Accepted)return {};
    if(selectedFilter)*selectedFilter=dialog.selectedNameFilter();
    return dialog.selectedFiles().value(0);
}
bool requires_session(const Document& document) {
    if (!document.groups().empty() || !(document.translation_settings() == TranslationSettings{})) return true;
    return std::any_of(document.rows().begin(), document.rows().end(), [](const auto& row) {
        return row.trace || row.kind == RowKind::Protein ||
            (row.kind == RowKind::Mask && inferred_row_kind(row.symbols.text()) != RowKind::Mask);
    });
}
QString output_path(QString path, OutputFormat format) {
    const auto suffix = QFileInfo(path).suffix().toLower();
    if (format == OutputFormat::Session && suffix != "kleio") path += ".kleio";
    if (format == OutputFormat::Abif && suffix != "ab1" && suffix != "abi") path += ".ab1";
    if (format == OutputFormat::Fasta) {
        const QStringList fastaSuffixes={"fasta","fas","fa","fna","ffn","faa","frn","fsa","mpfa","fnt","fsa_nt","fsa_aa"};
        if(!fastaSuffixes.contains(suffix))path+=".fasta";
    }
    return path;
}
QString output_path_error(const QString& path, OutputFormat format, const Document* source) {
    if(const auto error=output_filename_error(path);!error.isEmpty())return error;
    const QFileInfo info(path);
    if (info.isDir()) return "The output filename refers to a directory.";
    if (info.isSymLink() && !info.exists()) return "Choose an output filename without a broken symbolic link.";
    if (format != OutputFormat::Abif && (info.suffix().compare("ab1", Qt::CaseInsensitive) == 0 ||
                                         info.suffix().compare("abi", Qt::CaseInsensitive) == 0))
        return "A sequence export or session cannot overwrite a chromatogram.";
    if (format != OutputFormat::Session && info.suffix().compare("kleio", Qt::CaseInsensitive) == 0)
        return "A sequence or trace export cannot overwrite a saved session.";
    if (!info.exists()) return {};
    if (source) for (const auto& row : source->rows()) if (row.trace && !row.trace->original->sourcePath.empty()) {
        std::error_code error;
        const auto original = QString::fromUtf8(row.trace->original->sourcePath.c_str());
        if (std::filesystem::equivalent(native_path(original), native_path(path), error) && !error)
            return "Choose a different filename. An imported source chromatogram cannot be overwritten by export.";
    }
    QFile existing(path);
    if (!existing.open(QIODevice::ReadOnly)) return "Cannot inspect the existing output file: " + existing.errorString();
    const auto magic = existing.read(8);
    if (existing.error() != QFileDevice::NoError) return "Cannot inspect the existing output file: " + existing.errorString();
    if (format != OutputFormat::Abif && magic.startsWith("ABIF"))
        return "A sequence export or session cannot overwrite a chromatogram.";
    if (format != OutputFormat::Session && magic == QByteArray(session_magic.data(), static_cast<qsizetype>(session_magic.size())))
        return "A sequence or trace export cannot overwrite a saved session.";
    return {};
}
QString approve_output_path(QWidget* parent, QString path, OutputFormat format,
                            const Document* source, bool confirmReplacement) {
    if (path.isEmpty()) return {};
    const auto typedSuffix=QFileInfo(path).suffix().toLower();
    if((format!=OutputFormat::Session&&typedSuffix=="kleio")||
       (format!=OutputFormat::Abif&&(typedSuffix=="ab1"||typedSuffix=="abi"))){
        QMessageBox::warning(parent,"Choose another filename","Choose a different filename. This output cannot replace a saved session or chromatogram.");return {};
    }
    path = output_path(std::move(path), format);
    if (const auto error = output_path_error(path, format, source); !error.isEmpty()) {
        QMessageBox::warning(parent, "Choose another filename", error);
        return {};
    }
    if (confirmReplacement && QFileInfo::exists(path)) {
        const auto answer = QMessageBox::question(parent, "Replace existing file?",
            "Replace this existing file?\n" + QDir::toNativeSeparators(path),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) return {};
    }
    return path;
}
QString choose_export_path(QWidget* parent,const QString& caption,const QString& initial,
                           const QString& filters,OutputFormat format,const Document* source){
    const auto path=choose_output_filename(parent,caption,initial,filters,nullptr,QFileDialog::DontConfirmOverwrite);
    return approve_output_path(parent,path,format,source);
}
QString write_export_atomic(const QString& path,OutputFormat format,const Document* source,
                            const OutputWriter& writer,const std::function<bool()>& cancelled){
    if(const auto error=output_path_error(path,format,source);!error.isEmpty())return error;
    return write_output_atomic(path,writer,cancelled);
}
void write_output(QIODevice& device, const OutputWriter& writer) {
    DeviceBuffer buffer(device);
    std::ostream output(&buffer);
    writer(output);
    output.flush();
    if (!output) throw std::runtime_error("Could not write the complete output file.");
}
QString write_output_atomic(const QString& path, const OutputWriter& writer,
                            const std::function<bool()>& cancelled) {
    try {
        QSaveFile file(path);
        // Never fall back to truncating the destination if a temporary file cannot be created.
        file.setDirectWriteFallback(false);
        if (!file.open(QIODevice::WriteOnly)) return file.errorString();
        write_output(file, writer);
        if (cancelled && cancelled()) return "Export cancelled.";
        if (!file.commit()) return file.errorString();
        return {};
    } catch (const std::exception& error) {
        return QString::fromUtf8(error.what());
    }
}
}
