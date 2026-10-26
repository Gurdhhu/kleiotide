#include "gui/site_highlighter.hpp"
#include "gui/alignment_view.hpp"
#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFormLayout>
#include <QJsonArray>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <cmath>
namespace kleiotide {
SiteHighlighter::SiteHighlighter(AlignmentView* view,QWidget* owner):QObject(owner),view_(view),owner_(owner){
    work_.setInterval(4);debounce_.setSingleShot(true);debounce_.setInterval(120);
    connect(&debounce_,&QTimer::timeout,this,[this]{if(source_&&rules_.enabled())try{scan_=std::make_unique<SiteScan>(*source_,scope_);work_.start();advance();}catch(const std::exception& e){view_->setToolTip(QString::fromUtf8(e.what()));}});
    connect(&work_,&QTimer::timeout,this,[this]{advance();});
}
void SiteHighlighter::add_menu(QMenu* menu,std::vector<QAction*>& guarded){
    menu->setToolTipsVisible(true);menu->setObjectName("highlightMenu");
    auto add=[&](const QString& title,const char* id){auto* a=menu->addAction(title);a->setObjectName(id);guarded.push_back(a);return a;};
    scopeSummary_=menu->addAction("All nucleotide rows · all columns");scopeSummary_->setEnabled(false);
    scopeAction_=add("Analysis scope…","highlightScope");connect(scopeAction_,&QAction::triggered,this,[this]{choose_scope();});menu->addSeparator();
    const char* titles[]={"Conserved sites","Variable sites","Parsimony-informative sites","Singleton sites"};
    const char* ids[]={"highlightConserved","highlightVariable","highlightInformative","highlightSingleton"};
    const char* tips[]={"Calls share a possible state and contain information. Compatible ambiguities count; Unknown-only and gap-only columns do not. Use 100% Conservation for strict conservation.","Calls have no possible state in common (gaps and unknowns are missing).","At least two definite states each occur at least twice. Ambiguities do not count as definite observations.","Variable sites that are not parsimony-informative, matching the alignment statistics convention."};
    for(int i=0;i<4;++i){auto* a=add(titles[i],ids[i]);a->setCheckable(true);a->setToolTip(tips[i]);categories_.push_back(a);connect(a,&QAction::toggled,this,[this,i](bool on){const std::array<bool*,4> values{&rules_.conserved,&rules_.variable,&rules_.informative,&rules_.singleton};*values[i]=on;publish();});}
    coverageAction_=add("% Coverage…","highlightCoverage");coverageAction_->setCheckable(true);coverageAction_->setToolTip("Choose an inclusive percentage range and non-gap or informative coverage. Clicking reopens its settings.");connect(coverageAction_,&QAction::triggered,this,[this]{choose_threshold(false);});
    conservationAction_=add("% Conservation…","highlightConservation");conservationAction_->setCheckable(true);conservationAction_->setToolTip("Inclusive range of the most frequent state fraction. Partial ambiguities vote equally; unknowns and gaps do not vote.");connect(conservationAction_,&QAction::triggered,this,[this]{choose_threshold(true);});
    menu->addSeparator();auto* color=add("Highlight color…","highlightColor");connect(color,&QAction::triggered,this,[this]{const auto chosen=QColorDialog::getColor(color_,owner_,"Highlight color");if(chosen.isValid()){color_=chosen;publish();}});
    auto* clear=add("Clear highlighting","highlightClear");connect(clear,&QAction::triggered,this,[this]{rules_=HighlightRules{};sync_actions();publish();});sync_actions();
}
void SiteHighlighter::set_document(const Document* source){
    if(source_!=source){source_=source;scope_=AnalysisScope{};rules_=HighlightRules{};sync_actions();invalidate();}
    else if(source&&revision_!=source->content_id())invalidate();
}
void SiteHighlighter::invalidate(){
    work_.stop();debounce_.stop();scan_.reset();revision_=source_?source_->content_id():0;view_->set_site_highlights({},color_,scope_.molecule,scope_.selectedRows?&scope_.ids:nullptr);
    if(source_&&rules_.enabled())debounce_.start();
}
void SiteHighlighter::advance(){
    if(!source_||!scan_)return;
    if(revision_!=source_->content_id()){invalidate();return;}
    QElapsedTimer elapsed;elapsed.start();
    try{do{if(scan_->step(*source_,4096)){work_.stop();publish();return;}}while(elapsed.nsecsElapsed()<2000000);}
    catch(const std::exception& e){work_.stop();view_->setToolTip(QString::fromUtf8(e.what()));}
}
void SiteHighlighter::publish(){
    if(!rules_.enabled()){work_.stop();debounce_.stop();view_->set_site_highlights({},color_,scope_.molecule,scope_.selectedRows?&scope_.ids:nullptr);return;}
    if(source_&&revision_!=source_->content_id()){invalidate();return;}
    if(scan_&&scan_->complete())view_->set_site_highlights(std::make_shared<const std::vector<unsigned char>>(scan_->highlights(rules_)),color_,scope_.molecule,scope_.selectedRows?&scope_.ids:nullptr);
    else if(source_&&!work_.isActive()&&!debounce_.isActive())debounce_.start();
}
void SiteHighlighter::sync_actions(){
    const bool values[]={rules_.conserved,rules_.variable,rules_.informative,rules_.singleton};
    for(std::size_t i=0;i<categories_.size();++i){QSignalBlocker block(categories_[i]);categories_[i]->setChecked(values[i]);}
    if(coverageAction_)coverageAction_->setChecked(rules_.coverage);
    if(conservationAction_)conservationAction_->setChecked(rules_.conservation);
    if(scopeSummary_)scopeSummary_->setText(QString("%1 %2 · %3").arg(scope_.selectedRows?"Selected":"All").arg(scope_.molecule==RowKind::Protein?"protein rows":"nucleotide rows").arg(scope_.selectedColumns?QString("columns %1–%2").arg(scope_.begin+1).arg(scope_.end):"all columns"));
    if(scopeAction_)scopeAction_->setToolTip(QString("%1 rows; %2; %3. Masks never contribute. Rules combine by union. Highlights mark only the analyzed rows.")
        .arg(scope_.molecule==RowKind::Protein?"Protein":"Nucleotide").arg(scope_.selectedRows?QString("snapshot of %1 selected names").arg(scope_.ids.size()):"all names")
        .arg(scope_.selectedColumns?QString("columns %1–%2").arg(scope_.begin+1).arg(scope_.end):"all columns"));
}
void SiteHighlighter::choose_scope(){
    QDialog dialog(owner_);dialog.setObjectName("highlightScopeDialog");dialog.setWindowTitle("Highlight analysis scope");auto* layout=new QVBoxLayout(&dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* molecule=new QComboBox(&dialog);molecule->setObjectName("highlightMolecule");molecule->addItems({"Nucleotide","Protein"});molecule->setCurrentIndex(scope_.molecule==RowKind::Protein?1:0);form->addRow("Analyze",molecule);
    auto* rows=new QComboBox(&dialog);rows->setObjectName("highlightRows");rows->addItems({"All rows of this molecule type","Current selected sequence names (snapshot)"});rows->setCurrentIndex(scope_.selectedRows?1:0);form->addRow("Rows",rows);
    auto* columns=new QComboBox(&dialog);columns->setObjectName("highlightColumns");columns->addItems({"All alignment columns","Current selected columns (snapshot)"});columns->setCurrentIndex(scope_.selectedColumns?1:0);form->addRow("Columns",columns);
    auto* note=new QLabel("Rules combine by union. Selected names/columns are captured when Apply is pressed; later clicking or dragging a selection does not change the analysis scope. Deleted rows are ignored. Choose this scope again to capture a new selection. Only the analyzed rows receive shading. Protein and nucleotide populations never mix; masks are excluded.",&dialog);note->setWordWrap(true);layout->addWidget(note);
    auto* status=new QLabel(&dialog);status->setWordWrap(true);layout->addWidget(status);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,[&]{
        AnalysisScope next;next.molecule=molecule->currentIndex()?RowKind::Protein:RowKind::Nucleotide;next.selectedRows=rows->currentIndex()==1;next.ids=view_->selected_ids();next.selectedColumns=columns->currentIndex()==1;
        std::size_t eligible=0;if(source_)for(const auto& row:source_->rows())if(row.kind==next.molecule&&(!next.selectedRows||std::find(next.ids.begin(),next.ids.end(),row.id)!=next.ids.end()))++eligible;
        if(!eligible){status->setText("No eligible rows in this scope. Select names of the chosen type or choose All rows.");return;}
        if(next.selectedColumns){if(!view_->fragment()){status->setText("Select an alignment fragment or columns first.");return;}next.begin=view_->fragment()->start;next.end=next.begin+view_->fragment()->width;}
        if(!next.selectedRows)next.ids.clear();
        scope_=std::move(next);invalidate();sync_actions();dialog.accept();
    });dialog.resize(570,300);dialog.exec();
}
void SiteHighlighter::choose_threshold(bool conservation){
    QDialog dialog(owner_);dialog.setObjectName("highlightThresholdDialog");dialog.setWindowTitle(conservation?"% Conservation":"% Coverage");auto* layout=new QVBoxLayout(&dialog);
    auto* enabled=new QCheckBox("Enable this rule",&dialog);enabled->setObjectName("highlightThresholdEnabled");enabled->setChecked(true);layout->addWidget(enabled);
    auto* form=new QFormLayout;layout->addLayout(form);auto* low=new QDoubleSpinBox(&dialog);auto* high=new QDoubleSpinBox(&dialog);low->setObjectName("highlightMinimum");high->setObjectName("highlightMaximum");for(auto* n:{low,high}){n->setRange(0,100);n->setDecimals(2);n->setSuffix(" %");}
    low->setValue(conservation?rules_.conservationMin:rules_.coverageMin);high->setValue(conservation?rules_.conservationMax:rules_.coverageMax);form->addRow("At least (inclusive)",low);form->addRow("At most (inclusive)",high);
    auto* basis=new QComboBox(&dialog);basis->setObjectName("highlightCoverageBasis");const auto unknown=scope_.molecule==RowKind::Protein?QString("X"):QString("N");basis->addItems({"Non-gap: includes "+unknown+" and ?","Informative: excludes "+unknown+" and ?"});basis->setCurrentIndex(rules_.informativeCoverage?1:0);if(!conservation)form->addRow("Coverage basis",basis);else basis->hide();
    auto* note=new QLabel(conservation?"Conservation = largest state vote / all informative votes. IUPAC calls (and protein B/Z/J) split their vote equally. Unknowns (nucleotide N, protein X), ?, gaps and absent trailing cells do not vote; columns without votes are excluded. Conserved sites uses possible-state compatibility, which can differ from 100% conservation.":"Coverage = non-gap (or informative) calls / number of rows in the explicit analysis scope. Gaps and absent trailing cells count against coverage. Partial ambiguities and protein N are informative. This inclusive range is combined with all other enabled rules by union.",&dialog);note->setWordWrap(true);layout->addWidget(note);
    auto* status=new QLabel(&dialog);layout->addWidget(status);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{if(low->value()>high->value()){status->setText("Minimum must not exceed maximum.");return;}if(conservation){rules_.conservation=enabled->isChecked();rules_.conservationMin=low->value();rules_.conservationMax=high->value();}else{rules_.coverage=enabled->isChecked();rules_.coverageMin=low->value();rules_.coverageMax=high->value();rules_.informativeCoverage=basis->currentIndex()==1;}dialog.accept();});
    dialog.resize(560,340);dialog.exec();sync_actions();publish();
}
QJsonObject SiteHighlighter::state()const{
    QJsonObject s;s["molecule"]=scope_.molecule==RowKind::Protein?"protein":"nucleotide";s["selectedRows"]=scope_.selectedRows;s["selectedColumns"]=scope_.selectedColumns;s["begin"]=QString::number(static_cast<qulonglong>(scope_.begin));s["end"]=QString::number(static_cast<qulonglong>(scope_.end));
    QJsonArray ids;for(auto id:scope_.ids)ids.append(QString::number(id));s["ids"]=ids;s["color"]=color_.name();
    s["conserved"]=rules_.conserved;s["variable"]=rules_.variable;s["informative"]=rules_.informative;s["singleton"]=rules_.singleton;s["coverage"]=rules_.coverage;s["conservation"]=rules_.conservation;s["informativeCoverage"]=rules_.informativeCoverage;
    s["coverageMin"]=rules_.coverageMin;s["coverageMax"]=rules_.coverageMax;s["conservationMin"]=rules_.conservationMin;s["conservationMax"]=rules_.conservationMax;return s;
}
void SiteHighlighter::restore(const QJsonObject& s){
    scope_=AnalysisScope{};rules_=HighlightRules{};color_=QColor(198,173,242);
    scope_.molecule=s["molecule"].toString()=="protein"?RowKind::Protein:RowKind::Nucleotide;scope_.selectedRows=s["selectedRows"].toBool();scope_.selectedColumns=s["selectedColumns"].toBool();
    scope_.begin=static_cast<std::size_t>(std::min<qulonglong>(s["begin"].toString().toULongLong(),source_?source_->columns():0));scope_.end=static_cast<std::size_t>(std::clamp<qulonglong>(s["end"].toString().toULongLong(),scope_.begin,source_?source_->columns():0));
    for(const auto& id:s["ids"].toArray())if(auto n=id.toString().toULongLong())scope_.ids.push_back(n);
    const QColor color(s["color"].toString());if(color.isValid())color_=color;
    rules_.conserved=s["conserved"].toBool();rules_.variable=s["variable"].toBool();rules_.informative=s["informative"].toBool();rules_.singleton=s["singleton"].toBool();rules_.coverage=s["coverage"].toBool();rules_.conservation=s["conservation"].toBool();rules_.informativeCoverage=s["informativeCoverage"].toBool();
    auto percent=[&](const char* key,double fallback){const auto n=s[key].toDouble(fallback);return std::isfinite(n)?std::clamp(n,0.0,100.0):fallback;};rules_.coverageMin=percent("coverageMin",0);rules_.coverageMax=std::max(rules_.coverageMin,percent("coverageMax",100));rules_.conservationMin=percent("conservationMin",80);rules_.conservationMax=std::max(rules_.conservationMin,percent("conservationMax",100));sync_actions();invalidate();
}
}
