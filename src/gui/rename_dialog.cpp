#include "gui/rename_dialog.hpp"
#include "gui/table_io.hpp"
#include <QCheckBox>
#include <QSignalBlocker>
#include <QAbstractTableModel>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <limits>
namespace kleiotide {
QString rename_operation_label(RenameOperation op){
    const char* labels[]={"Spaces to underscores","Underscores to spaces","Prepend text…","Append text…","Replace text…","Append filename…","Reverse names","Strip numeric suffixes…","Make names unique…","Names from row numbers…","Names from aligned lengths…","Names from true lengths…","Append taxonomy / text from table…","Sanitize names","Append group name…","Prepend group name…","Using old name / new name table…"};
    return QString::fromUtf8(labels[static_cast<int>(op)]);
}
namespace {
class PreviewModel final:public QAbstractTableModel {
public:
    using QAbstractTableModel::QAbstractTableModel;RenamePlan plan;
    void set(RenamePlan value){beginResetModel();plan=std::move(value);endResetModel();}
    int rowCount(const QModelIndex& parent={})const override{return parent.isValid()?0:static_cast<int>(std::min<std::size_t>(plan.rows.size(),std::numeric_limits<int>::max()));}
    int columnCount(const QModelIndex& parent={})const override{return parent.isValid()?0:2;}
    QVariant headerData(int section,Qt::Orientation orientation,int role)const override{if(role!=Qt::DisplayRole)return {};if(orientation==Qt::Horizontal)return section==0?"Current name":"New name";return section+1;}
    QVariant data(const QModelIndex& index,int role)const override{if(!index.isValid())return {};const auto& row=plan.rows[static_cast<std::size_t>(index.row())];
        if(role==Qt::DisplayRole)return index.column()==0?row.before:row.after;
        if(role==Qt::ToolTipRole)return row.duplicate?QString("This name appears more than once in the resulting alignment."):index.column()==0?row.before:row.after;
        if(role==Qt::BackgroundRole&&index.column()==1&&row.duplicate)return QColor(255,244,218);
        if(role==Qt::ForegroundRole&&row.before==row.after)return QColor(120,120,120);
        return {};}
};
}
std::optional<std::vector<std::pair<SequenceId,std::string>>> show_batch_rename(const Document& document,const std::vector<SequenceId>& selected,const QString& filename,RenameOperation initial,QWidget* parent){
    QDialog dialog(parent);dialog.setObjectName("batchRenameDialog");dialog.setWindowTitle("Batch rename — preview before applying");QVBoxLayout layout(&dialog);QFormLayout fields;
    QComboBox operation,scope;operation.setObjectName("renameOperation");scope.setObjectName("renameScope");
    for(int i=0;i<=static_cast<int>(RenameOperation::Table);++i)operation.addItem(rename_operation_label(static_cast<RenameOperation>(i)));
    if(!selected.empty())scope.addItem(QString("Selected sequences (%1)").arg(static_cast<qulonglong>(selected.size())),true);
    scope.addItem(QString("All sequences (%1)").arg(static_cast<qulonglong>(document.rows().size())),false);
    if(initial==RenameOperation::Sanitize)scope.setCurrentIndex(scope.count()-1);
    fields.addRow("Operation",&operation);fields.addRow("Apply to",&scope);
    QLineEdit text,replacement,separator;text.setObjectName("renameText");replacement.setObjectName("renameReplacement");separator.setObjectName("renameSeparator");
    QLabel textLabel,replacementLabel("Replace with"),separatorLabel("Separator");fields.addRow(&textLabel,&text);fields.addRow(&replacementLabel,&replacement);fields.addRow(&separatorLabel,&separator);
    QPlainTextEdit table;table.setObjectName("renameTable");table.setTabChangesFocus(false);table.setMaximumHeight(150);QLabel tableLabel("Table");fields.addRow(&tableLabel,&table);layout.addLayout(&fields);
    QWidget tableControls;QFormLayout tableFields(&tableControls);tableFields.setContentsMargins(0,0,0,0);QPushButton load("Load table…");load.setObjectName("renameLoadTable");load.setToolTip("Load a TSV, CSV or XLSX table, then choose its columns.");QCheckBox header("First row is a header");header.setObjectName("renameTableHeader");QComboBox oldColumn,newColumn;oldColumn.setObjectName("renameOldColumn");newColumn.setObjectName("renameNewColumn");tableFields.addRow(&load);tableFields.addRow(&header);tableFields.addRow("Sequence name column",&oldColumn);tableFields.addRow("New name / appended text column",&newColumn);layout.addWidget(&tableControls);
    auto updateColumns=[&]{QSignalBlocker a(oldColumn),b(newColumn);const auto old=oldColumn.currentIndex(),next=newColumn.currentIndex();oldColumn.clear();newColumn.clear();TextTable parsed;try{parsed=parse_text_table(table.toPlainText());}catch(...){}int count=2;for(const auto& row:parsed)count=std::max(count,int(row.size()));for(int i=0;i<count;++i){QString label=QString("Column %1").arg(i+1);if(header.isChecked()&&!parsed.empty()&&i<parsed[0].size())label+=" — "+parsed[0][i].left(80);oldColumn.addItem(label);newColumn.addItem(label);}oldColumn.setCurrentIndex(count==2||old<0||old>=count?0:old);newColumn.setCurrentIndex(count==2||next<0||next>=count?1:next);oldColumn.setEnabled(count>2);newColumn.setEnabled(count>2);};updateColumns();
    QObject::connect(&load,&QPushButton::clicked,&dialog,[&]{if(auto content=load_table_file(&dialog,"Load rename table"))table.setPlainText(*content);});

    QLabel status("Click Preview to review the names. No changes are made until Apply.");status.setObjectName("renameStatus");status.setWordWrap(true);status.setTextInteractionFlags(Qt::TextSelectableByMouse);layout.addWidget(&status);
    QTableView preview;preview.setObjectName("renamePreview");PreviewModel model;preview.setModel(&model);preview.setEditTriggers(QAbstractItemView::NoEditTriggers);preview.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);preview.setWordWrap(false);layout.addWidget(&preview,1);
    QDialogButtonBox buttons;auto* build=buttons.addButton("Preview",QDialogButtonBox::ActionRole);auto* apply=buttons.addButton("Apply",QDialogButtonBox::AcceptRole);buttons.addButton(QDialogButtonBox::Cancel);build->setObjectName("renamePreviewButton");apply->setObjectName("renameApplyButton");apply->setEnabled(false);build->setDefault(true);layout.addWidget(&buttons);
    auto invalidate=[&]{apply->setEnabled(false);model.set({});status.setText(static_cast<RenameOperation>(operation.currentIndex())==RenameOperation::Sanitize?
        "Keep ASCII letters, digits and underscores; replace unsupported runs with underscores. Names are made unique ignoring case, with _2, _3, … suffixes. Preview before Apply.":
        static_cast<RenameOperation>(operation.currentIndex())==RenameOperation::AppendGroup||static_cast<RenameOperation>(operation.currentIndex())==RenameOperation::PrependGroup?
        "Add each sequence's group name using the separator below. Ungrouped sequences stay unchanged. Preview before Apply.":
        "Click Preview to review the names. No changes are made until Apply.");};
    auto configure=[&]{const auto op=static_cast<RenameOperation>(operation.currentIndex());const bool hasText=op==RenameOperation::Prepend||op==RenameOperation::Append||op==RenameOperation::Replace||op==RenameOperation::Filename||op==RenameOperation::RowNumber||op==RenameOperation::Length||op==RenameOperation::TrueLength;
        fields.setRowVisible(&text,hasText);fields.setRowVisible(&replacement,op==RenameOperation::Replace);fields.setRowVisible(&separator,op==RenameOperation::Filename||op==RenameOperation::StripSuffix||op==RenameOperation::Unique||op==RenameOperation::AppendTable||op==RenameOperation::AppendGroup||op==RenameOperation::PrependGroup);fields.setRowVisible(&table,op==RenameOperation::Table||op==RenameOperation::AppendTable);tableControls.setVisible(op==RenameOperation::Table||op==RenameOperation::AppendTable);
        textLabel.setText(op==RenameOperation::Replace?"Find (literal, case-sensitive)":op==RenameOperation::Filename?"Filename text":op==RenameOperation::RowNumber||op==RenameOperation::Length||op==RenameOperation::TrueLength?"Optional prefix":"Text (spaces are literal)");
        separator.setText(op==RenameOperation::StripSuffix||op==RenameOperation::Unique||op==RenameOperation::AppendGroup||op==RenameOperation::PrependGroup?"_":" ");if(op==RenameOperation::Filename&&text.text().isEmpty())text.setText(filename);
        table.setPlaceholderText(op==RenameOperation::AppendTable?"old name<TAB>taxonomy or appended text\nOne exact complete title per line.":"old name<TAB>new name\nOne exact complete title per line.");invalidate();};
    QObject::connect(&operation,&QComboBox::currentIndexChanged,&dialog,[&]{configure();});QObject::connect(&scope,&QComboBox::currentIndexChanged,&dialog,[&]{invalidate();});
    for(auto* input:{&text,&replacement,&separator})QObject::connect(input,&QLineEdit::textChanged,&dialog,[&]{invalidate();});
    QObject::connect(&table,&QPlainTextEdit::textChanged,&dialog,[&]{updateColumns();invalidate();});QObject::connect(&header,&QCheckBox::toggled,&dialog,[&]{updateColumns();invalidate();});for(auto* c:{&oldColumn,&newColumn})QObject::connect(c,&QComboBox::currentIndexChanged,&dialog,[&]{invalidate();});
    QObject::connect(build,&QPushButton::clicked,&dialog,[&]{try{QString mapped=table.toPlainText();const auto op=static_cast<RenameOperation>(operation.currentIndex());if(op==RenameOperation::Table||op==RenameOperation::AppendTable){if(oldColumn.currentIndex()==newColumn.currentIndex())throw std::invalid_argument("Choose two different columns.");auto rows=parse_text_table(mapped);TextTable selectedRows;for(std::size_t i=header.isChecked()?1:0;i<rows.size();++i){const auto& cells=rows[i];if(oldColumn.currentIndex()>=cells.size()||newColumn.currentIndex()>=cells.size())throw std::invalid_argument("A selected column is missing from a table row.");selectedRows.push_back({cells[oldColumn.currentIndex()],cells[newColumn.currentIndex()]});}mapped=table_text(selectedRows);}
        const RenameOptions options{static_cast<RenameOperation>(operation.currentIndex()),text.text(),replacement.text(),separator.text(),mapped};auto plan=make_rename_plan(document,options,scope.currentData().toBool()?selected:std::vector<SequenceId>{});
        QString report=QString("Names to change: %1 · Unchanged: %2 · Rows with duplicate names: %3").arg(static_cast<qulonglong>(plan.changed)).arg(static_cast<qulonglong>(plan.rows.size()-plan.changed)).arg(static_cast<qulonglong>(plan.duplicateRows));
        if(!plan.unmatched.isEmpty())report+=QString("\nUnmatched table entries (%1): %2").arg(plan.unmatched.size()).arg(plan.unmatched.mid(0,8).join(", "))+(plan.unmatched.size()>8?"…":"");
        if(plan.multipleMatches)report+=QString("\nTable entries matching multiple rows: %1. Every matching row will receive the mapping.").arg(static_cast<qulonglong>(plan.multipleMatches));
        status.setText(report);layout.invalidate();layout.activate();apply->setEnabled(plan.changed!=0);model.set(std::move(plan));
    }catch(const std::exception& e){model.set({});apply->setEnabled(false);status.setText(QString::fromUtf8(e.what()));}});
    QObject::connect(apply,&QPushButton::clicked,&dialog,[&]{if(!apply->isEnabled())return;if(!rename_plan_current(document,model.plan)){invalidate();status.setText("The alignment changed. Preview again before applying.");return;}dialog.accept();});QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    operation.setCurrentIndex(static_cast<int>(initial));configure();dialog.resize(960,660);
    if(dialog.exec()!=QDialog::Accepted||!apply->isEnabled()||!rename_plan_current(document,model.plan))return {};
    return model.plan.patches();
}
}
