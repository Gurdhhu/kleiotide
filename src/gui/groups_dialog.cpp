#include "gui/groups_dialog.hpp"
#include "gui/main_window.hpp"
#include "gui/alignment_view.hpp"
#include <QStatusBar>
#include <QMessageBox>
#include "core/group_import.hpp"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QPixmap>
#include <QPainter>
#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
namespace kleiotide {
namespace {
QString text(const std::string& s){return QString::fromUtf8(s.data(),static_cast<qsizetype>(s.size()));}
QColor color(std::uint32_t rgb){return QColor::fromRgb(rgb);}
std::uint32_t rgb(const QColor& c){return c.rgb()&0xffffff;}
QIcon group_marker(std::uint32_t groupColor){
    QPixmap swatch(14,14);swatch.fill(Qt::transparent);
    {QPainter painter(&swatch);painter.setPen(QColor(90,90,90));painter.setBrush(color(groupColor));painter.drawRect(1,1,11,11);}
    QIcon icon;for(auto mode:{QIcon::Normal,QIcon::Selected,QIcon::Active,QIcon::Disabled})icon.addPixmap(swatch,mode,QIcon::Off);return icon;
}
std::string unique_name(const std::vector<SequenceGroup>& groups,std::string name){auto result=name;unsigned n=2;while(std::any_of(groups.begin(),groups.end(),[&](const auto& g){return g.name==result;}))result=name+" "+std::to_string(n++);return result;}
}
bool manage_groups(Document& d,const std::vector<SequenceId>& selected,QWidget* parent){
    auto groups=d.groups();auto next=d.next_group_id();std::unordered_map<SequenceId,GroupId> members;std::unordered_map<GroupId,std::size_t> counts;std::unordered_set<GroupId> dirtyCounts;std::unordered_map<GroupId,QListWidgetItem*> groupItems;
    struct GroupStyle {QString name;QIcon marker;};std::unordered_map<GroupId,GroupStyle> groupStyles;
    for(const auto& row:d.rows()){members[row.id]=row.groupId;++counts[row.groupId];}
    // Update staged totals in constant time per changed member. Bulk operations
    // update the group labels once, without rescanning all sequence names.
    auto setMember=[&](SequenceId row,GroupId target){auto& old=members.at(row);if(old==target)return;--counts[old];++counts[target];dirtyCounts.insert(old);dirtyCounts.insert(target);old=target;};
    QDialog dialog(parent);dialog.setObjectName("groupManager");dialog.setWindowTitle("Sequence groups");dialog.resize(850,580);QVBoxLayout layout(&dialog);
    QLabel note("Each sequence belongs to one group or Ungrouped. Check members, or drag to select multiple names and use Check selected / Uncheck selected. Colored markers show each sequence’s current group; unmarked names are Ungrouped. Apply saves all changes.");note.setWordWrap(true);layout.addWidget(&note);
    QHBoxLayout body;layout.addLayout(&body,1);QListWidget list;list.setObjectName("groupList");list.setMaximumWidth(280);body.addWidget(&list);
    QVBoxLayout right;body.addLayout(&right,1);QLineEdit filter;filter.setObjectName("groupMemberFilter");filter.setPlaceholderText("Filter sequence names");right.addWidget(&filter);QListWidget rows;rows.setObjectName("groupMembers");rows.setSelectionMode(QAbstractItemView::ExtendedSelection);rows.setSelectionRectVisible(true);rows.setDragEnabled(false);right.addWidget(&rows);
    QHBoxLayout memberButtons;QPushButton selectAll("Check all"),unselectAll("Uncheck all"),checkSelected("Check selected"),uncheckSelected("Uncheck selected");checkSelected.setObjectName("groupMembersCheckSelected");uncheckSelected.setObjectName("groupMembersUncheckSelected");selectAll.setObjectName("groupMembersSelectAll");unselectAll.setObjectName("groupMembersUnselectAll");selectAll.setToolTip("Check all currently visible sequence names as members of this group");unselectAll.setToolTip("Uncheck all currently visible sequence names in this group");memberButtons.addWidget(&selectAll);memberButtons.addWidget(&unselectAll);memberButtons.addWidget(&checkSelected);memberButtons.addWidget(&uncheckSelected);memberButtons.addStretch();right.addLayout(&memberButtons);
    QHBoxLayout tools;QPushButton create("New…"),rename("Rename…"),paint("Color…"),remove("Delete"),up("↑"),down("↓");
    create.setObjectName("groupNew");rename.setObjectName("groupRename");paint.setObjectName("groupColor");remove.setObjectName("groupDelete");up.setObjectName("groupUp");down.setObjectName("groupDown");
    up.setToolTip("Move group earlier in By group sorting order");down.setToolTip("Move group later in By group sorting order");
    for(auto* b:{&create,&rename,&paint,&remove,&up,&down})tools.addWidget(b);
    layout.addLayout(&tools);
    QPushButton clearAll("Clear and delete all groups"),deleteEmpty("Delete empty groups");clearAll.setObjectName("groupClearAll");deleteEmpty.setObjectName("groupDeleteEmpty");deleteEmpty.setToolTip("Delete groups with no members across the whole alignment, regardless of the name filter");QHBoxLayout cleanup;cleanup.addWidget(&deleteEmpty);cleanup.addWidget(&clearAll);layout.addLayout(&cleanup);
    QPushButton add("Add alignment-selected sequences to this group");add.setObjectName("groupAssignSelected");add.setEnabled(!selected.empty());layout.addWidget(&add);
    QLabel error;error.setObjectName("groupManagerError");error.setWordWrap(true);layout.addWidget(&error);QDialogButtonBox buttons(QDialogButtonBox::Apply|QDialogButtonBox::Cancel);layout.addWidget(&buttons);
    auto chosen=[&]()->GroupId{return list.currentItem()?list.currentItem()->data(Qt::UserRole).toULongLong():0;};
    auto refreshCounts=[&]{
        for(auto id:dirtyCounts){const auto it=groupItems.find(id);if(it==groupItems.end())continue;auto* item=it->second;const auto count=static_cast<qulonglong>(counts[id]);item->setText(QString("%1 (%2)").arg(item->data(Qt::UserRole+1).toString()).arg(count));item->setData(Qt::UserRole+2,QVariant::fromValue<qulonglong>(count));item->setToolTip(QString("%1 sequences in the full alignment; the name filter does not change this count.").arg(count));}
        dirtyCounts.clear();deleteEmpty.setEnabled(std::any_of(groups.begin(),groups.end(),[&](const auto& group){return counts[group.id]==0;}));
    };
    auto styleMember=[&](QListWidgetItem* item){
        QSignalBlocker guard(rows);const auto member=members.at(item->data(Qt::UserRole).toULongLong());const auto found=groupStyles.find(member);
        item->setIcon(found==groupStyles.end()?QIcon{}:found->second.marker);
        item->setToolTip(found==groupStyles.end()?QString("Ungrouped"):QString("Group: %1").arg(found->second.name));
        item->setData(Qt::UserRole+1,QVariant::fromValue<qulonglong>(member));
    };
    auto refreshRows=[&]{QSignalBlocker guard(rows);rows.clear();const auto id=chosen();for(const auto& row:d.rows()){
        auto* item=new QListWidgetItem(QString("%1. %2").arg(rows.count()+1).arg(text(row.name)),&rows);item->setData(Qt::UserRole,QVariant::fromValue<qulonglong>(row.id));
        if(id)item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
        item->setCheckState(members.at(row.id)==id?Qt::Checked:Qt::Unchecked);if(!id)item->setFlags(item->flags()&~Qt::ItemIsUserCheckable);
        styleMember(item);item->setHidden(!text(row.name).contains(filter.text(),Qt::CaseInsensitive));}
        clearAll.setEnabled(!groups.empty());checkSelected.setEnabled(id&&!rows.selectedItems().empty());uncheckSelected.setEnabled(id&&!rows.selectedItems().empty());selectAll.setEnabled(id!=0);unselectAll.setEnabled(id!=0);add.setText(id?"Add alignment-selected sequences to this group":"Set alignment-selected sequences to Ungrouped");rename.setEnabled(id!=0);paint.setEnabled(id!=0);remove.setEnabled(id!=0);up.setEnabled(list.currentRow()>1);down.setEnabled(id&&list.currentRow()<list.count()-1);
    };
    auto refresh=[&](GroupId focus){QSignalBlocker guard(list);groupStyles.clear();for(const auto& group:groups)groupStyles.emplace(group.id,GroupStyle{text(group.name),group_marker(group.color)});groupItems.clear();list.clear();auto* ungrouped=new QListWidgetItem("Ungrouped",&list);ungrouped->setData(Qt::UserRole,QVariant::fromValue<qulonglong>(0));ungrouped->setData(Qt::UserRole+1,"Ungrouped");groupItems[0]=ungrouped;dirtyCounts.insert(0);int current=0;
        for(const auto& g:groups){auto* item=new QListWidgetItem(text(g.name),&list);item->setData(Qt::UserRole,QVariant::fromValue<qulonglong>(g.id));item->setData(Qt::UserRole+1,text(g.name));groupItems[g.id]=item;dirtyCounts.insert(g.id);item->setIcon(groupStyles.at(g.id).marker);if(g.id==focus)current=list.count()-1;}
        list.setCurrentRow(current);refreshCounts();refreshRows();};
    QObject::connect(&list,&QListWidget::currentRowChanged,&dialog,[&]{refreshRows();});QObject::connect(&filter,&QLineEdit::textChanged,&dialog,[&]{refreshRows();});
    QObject::connect(&rows,&QListWidget::itemChanged,&dialog,[&](QListWidgetItem* item){const auto id=chosen();if(!id)return;const auto row=item->data(Qt::UserRole).toULongLong();if(item->checkState()==Qt::Checked)setMember(row,id);else if(members.at(row)==id)setMember(row,0);styleMember(item);refreshCounts();});
    QObject::connect(&rows,&QListWidget::itemSelectionChanged,&dialog,[&]{const bool on=chosen()&&!rows.selectedItems().empty();checkSelected.setEnabled(on);uncheckSelected.setEnabled(on);});
    auto setVisible=[&](bool checked,bool highlightedOnly=false){const auto id=chosen();if(!id)return;QSignalBlocker guard(rows);for(int i=0;i<rows.count();++i){auto* item=rows.item(i);if(item->isHidden()||(highlightedOnly&&!item->isSelected()))continue;const auto row=item->data(Qt::UserRole).toULongLong();if(checked)setMember(row,id);else if(members.at(row)==id)setMember(row,0);item->setCheckState(members.at(row)==id?Qt::Checked:Qt::Unchecked);styleMember(item);}refreshCounts();};
    QObject::connect(&selectAll,&QPushButton::clicked,&dialog,[&]{setVisible(true);});QObject::connect(&unselectAll,&QPushButton::clicked,&dialog,[&]{setVisible(false);});
    QObject::connect(&checkSelected,&QPushButton::clicked,&dialog,[&]{setVisible(true,true);});QObject::connect(&uncheckSelected,&QPushButton::clicked,&dialog,[&]{setVisible(false,true);});
    QObject::connect(&clearAll,&QPushButton::clicked,&dialog,[&]{if(QMessageBox::warning(&dialog,"Clear and delete all groups","Clear all group memberships and delete every group? All sequences will become Ungrouped when you click Apply. Cancel in this manager discards this change; Undo restores it after Apply.",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)!=QMessageBox::Yes)return;groups.clear();for(auto& [row,group]:members){(void)group;setMember(row,0);}refresh(0);});
    QObject::connect(&deleteEmpty,&QPushButton::clicked,&dialog,[&]{const auto focus=chosen();std::erase_if(groups,[&](const auto& group){return counts[group.id]==0;});refresh(focus);});
    QObject::connect(&add,&QPushButton::clicked,&dialog,[&]{for(auto id:selected)setMember(id,chosen());refreshCounts();refreshRows();});
    QObject::connect(&create,&QPushButton::clicked,&dialog,[&]{bool ok=false;const auto name=QInputDialog::getText(&dialog,"New group","Name",QLineEdit::Normal,text(unique_name(groups,"Group")),&ok);if(!ok)return;if(next==std::numeric_limits<GroupId>::max()){error.setText("Group ID space exhausted.");return;}const auto id=next++;groups.push_back({id,name.toUtf8().toStdString(),default_group_color(groups.size())});refresh(id);});
    QObject::connect(&rename,&QPushButton::clicked,&dialog,[&]{const auto id=chosen();auto it=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return g.id==id;});if(it==groups.end())return;bool ok=false;const auto name=QInputDialog::getText(&dialog,"Rename group","Name",QLineEdit::Normal,text(it->name),&ok);if(ok){it->name=name.toUtf8().toStdString();refresh(id);}});
    QObject::connect(&paint,&QPushButton::clicked,&dialog,[&]{const auto id=chosen();for(auto& g:groups)if(g.id==id){const auto c=QColorDialog::getColor(color(g.color),&dialog,"Group color");if(c.isValid())g.color=rgb(c);break;}refresh(id);});
    QObject::connect(&remove,&QPushButton::clicked,&dialog,[&]{const auto id=chosen();std::erase_if(groups,[&](const auto& g){return g.id==id;});for(auto& [row,g]:members)if(g==id)setMember(row,0);refresh(0);});
    auto move=[&](int delta){const auto id=chosen();auto it=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return g.id==id;});if(it==groups.end())return;const auto index=it-groups.begin(),target=index+delta;if(target<0||target>=static_cast<std::ptrdiff_t>(groups.size()))return;std::iter_swap(it,groups.begin()+target);refresh(id);};
    QObject::connect(&up,&QPushButton::clicked,&dialog,[&]{move(-1);});QObject::connect(&down,&QPushButton::clicked,&dialog,[&]{move(1);});
    bool changed=false;QObject::connect(buttons.button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,[&]{try{std::vector<std::pair<SequenceId,GroupId>> assignments;for(const auto& row:d.rows())assignments.emplace_back(row.id,members.at(row.id));const auto before=d.state_id();d.apply_groups(groups,assignments);changed=d.state_id()!=before;dialog.accept();}catch(const std::exception& e){error.setText(e.what());}});
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);refresh(groups.empty()?0:groups.front().id);dialog.exec();return changed;
}
bool assign_group(Document& d,const std::vector<SequenceId>& ids,QWidget* parent){
    if(ids.empty())return false;
    QDialog dialog(parent);dialog.setObjectName("assignGroupDialog");dialog.setWindowTitle("Add to group");QFormLayout layout(&dialog);
    QComboBox choice;choice.setObjectName("assignGroupChoice");for(const auto& g:d.groups())choice.addItem(group_marker(g.color),text(g.name),QVariant::fromValue<qulonglong>(g.id));choice.addItem("Create new…",QVariant::fromValue<qulonglong>(0));layout.addRow("Group",&choice);
    QLineEdit name;name.setObjectName("assignGroupName");name.setText(text(unique_name(d.groups(),"Group")));layout.addRow("New group name",&name);QColor selected=color(default_group_color(d.groups().size()));QPushButton paint("Choose color…");paint.setObjectName("assignGroupColor");layout.addRow(&paint);
    auto refresh=[&]{const bool fresh=choice.currentData().toULongLong()==0;name.setEnabled(fresh);paint.setEnabled(fresh);};QObject::connect(&choice,&QComboBox::currentIndexChanged,&dialog,[&]{refresh();});refresh();
    QObject::connect(&paint,&QPushButton::clicked,&dialog,[&]{const auto c=QColorDialog::getColor(selected,&dialog,"Group color");if(c.isValid())selected=c;});
    QLabel error;error.setObjectName("assignGroupError");error.setWordWrap(true);layout.addRow(&error);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout.addRow(&buttons);bool changed=false;
    QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,[&]{try{auto groups=d.groups();auto id=choice.currentData().toULongLong();if(!id){id=d.next_group_id();groups.push_back({id,name.text().toUtf8().toStdString(),rgb(selected)});}std::vector<std::pair<SequenceId,GroupId>> members;for(auto row:ids)members.emplace_back(row,id);const auto before=d.state_id();d.apply_groups(std::move(groups),members);changed=before!=d.state_id();dialog.accept();}catch(const std::exception& e){error.setText(e.what());}});
    QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);dialog.exec();return changed;
}
std::optional<std::vector<SequenceId>> select_groups(const Document& d,QWidget* parent){
    QDialog dialog(parent);dialog.setObjectName("selectGroupsDialog");dialog.setWindowTitle("Select sequence groups");dialog.resize(420,420);QVBoxLayout layout(&dialog);QListWidget list;list.setObjectName("selectGroupsList");layout.addWidget(&list);
    auto add=[&](const QString& title,GroupId id){auto* item=new QListWidgetItem(title,&list);item->setData(Qt::UserRole,QVariant::fromValue<qulonglong>(id));item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(Qt::Unchecked);};add("Ungrouped",0);for(const auto& g:d.groups())add(text(g.name),g.id);
    QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout.addWidget(&buttons);QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);if(dialog.exec()!=QDialog::Accepted)return {};
    std::vector<SequenceId> ids;for(const auto& row:d.rows())for(int i=0;i<list.count();++i)if(list.item(i)->checkState()==Qt::Checked&&list.item(i)->data(Qt::UserRole).toULongLong()==row.groupId){ids.push_back(row.id);break;}return ids;
}
void MainWindow::group_action(int operation){
    if(busy_)return;
    try{const auto ids=view_->selected_ids();bool changed=false;
        if(operation==0)changed=manage_groups(*document_,ids,this);
        else if(operation==5){
            const auto plan=plan_identical_groups(*document_);
            QDialog dialog(this);dialog.setObjectName("groupIdenticalDialog");dialog.setWindowTitle("Group identical sequences");QVBoxLayout layout(&dialog);
            QLabel count(QString("Identical sequence groups: %1").arg(plan.groups.size()));layout.addWidget(&count);QLabel warning("This replaces all existing groups and memberships");layout.addWidget(&warning);QToolButton toggle;toggle.setText("Comparison rules");toggle.setCheckable(true);toggle.setArrowType(Qt::RightArrow);toggle.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);layout.addWidget(&toggle,0,Qt::AlignLeft);QLabel rules("Matching ignores case and gaps. Full ungapped strings must match; each sequence type is compared separately. Singletons become Ungrouped.");rules.setWordWrap(true);rules.hide();layout.addWidget(&rules);connect(&toggle,&QToolButton::toggled,&dialog,[&](bool on){rules.setVisible(on);toggle.setArrowType(on?Qt::DownArrow:Qt::RightArrow);});
            QDialogButtonBox buttons(QDialogButtonBox::Apply|QDialogButtonBox::Cancel);layout.addWidget(&buttons);connect(buttons.button(QDialogButtonBox::Apply),&QPushButton::clicked,&dialog,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
            if(dialog.exec()==QDialog::Accepted){const auto before=document_->state_id();document_->apply_groups(plan.groups,plan.assignments);changed=before!=document_->state_id();}
        }
        else if(operation==3)changed=import_groups(*document_,this);
        else if(operation==4){if(auto selected=select_groups(*document_,this))view_->select_ids(*selected);return;}
        else if(ids.empty()){statusBar()->showMessage("Select sequence names first.",5000);return;}
        else if(operation==1)changed=assign_group(*document_,ids,this);
        else if(operation==2){std::vector<std::pair<SequenceId,GroupId>> members;for(auto id:ids)members.emplace_back(id,0);const auto before=document_->state_id();document_->apply_groups(document_->groups(),members);changed=before!=document_->state_id();}
        if(changed){view_->refresh();update_state();}
    }catch(const std::exception& e){QMessageBox::warning(this,"Sequence groups",QString::fromUtf8(e.what()));}
}

}
