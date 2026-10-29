#include "gui/trace_link_dialog.hpp"
#include "core/abif.hpp"
#include "core/trace_linking.hpp"
#include <QAbstractTableModel>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QScreen>
#include <QMessageBox>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QStyleOptionButton>
#include <QApplication>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <atomic>
#include <filesystem>
#include <fstream>
namespace kleiotide {namespace {
struct File {QString path,note;std::shared_ptr<const Chromatogram> original;};
class Model final:public QAbstractTableModel{
public:
 std::shared_ptr<Document> source;std::vector<SequenceId> ids;std::vector<File> files;std::vector<int> assignments;std::vector<TraceMapping> results;std::function<void()> invalidate;
 int rowCount(const QModelIndex& p={})const override{return p.isValid()?0:int(ids.size());}int columnCount(const QModelIndex& p={})const override{return p.isValid()?0:5;}
 QVariant headerData(int c,Qt::Orientation o,int role)const override{if(o==Qt::Horizontal&&role==Qt::DisplayRole)return QStringList{"Sequence","AB1 file","Orientation","Peak mapping","Add AB1…"}.value(c);return {};}
 QVariant data(const QModelIndex& i,int role)const override{const auto row=std::size_t(i.row());if(!i.isValid()||row>=ids.size())return {};
  if(role==Qt::BackgroundRole&&row<results.size()&&i.column()>=2)return QColor(!results[row].link?"#f9d8dc":results[row].quality=="Terrible"?"#fff0bd":"#d8efdf");
  if(role==Qt::ToolTipRole){if(i.column()==1&&assignments[row]>=0)return files[assignments[row]].path+"\n"+files[assignments[row]].note;if(row<results.size())return QString::fromStdString(results[row].reason);}
  if(role==Qt::EditRole&&i.column()==1)return assignments[row];
  if(role!=Qt::DisplayRole)return {};
  if(i.column()==0)return QString::fromStdString(source->sequence(ids[row]).name);
  if(i.column()==1)return assignments[row]<0?"— Unassigned":QFileInfo(files[assignments[row]].path).fileName();
  if(i.column()==4)return "Add AB1…";
  if(row>=results.size())return "Preview required";
  return QString::fromStdString(i.column()==2?results[row].orientation:results[row].quality);
 }
 Qt::ItemFlags flags(const QModelIndex& i)const override{return QAbstractTableModel::flags(i)|(i.column()==1?Qt::ItemIsEditable:Qt::NoItemFlags);}
 bool setData(const QModelIndex& i,const QVariant& value,int role)override{if(role!=Qt::EditRole||i.column()!=1)return false;const auto file=value.toInt();if(file< -1||file>=int(files.size()))return false;assignments[i.row()]=file;if(invalidate)invalidate();emit dataChanged(index(0,0),index(rowCount()-1,4));return true;}
 void refresh(){beginResetModel();endResetModel();}
};
class Delegate final:public QStyledItemDelegate{public:Model* model;Delegate(Model* m,QObject* p):QStyledItemDelegate(p),model(m){}
 QWidget* createEditor(QWidget* parent,const QStyleOptionViewItem&,const QModelIndex&)const override{auto* c=new QComboBox(parent);c->setEditable(true);c->setInsertPolicy(QComboBox::NoInsert);c->addItem("— Unassigned",-1);for(std::size_t i=0;i<model->files.size();++i)c->addItem(model->files[i].path,int(i));c->completer()->setCompletionMode(QCompleter::PopupCompletion);c->completer()->setFilterMode(Qt::MatchContains);c->completer()->setCaseSensitivity(Qt::CaseInsensitive);return c;}
 void setEditorData(QWidget* w,const QModelIndex& i)const override{auto* c=static_cast<QComboBox*>(w);c->setCurrentIndex(c->findData(i.data(Qt::EditRole)));}
 void setModelData(QWidget* w,QAbstractItemModel* m,const QModelIndex& i)const override{auto* c=static_cast<QComboBox*>(w);const int found=c->findText(c->currentText(),Qt::MatchExactly);if(found>=0)m->setData(i,c->itemData(found),Qt::EditRole);}
};
class AddDelegate final:public QStyledItemDelegate {public:using QStyledItemDelegate::QStyledItemDelegate;
 void paint(QPainter* painter,const QStyleOptionViewItem& option,const QModelIndex&)const override{QStyleOptionButton button;button.rect=option.rect.adjusted(3,3,-3,-3);button.text="Add AB1…";button.state=QStyle::State_Enabled;QApplication::style()->drawControl(QStyle::CE_PushButton,&button,painter);}
};
class Dialog final:public QDialog{public:
 Model model;QTableView* table;QLabel* status;QPushButton *apply,*preview,*add;std::shared_ptr<std::atomic_bool> cancel;bool running=false,closing=false,applied=false;std::uint64_t revision=0;
 Dialog(std::shared_ptr<Document> d,std::vector<SequenceId> ids,QWidget* p):QDialog(p){setObjectName("traceLinkDialog");setWindowTitle("Link trace (AB1)");resize(1000,600);model.source=std::move(d);model.ids=std::move(ids);model.assignments.assign(model.ids.size(),-1);
  auto* l=new QVBoxLayout(this);auto* help=new QLabel("Assign AB1 files (double-click the file cell to choose/search). Add AB1… in a row assigns a new file there. The same trace may serve multiple sequences. Unassigned sequences and unused files are allowed. Preview is required; mapping quality is heuristic, not a Phred score.");help->setWordWrap(true);l->addWidget(help);
  table=new QTableView(this);table->setObjectName("traceAssignments");table->setModel(&model);table->setItemDelegateForColumn(1,new Delegate(&model,table));table->setItemDelegateForColumn(4,new AddDelegate(table));table->setWordWrap(false);table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);const int widths[]={220,260,120,150,120};for(int c=0;c<5;++c)table->setColumnWidth(c,widths[c]);l->addWidget(table,1);
  status=new QLabel("Add AB1 files to begin.");status->setObjectName("traceLinkStatus");status->setWordWrap(true);l->addWidget(status);auto* b=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel);l->addWidget(b);apply=b->button(QDialogButtonBox::Apply);apply->setObjectName("traceLinkApply");apply->setEnabled(false);preview=b->addButton("Preview",QDialogButtonBox::ActionRole);preview->setObjectName("traceLinkPreview");add=b->addButton("Add AB1 files…",QDialogButtonBox::ActionRole);add->setObjectName("traceLinkAdd");b->removeButton(add);l->insertWidget(1,add,0,Qt::AlignLeft);
  if(auto* screen=this->screen()){const auto size=screen->availableGeometry().size();resize(std::min(1000,std::max(360,size.width()-48)),std::min(600,std::max(360,size.height()-64)));}
  model.invalidate=[this]{model.results.clear();apply->setEnabled(false);status->setText("Assignments changed; Preview again.");};
  connect(add,&QPushButton::clicked,this,[this]{add_files(-1);});connect(table,&QTableView::clicked,this,[this](auto i){if(i.column()==4)add_files(i.row());});connect(b,&QDialogButtonBox::rejected,this,&Dialog::reject);
  connect(preview,&QPushButton::clicked,this,[this]{run_preview();});connect(apply,&QPushButton::clicked,this,[this]{if(running||revision!=model.source->state_id()){status->setText("Source changed; Preview again.");return;}try{std::vector<std::pair<SequenceId,std::shared_ptr<const TraceLink>>> links;for(std::size_t i=0;i<model.results.size();++i)if(model.results[i].link)links.push_back({model.ids[i],model.results[i].link});apply_trace_links(*model.source,links);applied=true;accept();}catch(const std::exception& e){status->setText(e.what());}});
 }
 void reject()override{if(running){closing=true;if(cancel)cancel->store(true);status->setText("Cancelling…");return;}QDialog::reject();}
 void busy(bool value){running=value;table->setEnabled(!value);preview->setEnabled(!value);add->setEnabled(!value);apply->setEnabled(false);}
 void add_files(int row){const auto paths=QFileDialog::getOpenFileNames(this,"Add AB1 evidence",{},"Sanger chromatograms (*.ab1 *.abi);;All files (*)");if(paths.isEmpty())return;cancel=std::make_shared<std::atomic_bool>(false);busy(true);auto* watcher=new QFutureWatcher<std::vector<File>>(this);
  connect(watcher,&QFutureWatcher<std::vector<File>>::finished,this,[this,watcher,row]{const auto added=watcher->result();watcher->deleteLater();busy(false);int first=-1;QStringList errors;
   for(const auto& f:added){if(!f.original){errors<<f.path+": "+f.note;continue;}auto found=std::find_if(model.files.begin(),model.files.end(),[&](const auto& old){return old.path==f.path;});int index=found==model.files.end()?int(model.files.size()):int(found-model.files.begin());if(found==model.files.end())model.files.push_back(f);if(first<0)first=index;}
   model.invalidate();
   if(row>=0&&first>=0){model.assignments[row]=first;model.refresh();status->setText(errors.isEmpty()?"File assigned. Preview again.":errors.join("\n"));if(closing)QDialog::reject();return;}
   // Cooperative matching prevents a large sequence/file table from blocking UI.
   busy(true);auto stems=std::make_shared<std::vector<std::string>>(),internal=std::make_shared<std::vector<std::string>>();
   for(const auto& f:model.files){stems->push_back(QFileInfo(f.path).completeBaseName().toStdString());internal->push_back(f.original->sampleName);}
   auto position=std::make_shared<std::size_t>(0);auto* matching=new QTimer(this);matching->setInterval(0);
   connect(matching,&QTimer::timeout,this,[this,matching,position,stems,internal,errors]{
    if(closing){matching->stop();matching->deleteLater();busy(false);QDialog::reject();return;}
    QElapsedTimer elapsed;elapsed.start();do{if(*position>=model.ids.size())break;const auto i=(*position)++;if(model.assignments[i]<0)model.assignments[i]=suggest_trace(model.source->sequence(model.ids[i]).name,*stems,*internal);}while(elapsed.elapsed()<2);
    status->setText(QString("Matching names: %1/%2").arg(*position).arg(model.ids.size()));
    if(*position==model.ids.size()){matching->stop();matching->deleteLater();busy(false);model.refresh();status->setText(errors.isEmpty()?QString("%1 files loaded. Review assignments, then Preview.").arg(model.files.size()):errors.join("\n"));}
   });matching->start();});
  const auto evidenceBudget=std::min<std::size_t>(model.source->bulk_edit_memory_limit(),512*1024*1024);std::size_t existing=0;for(const auto& f:model.files){existing+=f.original->calls.size()+f.original->quality.size()+f.original->peaks.size()*4;for(const auto& channel:f.original->channels)existing+=channel.size()*4;}
  watcher->setFuture(QtConcurrent::run([paths,cancelled=cancel,evidenceBudget,existing]{std::size_t used=existing;std::vector<File> result;for(const auto& path:paths){if(cancelled->load())break;File f;f.path=QFileInfo(path).absoluteFilePath();try{if(QFileInfo(path).size()>64*1024*1024)throw std::length_error("AB1 file exceeds the 64 MiB linking-input limit");
#ifdef _WIN32
   std::ifstream in(std::filesystem::path(f.path.toStdWString()),std::ios::binary);
#else
   std::ifstream in(f.path.toStdString(),std::ios::binary);
#endif
   if(!in)throw std::runtime_error("Could not read file");
   std::string note;const auto r=read_abif(in,QFileInfo(path).completeBaseName().toStdString(),f.path.toUtf8().toStdString(),&note);std::size_t bytes=r.trace->original->calls.size()+r.trace->original->quality.size()+r.trace->original->peaks.size()*4;for(const auto& channel:r.trace->original->channels)bytes+=channel.size()*4;if(used>evidenceBudget||bytes>evidenceBudget-used)throw std::length_error("Loaded trace evidence exceeds the working-memory limit; use a smaller batch");used+=bytes;f.original=r.trace->original;f.note=QString::fromStdString(note);
   }catch(const std::exception& e){f.note=e.what();}result.push_back(std::move(f));}return result;}));
 }
 void run_preview(){if(running)return;
  const auto budget=std::min<std::size_t>(model.source->bulk_edit_memory_limit(),256*1024*1024);std::size_t planned=model.ids.size()*sizeof(TraceMapping);
  for(std::size_t i=0;i<model.ids.size();++i)if(model.assignments[i]>=0){const auto count=model.source->sequence(model.ids[i]).symbols.residue_count();if(planned>budget||count>(budget-planned)/sizeof(std::int32_t)){status->setText("Mapping output exceeds the working-memory limit. Assign fewer/shorter sequences; this preview respects the bulk setting and a 256 MiB cap.");return;}planned+=count*sizeof(std::int32_t);}
  if(planned>=budget){status->setText("Mapping preview exceeds the working-memory limit; choose fewer sequences.");return;}
  const auto cells=std::min<std::size_t>(8000000,(budget-planned)/16);
  busy(true);model.results.clear();model.refresh();revision=model.source->state_id();cancel=std::make_shared<std::atomic_bool>(false);auto* w=new QFutureWatcher<std::vector<TraceMapping>>(this);const auto source=model.source;const auto ids=model.ids;const auto files=model.files;const auto assignments=model.assignments;const auto cancelled=cancel;status->setText("Mapping both orientations…");
  connect(w,&QFutureWatcher<std::vector<TraceMapping>>::finished,this,[this,w]{model.results=w->result();w->deleteLater();busy(false);model.refresh();if(closing){QDialog::reject();return;}bool valid=false;std::size_t failed=0;for(const auto& r:model.results){valid|=bool(r.link);failed+=!r.link;}apply->setEnabled(valid&&revision==model.source->state_id());status->setText(QString("%1 mappings ready; %2 unassigned/failed rows will keep their current traces. Terrible mappings need careful review before Apply.").arg(model.results.size()-failed).arg(failed));});
  w->setFuture(QtConcurrent::run([source,ids,files,assignments,cancelled,cells]{std::vector<TraceMapping> results;for(std::size_t i=0;i<ids.size();++i){TraceMapping result;try{if(cancelled->load())break;if(assignments[i]>=0)result=map_trace(source->sequence(ids[i]).symbols.text(),files[assignments[i]].original,[cancelled]{return cancelled->load();},cells);}catch(const std::exception& e){result.reason=e.what();}results.push_back(std::move(result));}return results;}));
 }
};}
bool link_trace_dialog(std::shared_ptr<Document> d,const std::vector<SequenceId>& ids,QWidget* p){Dialog dialog(std::move(d),ids,p);dialog.exec();return dialog.applied;}
}
