#include "gui/motif_navigator.hpp"
#include "gui/motif_search.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
namespace kleiotide {
MotifNavigator::MotifNavigator(QWidget* parent):QWidget(parent){
    setObjectName("motifNavigator");auto* layout=new QVBoxLayout(this);layout->setContentsMargins(8,4,8,4);
    auto* top=new QHBoxLayout;layout->addLayout(top);top->addWidget(new QLabel("Nucleotide motif",this));
    query_=new QLineEdit(this);query_->setObjectName("motifQuery");query_->setPlaceholderText("Nucleotide / IUPAC motif");query_->setClearButtonEnabled(true);top->addWidget(query_,1);
    scope_=new QComboBox(this);scope_->setObjectName("motifScope");scope_->addItems({"All nucleotide sequences","Selected sequence names"});top->addWidget(scope_);
    find_=new QPushButton("Find",this);find_->setObjectName("motifFind");top->addWidget(find_);
    previous_=new QPushButton("Previous",this);previous_->setObjectName("motifPrevious");previous_->setToolTip("Previous overlapping hit (Shift+F3)");top->addWidget(previous_);
    next_=new QPushButton("Next",this);next_->setObjectName("motifNext");next_->setToolTip("Next overlapping hit (F3)");top->addWidget(next_);
    auto* options=new QHBoxLayout;layout->addLayout(options);
    ignoreGaps_=new QCheckBox("Ignore gaps",this);ignoreGaps_->setObjectName("motifIgnoreGaps");ignoreGaps_->setChecked(true);ignoreGaps_->setToolTip("Ignore open/locked gaps in the query and rows. Otherwise match aligned gap types exactly; dot means open gap.");
    reverse_=new QCheckBox("Include reverse complement",this);reverse_->setObjectName("motifReverse");
    compatible_=new QCheckBox("Compatible IUPAC",this);compatible_->setObjectName("motifCompatible");compatible_->setToolTip("Match shared possible bases. N/? can match any base; T/U are compatible. Off: literal codes, case-insensitive.");
    wrap_=new QCheckBox("Wrap around",this);wrap_->setObjectName("motifWrap");wrap_->setChecked(true);
    regex_=new QCheckBox("Regex",this);regex_->setObjectName("motifRegex");regex_->setToolTip("Perl-style regex, case-insensitive: [AG], A.{2,4}G, (AAG|CTT), ^ and $. Dot is a wildcard; use ~ for an open gap with Ignore gaps off. IUPAC expansion is disabled in regex mode. Empty matches are not selectable and are skipped.");
    excludeNOnly_=new QCheckBox("Exclude N-only matches",this);excludeNOnly_->setObjectName("motifExcludeNOnly");excludeNOnly_->setToolTip("Exclude intervals whose non-gap symbols are all N (case-insensitive). Mixed matches and gap-only matches remain. Applies to literal and regex searches on both strands.");
    for(auto* box:{ignoreGaps_,reverse_,compatible_,regex_,wrap_,excludeNOnly_})options->addWidget(box);
    options->addStretch();
    cancel_=new QPushButton("Cancel search",this);cancel_->setObjectName("motifCancel");options->addWidget(cancel_);
    counts_=new QLabel("No completed search",this);counts_->setObjectName("motifCounts");counts_->setTextFormat(Qt::PlainText);counts_->setToolTip("Total counts unique aligned intervals. A both-strand interval contributes to Forward and Reverse, but only once to Total.");layout->addWidget(counts_);
    result_=new QLabel("Find selects the first match and shades all matches. Next/Previous select each hit.",this);result_->setObjectName("motifResult");result_->setWordWrap(true);result_->setTextFormat(Qt::PlainText);layout->addWidget(result_);
    connect(find_,&QPushButton::clicked,this,[this]{search(false,true);});connect(query_,&QLineEdit::returnPressed,this,[this]{search(false,true);});
    connect(next_,&QPushButton::clicked,this,[this]{next();});connect(previous_,&QPushButton::clicked,this,[this]{next(true);});
    connect(cancel_,&QPushButton::clicked,this,[this]{cancel();});
    connect(query_,&QLineEdit::textChanged,this,[this]{invalidate("Search settings changed. Choose Find.");});
    connect(scope_,&QComboBox::currentIndexChanged,this,[this]{invalidate("Search scope changed. Choose Find.");});
    for(auto* box:{ignoreGaps_,reverse_,compatible_,excludeNOnly_})connect(box,&QCheckBox::toggled,this,[this]{invalidate("Search settings changed. Choose Find.");});
    connect(regex_,&QCheckBox::toggled,this,[this]{invalidate("Search syntax changed. Choose Find.");});
    update_controls();
}
MotifNavigator::~MotifNavigator(){cancel();}
void MotifNavigator::cancel(){if(cancelled_)cancelled_->store(true);}
void MotifNavigator::focus_query(){query_->setFocus();query_->selectAll();}
void MotifNavigator::set_available(bool available){available_=available;update_controls();}
void MotifNavigator::update_controls(){
    const bool ready=available_&&!searching_;
    for(auto* control:std::initializer_list<QWidget*>{query_,scope_,ignoreGaps_,reverse_,compatible_,regex_,wrap_,excludeNOnly_})control->setEnabled(ready);
    compatible_->setEnabled(ready&&!regex_->isChecked());
    for(auto* button:{find_,previous_,next_})button->setEnabled(ready&&!query_->text().trimmed().isEmpty());
    cancel_->setEnabled(searching_);
}
void MotifNavigator::invalidate(const QString& message,bool clear){cancel();dirty_=true;
    if(clear){hit_.reset();index_.reset();counts_->setText("No completed search");if(results_changed)results_changed({});if(highlighted)highlighted({});}
    result_->setText(message+(index_?" Previous highlights remain until the next completed search or panel close.":""));update_controls();}
void MotifNavigator::close_results(){invalidate("Enter a motif and choose Find.",true);}
void MotifNavigator::set_document(std::shared_ptr<const Document> document,const std::vector<SequenceId>& selected){
    const bool changed=document_.get()!=document.get()||(document&&revision_!=document->state_id());
    const bool scopeChanged=scope_->currentIndex()==1&&selected_!=selected;
    document_=std::move(document);revision_=document_?document_->state_id():0;selected_=selected;
    if(changed||scopeChanged)invalidate(changed?"Document changed. Choose Find to refresh the search.":"Selected sequence names changed. Choose Find.",changed);
}
void MotifNavigator::next(bool backwards){search(backwards,false);}
void MotifNavigator::show_current(bool wrapped){
    if(!index_||!index_->total)return;
    hit_=index_->at(current_);const auto& row=document_->sequence(hit_->id);
    result_->setText(QString("%1Match %2 of %3 · %4 · columns %5–%6 · %7").arg(wrapped?"Wrapped · ":"")
        .arg(static_cast<qulonglong>(current_+1)).arg(static_cast<qulonglong>(index_->total)).arg(QString::fromUtf8(row.name.c_str()))
        .arg(static_cast<qulonglong>(hit_->match.start+1)).arg(static_cast<qulonglong>(hit_->match.start+hit_->match.width))
        .arg(hit_->match.bothStrands?"both strands":hit_->match.reversed?"reverse complement":"forward"));
    if(highlighted)highlighted(hit_);
    if(select_requested)select_requested(*hit_);
}
void MotifNavigator::search(bool backwards,bool restart){
    if(!available_||searching_||!document_)return;
    const auto text=query_->text().toUtf8().toStdString();if(text.empty()){focus_query();return;}
    if(!restart&&!dirty_&&index_){if(!index_->total)return;bool wrapped=false;
        if(backwards){if(current_)--current_;else if(wrap_->isChecked()){current_=index_->total-1;wrapped=true;}else{result_->setText("No previous match; wrap is off. Current match retained.");return;}}
        else{if(current_+1<index_->total)++current_;else if(wrap_->isChecked()){current_=0;wrapped=true;}else{result_->setText("No next match; wrap is off. Current match retained.");return;}}
        show_current(wrapped);return;}
    const auto snapshot=document_;const auto revision=revision_;const bool regex=regex_->isChecked(),excludeNOnly=excludeNOnly_->isChecked();
    const MotifOptions options{ignoreGaps_->isChecked(),reverse_->isChecked(),!regex&&compatible_->isChecked()};
    std::optional<std::vector<SequenceId>> scope;if(scope_->currentIndex()==1)scope=selected_;
    cancelled_=std::make_shared<std::atomic_bool>(false);const auto token=cancelled_;
    searching_=true;result_->setText("Searching…");update_controls();if(searching_changed)searching_changed(true);
    struct Result {MotifSearchResult search;QString error;};auto* watcher=new QFutureWatcher<Result>(this);
    connect(watcher,&QFutureWatcher<Result>::finished,this,[this,watcher,snapshot,revision,token]{
        const auto result=watcher->result();watcher->deleteLater();searching_=false;
        const bool stale=document_.get()!=snapshot.get()||!document_||document_->state_id()!=revision;
        if(searching_changed)searching_changed(false);
        if(stale)invalidate("Document changed. Search again.",true);
        else if(token->load())result_->setText(index_?"Search cancelled; the previous matches are unchanged.":"Search cancelled.");
        else if(!result.error.isEmpty())result_->setText(result.error+(index_?" Previous results are unchanged.":""));
        else{index_=result.search.index;current_=0;dirty_=false;hit_.reset();
            counts_->setText(QString("Total: %1 · Forward: %2 · Reverse complement: %3 · Both: %4%5")
                .arg(static_cast<qulonglong>(index_->total)).arg(static_cast<qulonglong>(index_->forward)).arg(static_cast<qulonglong>(index_->reverse)).arg(static_cast<qulonglong>(index_->both))
                .arg(result.search.emptyMatches?QString(" · %1 empty regex matches ignored").arg(static_cast<qulonglong>(result.search.emptyMatches)):QString{}));
            if(results_changed)results_changed(index_);
            if(index_->total)show_current();else{if(highlighted)highlighted({});result_->setText("No matching nucleotide sequence in this scope.");}}
        update_controls();
    });
    watcher->setFuture(QtConcurrent::run([snapshot,text,options,regex,excludeNOnly,scope,token]{Result result;
        try{result.search=search_motifs(*snapshot,text,options,regex,scope,[token]{return token->load();},excludeNOnly);}
        catch(const std::exception& e){result.error=QString::fromUtf8(e.what());}return result;}));
}
}
