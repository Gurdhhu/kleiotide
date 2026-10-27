#include "gui/main_window.hpp"
#include "gui/app_settings.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "core/session.hpp"
#include "core/translation.hpp"
#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QUrl>
#include <algorithm>
#include <sstream>
namespace kleiotide {
namespace {
constexpr auto sequencesMime="application/x-kleiotide-sequences";
constexpr auto residuesMime="application/x-kleiotide-residues";
constexpr qsizetype clipboardLimit=256*1024*1024;
}
void MainWindow::copy_selected(bool fasta){
    if(busy_)return;
    const auto ids=view_->selected_ids();if(ids.empty()){statusBar()->showMessage("Select sequence names first.",5000);return;}
    try{std::ostringstream out;for(auto id:ids){const auto& row=document_->sequence(id);if(fasta)write_fasta_record(out,row.name,row.symbols.text());else out<<row.name<<'\n';}
        if(!fasta){QApplication::clipboard()->setText(QString::fromUtf8(out.str().c_str()));return;}
        auto part=document_->subset(ids);std::ostringstream binary(std::ios::binary);write_session(binary,part);auto bytes=binary.str();if(bytes.size()>static_cast<std::size_t>(clipboardLimit))throw std::runtime_error("Sequence clipboard exceeds 256 MiB. Select fewer sequences.");
        auto* mime=new QMimeData;mime->setText(QString::fromUtf8(out.str().c_str()));mime->setData(sequencesMime,QByteArray::fromStdString(bytes));QApplication::clipboard()->setMimeData(mime);
        statusBar()->showMessage(QString("Copied %1 sequences with types, groups and trace links").arg(ids.size()),5000);
    }catch(const std::exception& e){QMessageBox::warning(this,"Copy sequences",QString::fromUtf8(e.what()));}
}
void MainWindow::cut_selected(){
    if(busy_)return;
    const auto ids=view_->selected_ids();
    if(!ids.empty()){
        // Build and publish a complete validated payload before deleting rows.
        try{auto part=document_->subset(ids);std::ostringstream plain,binary(std::ios::binary);write_fasta(plain,part);write_session(binary,part);const auto bytes=binary.str();if(bytes.size()>static_cast<std::size_t>(clipboardLimit))throw std::runtime_error("Sequence clipboard exceeds 256 MiB. Select fewer sequences.");
            auto* mime=new QMimeData;mime->setText(QString::fromUtf8(plain.str().c_str()));mime->setData(sequencesMime,QByteArray::fromStdString(bytes));QApplication::clipboard()->setMimeData(mime);
            document_->erase_rows(ids);view_->refresh();update_state();
        }catch(const std::exception& e){QMessageBox::warning(this,"Cut sequences",QString::fromUtf8(e.what()));}return;
    }
}
void MainWindow::cut_clipboard(){copy_fragment_clipboard(true);}
void MainWindow::copy_fragment_clipboard(bool cut){
    if(busy_)return;
    if(cut&&view_->mode()!=AlignmentView::Mode::Insert&&view_->mode()!=AlignmentView::Mode::Rewrite)return;
    const auto rows=view_->selected_residue_rows();if(!rows)return;
    try{const auto& fragment=*view_->fragment();const auto& row=document_->sequence(fragment.ids.front());
        if(cut&&!complete_protein_selection(row,fragment.start,fragment.width))throw std::invalid_argument("Expand selection to complete amino acids before cutting.");
        QJsonArray values;QStringList plain;for(const auto& text:*rows){const auto value=QString::fromLatin1(text.data(),static_cast<qsizetype>(text.size()));values.append(value);plain.append(value);}
        QJsonObject payload{{"version",2},{"rows",values}};
        const auto residueBytes=QJsonDocument(payload).toJson(QJsonDocument::Compact);if(residueBytes.size()>clipboardLimit)throw std::runtime_error("Residue clipboard exceeds 256 MiB.");
        auto* mime=new QMimeData;mime->setText(plain.join('\n'));mime->setData(residuesMime,residueBytes);QApplication::clipboard()->setMimeData(mime);if(cut)view_->cut_residues();
    }catch(const std::exception& e){QMessageBox::warning(this,cut?"Cut fragment":"Copy fragment",QString::fromUtf8(e.what()));}
}
void MainWindow::paste_fragment(){
    if(busy_)return;
    // Fragment shortcuts never import files or whole sequences, regardless of name selection.
    auto* canvas=traces_->canvas();const auto* focus=QApplication::focusWidget();
    if((focus==canvas||focus==canvas->viewport())&&!prepare_trace_edit())return;
    if(view_->mode()!=AlignmentView::Mode::Insert&&view_->mode()!=AlignmentView::Mode::Rewrite)return;
    const auto& fragment=view_->fragment();
    if((fragment&&(fragment->ids.size()!=1||(view_->mode()==AlignmentView::Mode::Insert&&!view_->insertable_selection())))||!view_->caret())return;
    const auto* mime=QApplication::clipboard()->mimeData();
    try{
        if(mime->hasFormat(residuesMime)){
            const auto residueBytes=mime->data(residuesMime);if(residueBytes.size()>clipboardLimit)throw std::runtime_error("Residue clipboard exceeds 256 MiB.");
            QJsonParseError error;const auto object=QJsonDocument::fromJson(residueBytes,&error).object();
            if(error.error!=QJsonParseError::NoError||(object.value("version").toInt()!=1&&object.value("version").toInt()!=2)||!object.value("rows").isArray())throw std::runtime_error("Invalid Kleiotide residue clipboard.");
            const auto array=object.value("rows").toArray();if(array.size()!=1){statusBar()->showMessage("Multi-row residue clipboard data cannot be pasted.",5000);return;}
            if(!array[0].isString())throw std::runtime_error("Invalid residue clipboard row.");
            auto text=array[0].toString();
            if(!view_->paste_residue_rows({text.toUtf8().toStdString()}))statusBar()->showMessage("Clipboard symbols are not valid for the selected row.",5000);
            return;
        }
        if(mime->hasUrls()||mime->hasFormat(sequencesMime)||mime->text().trimmed().startsWith('>')){statusBar()->showMessage("Use Paste sequences for whole sequences or FASTA data.",5000);return;}
        if(!view_->paste_residues(mime->text()))statusBar()->showMessage("Clipboard symbols are not valid for the selected row.",5000);
    }catch(const std::exception& e){QMessageBox::warning(this,"Paste fragment",QString::fromUtf8(e.what()));}
}
void MainWindow::paste(bool newDocument,bool atCaret){
    if(busy_)return;
    const auto* mime=QApplication::clipboard()->mimeData();
    if(mime->hasUrls()){QStringList paths;for(const auto& url:mime->urls()){if(!url.isLocalFile()){QMessageBox::warning(this,"Paste","Only local copied files are supported.");return;}paths.push_back(url.toLocalFile());}if(newDocument&&!maybe_save([this,paths]{load_files(paths,false);}))return;load_files(paths,!newDocument);return;}
    try{
        std::shared_ptr<Document> incoming;std::vector<Record> records;
        if(mime->hasFormat(sequencesMime)){const auto bytes=mime->data(sequencesMime);if(bytes.size()>clipboardLimit)throw std::runtime_error("Sequence clipboard exceeds 256 MiB.");std::istringstream input(std::string(bytes.constData(),static_cast<std::size_t>(bytes.size())),std::ios::binary);incoming=std::make_shared<Document>(read_session(input).document);}
        else{std::istringstream input(mime->text().toUtf8().toStdString());records=read_fasta(input,{},fastaImport_);}
        if(newDocument){if(!incoming){incoming=std::make_shared<Document>();incoming->load(std::move(records),AppSettings::instance().values().translation());}install_document(std::move(incoming),{},false);return;}
        auto position=document_->rows().size();const auto selected=view_->selected_ids();if(atCaret&&!selected.empty()){position=0;for(std::size_t i=0;i<document_->rows().size();++i)if(std::find(selected.begin(),selected.end(),document_->rows()[i].id)!=selected.end())position=i+1;}
        const auto count=incoming?incoming->rows().size():records.size();if(incoming)document_->insert_document(*incoming,position);else document_->insert_records(std::move(records),position);
        view_->refresh();if(atCaret){std::vector<SequenceId> ids;for(std::size_t i=0;i<count;++i)ids.push_back(document_->rows()[position+i].id);view_->select_ids(ids);}update_state();
    }catch(const std::exception& e){QMessageBox::warning(this,"Paste",QString::fromUtf8(e.what()));}
}
}
