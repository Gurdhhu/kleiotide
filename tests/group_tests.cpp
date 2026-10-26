#include "core/group_import.hpp"
#include "core/session.hpp"
#include "core/abif.hpp"
#include "core/mafft_template.hpp"
#include "abif_fixture.hpp"
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace kleiotide;
namespace {void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}template<class F>void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}require(threw,"Expected rejection");}
std::vector<SequenceGroup> catalog(){return {{1,"Alps",0x0072b2},{2,"Caucasus",0xe69f00}};}
}
int main(){int passed=0,failed=0;auto run=[&](const char* name,const std::function<void()>& f){try{f();++passed;std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}};
run("Groups edit membership atomically without invalidating biological content",[]{Document d;d.load({{"dna","ACGT"},{"protein","MEF",RowKind::Protein},{"mask","012",RowKind::Mask}});const auto content=d.content_id();d.apply_groups(catalog(),{{1,1},{2,2},{3,1}});
    require(d.sequence(1).groupId==1&&d.sequence(2).groupId==2&&d.sequence(3).groupId==1&&d.content_id()==content,"All row kinds can belong; metadata does not rescan residues");
    const auto state=d.state_id();d.apply_groups(catalog(),{{1,1}});require(d.state_id()==state,"No-op group assignment adds no history");d.undo();require(d.groups().empty()&&!d.sequence(1).groupId&&!d.can_undo(),"One undo restores catalog and members");d.redo();require(d.groups()==catalog()&&d.sequence(3).groupId==1,"One redo restores all metadata");
});
run("Delete/rename/reorder groups and rows retain stable membership and exact undo",[]{Document d;d.load({{"a","A"},{"b","C"},{"c","G"},{"d","T"}});d.apply_groups(catalog(),{{1,2},{2,1},{3,2}});
    require(d.sorted_order(Document::Sort::Group)==std::vector<SequenceId>({2,1,3,4}),"Stable group-manager order with ungrouped last");auto groups=d.groups();groups[0].name="Renamed";std::swap(groups[0],groups[1]);d.apply_groups(groups);require(d.sequence(2).groupId==1&&d.sorted_order(Document::Sort::Group)==std::vector<SequenceId>({1,3,2,4}),"Catalog order and names do not rewrite membership");
    d.rename({{1,"new title"}});d.move_rows({1},4);d.erase_rows({2});d.undo();require(d.sequence(2).groupId==1,"Deleted-row undo restores group");d.undo();d.undo();
    d.apply_groups({groups[1]});require(!d.sequence(1).groupId&&d.sequence(2).groupId==1,"Deleting group clears only its members");d.undo();require(d.groups()==groups&&d.sequence(1).groupId==2,"Undo restores deleted group and members");
});
run("Invalid catalogs, dangling references and retired IDs reject without mutation",[]{Document d;d.load({{"one","A"}});d.apply_groups(catalog(),{{1,1}});const auto state=d.state_id(),next=d.next_group_id();
    for(auto bad:std::vector<std::vector<SequenceGroup>>{{{0,"bad",0}},{{1,"same",0},{2,"same",1}},{{1,"bad\nname",0}},{{1,"bad",0x1000000}}})rejects([&]{d.apply_groups(bad);});
    rejects([&]{d.apply_groups(catalog(),{{1,3}});});rejects([&]{d.apply_groups(catalog(),{{1,1},{1,2}});});rejects([&]{d.insert_records({{"bad","A",RowKind::Nucleotide,{},RowRepresentation::AlignedSymbols,99}},0);});
    require(d.state_id()==state&&d.next_group_id()==next&&d.groups()==catalog(),"Failed validations change no state or IDs");d.apply_groups({catalog()[0]});rejects([&]{d.apply_groups(catalog());});d.undo();require(d.groups()==catalog(),"Only undo may restore retired group IDs");
});
run("TSV preview handles exact names, header, Unicode, duplicates, unmatched and conflicts",[]{Document d;d.load({{"same","A"},{"same","C"},{"name with spaces Я","G"}});
    const std::string table="\xef\xbb\xbfSequence_name\tGroup_name\r\nsame\tAlps\r\nname with spaces Я\tCaucasus\r\nmissing\tIgnored\r\n";
    auto p=plan_group_import(d,table,true,false);require(p.errors==1&&p.warnings==1&&p.assignments.size()==1&&!d.can_undo(),"Ambiguous duplicate titles block apply in a read-only preview");rejects([&]{apply_group_import(d,p);});
    p=plan_group_import(d,table,true,true);require(!p.errors&&p.assignments.size()==3&&p.groups.size()==2,"Explicit duplicate policy assigns all and creates only matched groups");apply_group_import(d,p);require(d.sequence(1).groupId==d.sequence(2).groupId&&d.sequence(1).groupId!=d.sequence(3).groupId,"Exact full titles map by stable row IDs");
    auto conflict=plan_group_import(d,"same\tAlps\nsame\tCaucasus\n",false,true);require(conflict.errors==1,"Conflicting repeated assignments reject");rejects([&]{apply_group_import(d,conflict);});
    p=plan_group_import(d,"name with spaces Я\t\n",false,false);d.rename({{3,"renamed"}});rejects([&]{apply_group_import(d,p);});d.undo();p=plan_group_import(d,"name with spaces Я\t\n",false,false);apply_group_import(d,p);require(!d.sequence(3).groupId,"Empty group explicitly means Ungrouped");
});
run("TSV cells unwrap parentheses and quoted fields before matching",[]{
    Document d;d.load({{"seq one","A"},{"seq_(copy_2)","C"},{"seq\twith tab","G"},{"seq \"quoted\"","T"},{"(a)_(b)","A"}});
    const auto p=plan_group_import(d,"(seq one)\t(Group A)\n\"seq_(copy_2)\"\t\"Group B\"\n\"seq\twith tab\"\t\"Group B\"\n\"seq \"\"quoted\"\"\"\t(Group A)\n(a)_(b)\tGroup B\n",false,false);
    require(!p.errors&&!p.warnings&&p.assignments.size()==5&&p.groups.size()==2,"Wrapped cells match actual names and normalized groups");require(p.rows[0].title=="seq one"&&p.rows[0].name=="Group A"&&p.rows[1].title=="seq_(copy_2)","Preview contains decoded cells; internal parentheses retained");
    apply_group_import(d,p);require(d.sequence(1).groupId==d.sequence(4).groupId&&d.sequence(2).groupId==d.sequence(3).groupId,"Decoded group names share memberships");d.undo();require(d.groups().empty()&&!d.can_undo(),"Wrapped import uses one Undo");d.redo();
    const auto empty=plan_group_import(d,"\"seq one\"\t\"\"\n(seq_(copy_2))\t()\n",false,false);require(!empty.errors&&empty.assignments.size()==2&&empty.assignments[0].second==0&&empty.assignments[1].second==0,"Wrapped empty groups mean Ungrouped");
    const auto bad=plan_group_import(d,"\"seq one\tG\n\"seq one\"junk\tG\n\"seq one\"\t\"bad\tgroup\"\n",false,false);require(bad.errors==3&&bad.assignments.empty(),"Malformed quotes and invalid decoded group names block Apply");rejects([&]{apply_group_import(d,bad);});
});
run("Large grouping-preview tables import through default columns without mutation",[]{
    Document d;std::vector<Record> rows;std::string table="Sequence\tProposed group\tGroup centroid\tIdentity (%)\tOverlap (%)\tResult\n";for(int i=0;i<5555;++i){const auto name="row_"+std::to_string(i);rows.push_back({name,"ACGT"});table+=name+"\tIdentity_1\trow_0\t100.00\t100.00\tGroup member\n";}d.load(std::move(rows));auto snapshot=group_import_source(d);const auto p=plan_group_import(snapshot,table,true,false);require(p.errors==0&&p.warnings==0&&p.rows.size()==5555&&p.assignments.size()==5555&&p.columnCount==6&&p.columnNames[1]=="Proposed group","Large preview table uses first two fields and retains header metadata");require(d.groups().empty()&&!d.can_undo(),"Large preview import plan is read-only");
    int checks=0;rejects([&]{plan_group_import(snapshot,table,true,false,[&]{return ++checks>20;});});require(checks>20,"Import preparation/parse cooperatively cancels");
    auto copied=group_import_source(d);d.rename({{1,"new name"}});const auto stale=plan_group_import(copied,"row_0\tNew group\n",false,false);require(stale.assignments.size()==1,"Snapshot remains independent of live row names");rejects([&]{apply_group_import(d,stale);});
});
run("Manual column mapping handles headers, quoted tabs, ignored fields and Undo",[]{
    Document d;d.load({{"alpha","AC"},{"beta_(copy)","GT"}});
    const std::string table="Group\tNotes\tSequence name\tScore\n\"Set 1\"\t\"note\twith tab\"\t(alpha)\t99\n(Set 2)\tignored\t\"beta_(copy)\"\t98\n";
    auto p=plan_group_import(d,table,true,false,{2,0});require(!p.errors&&!p.warnings&&p.assignments.size()==2&&p.columnCount==4&&p.columnNames[2]=="Sequence name","Chosen columns and decoded header metadata");require(p.rows[0].title=="alpha"&&p.rows[0].name=="Set 1"&&p.rows[1].title=="beta_(copy)","Only mapped values become assignments");apply_group_import(d,p);require(d.groups().size()==2&&d.sequence(1).groupId!=d.sequence(2).groupId,"Mapped groups applied");d.undo();require(d.groups().empty()&&!d.can_undo(),"One Undo restores mapped import");
    p=plan_group_import(d,"alpha\tGroup A\tignored\n",false,false);require(!p.errors&&p.assignments.size()==1&&p.columnCount==3,"Default remains first and second columns");
    auto missing=plan_group_import(d,"G\tnote\n",false,false,{2,0});require(missing.errors==1&&missing.assignments.empty(),"Missing selected column blocks Apply");auto same=plan_group_import(d,"alpha\tG\tmore\n",false,false,{0,0});require(same.errors==1&&same.assignments.empty(),"Same column cannot identify both sequence and group");
    auto invalid=plan_group_import(d,"alpha\tG\t\"unclosed\n",false,false);require(invalid.errors==1&&invalid.assignments.empty(),"Ignored columns still require valid TSV syntax");
    std::string tooWide="a";for(std::size_t i=0;i<group_import_max_columns;++i)tooWide+="\tx";rejects([&]{plan_group_import(d,tooWide,false,false);});
});
run("Group table bounds reject oversize input and excess data rows",[]{
    Document d;d.load({{"a","ACGT"}});rejects([&]{plan_group_import(d,std::string(group_import_max_bytes+1,'x'),false,false);});std::string many;for(std::size_t i=0;i<=group_import_max_rows;++i)many+="a\tG\n";rejects([&]{plan_group_import(d,many,false,false);});require(!d.can_undo()&&d.groups().empty(),"Limits cannot partially mutate groups");
});
run("Format 3 retains catalog/order/colors/members/evidence and reads formats 1 and 2",[]{std::istringstream ab1(test::synthetic_abif());Document d;d.load({read_abif(ab1,"trace","gone.ab1"),{"protein","ACGT",RowKind::Protein},{"mask","ACGT",RowKind::Mask}});d.apply_groups(catalog(),{{1,2},{2,1},{3,2}});std::ostringstream out(std::ios::binary);write_session(out,d);std::istringstream in(out.str(),std::ios::binary);const auto saved=read_session(in);
    require(saved.formatVersion==4&&saved.document.groups()==d.groups()&&saved.document.next_group_id()==d.next_group_id(),"Group catalog roundtrip");for(const auto& row:d.rows())require(saved.document.sequence(row.id).groupId==row.groupId&&saved.document.sequence(row.id).kind==row.kind,"Each row retains semantic metadata");require(saved.document.sequence(1).trace->original->channels==d.sequence(1).trace->original->channels,"Embedded evidence retained");
    for(const auto version:{1,2}){std::ifstream old(std::filesystem::path(__FILE__).parent_path()/("fixtures/session_v"+std::to_string(version)+".kleio"),std::ios::binary);require(bool(old),"Legacy fixture");const auto migrated=read_session(old);require(migrated.formatVersion==static_cast<unsigned>(version)&&migrated.document.groups().empty(),"Older formats migrate to Ungrouped");for(const auto& row:migrated.document.rows())require(!row.groupId,"Old rows are ungrouped");if(version==2)require(migrated.document.sequence(2).kind==RowKind::Protein&&migrated.document.sequence(3).kind==RowKind::Mask,"Version 2 retains overlapping protein and forced mask types");}
});
run("Typed clipboard subset/paste remaps collisions and undoes groups plus inserted rows once",[]{std::istringstream ab1(test::synthetic_abif());Document source;source.load({read_abif(ab1,"trace","gone.ab1"),{"protein","MEF",RowKind::Protein},{"mask","ACGT",RowKind::Mask}});source.apply_groups(catalog(),{{1,1},{2,2},{3,1}});auto part=source.subset({3,1});require(part.rows()[0].id==1&&part.rows()[1].id==3&&part.groups().size()==1,"Clipboard subset keeps alignment order and only used groups");
    Document target;target.load({{"existing","A"}});target.apply_groups({{1,"Alps",0xff0000}},{{1,1}});target.insert_document(part,0);require(target.rows().size()==3&&target.rows()[0].name=="trace"&&target.rows()[1].kind==RowKind::Mask&&target.sequence(1).groupId==1,"Paste inserts at requested position without replacing existing rows");
    require(target.groups().size()==2&&target.groups()[1].name=="Alps (2)"&&target.groups()[1].color==catalog()[0].color&&target.rows()[0].groupId==target.groups()[1].id,"Same-name/different-color groups get a distinct preserved catalog entry");
    const auto pastedId=target.rows()[0].id;require(target.rows()[0].trace==source.sequence(1).trace,"Core paste retains immutable evidence");target.undo();require(target.rows().size()==1&&target.groups().size()==1,"One undo removes pasted rows and newly imported group");target.redo();require(target.rows()[0].id==pastedId,"Redo restores same pasted IDs");target.insert_document(part,target.rows().size());require(target.groups().size()==2,"Repeat paste reuses exact matching group");
});
run("MAFFT derived results preserve groups through whole and template integrations",[]{Document d;d.load({{"one","AC"},{"two","AC"},{"mask","12",RowKind::Mask}});d.apply_groups(catalog(),{{1,1},{2,2},{3,1}});MafftAlignment alignment(d);auto result=alignment.merge({{"kleio_1","A-C"},{"kleio_2","A-C"}});auto derived=d.derive(result.records);require(derived.groups()==d.groups()&&derived.sequence(2).groupId==2&&derived.sequence(3).groupId==1,"Whole alignment retains all groups and mask memberships");
    MafftTemplateAlignment templates(d,{{2},{1},false});auto added=templates.merge({{"kleio_1","AC"},{"kleio_2","AC"}});require(d.derive(added.alignment.records).groups()==d.groups(),"Target/template result keeps catalog");result.records[0].groupId=2;rejects([&]{d.derive(result.records);});
});
run("Identical grouping replaces prior memberships atomically and separates row kinds",[]{
    Document d;d.load({{"a","aC-~"},{"b","AC-~"},{"gap order","AC~-"},{"protein1","AC-~",RowKind::Protein},{"protein2","ac-~",RowKind::Protein},{"mask","AC-~",RowKind::Mask}});
    d.apply_groups(catalog(),{{1,1},{3,2},{6,1}});const auto old=d.groups();const auto content=d.content_id();
    const auto plan=plan_identical_groups(d);require(plan.groups.size()==2&&plan.assignments.size()==6,"Two duplicate clusters; singletons ungrouped");
    d.apply_groups(plan.groups,plan.assignments);require(d.sequence(1).groupId==d.sequence(2).groupId&&d.sequence(4).groupId==d.sequence(5).groupId&&d.sequence(1).groupId!=d.sequence(4).groupId&&d.sequence(3).groupId==d.sequence(1).groupId&&!d.sequence(6).groupId,"Case insensitive, gaps ignored, kind separation");
    require(d.sequence(1).groupId>=3&&d.content_id()==content&&d.rows()[0].name=="a","Fresh group IDs without biological edits or reorder");
    d.undo();require(d.groups()==old&&d.sequence(1).groupId==1&&d.sequence(3).groupId==2&&d.sequence(6).groupId==1,"One undo restores entire old grouping");d.redo();require(d.groups()==plan.groups,"Redo restores generated grouping");
});
run("Template target reversal is permission checked and preserves original trace evidence",[]{
    std::istringstream ab1(test::synthetic_abif());Document d;d.load({read_abif(ab1,"trace","gone.ab1"),{"template","ACGTAC"},{"mask","0123456789",RowKind::Mask}});
    auto target=d.sequence(1).symbols.text();Document reversed;reversed.load({{"t",target}});reversed.transform({1},Document::Transform::ReverseComplement);const auto rc=reversed.sequence(1).symbols.text();
    const auto width=std::max(rc.size(),std::size_t(6));std::vector<Record> output={{"_R_kleio_1",rc+std::string(width-rc.size(),'-')},{"kleio_2","ACGTAC"+std::string(width-6,'-')}};
    MafftTemplateAlignment job(d,{{1},{2},false});rejects([&]{job.merge(output);});auto result=job.merge(output,{},true);
    require(result.alignment.reversed==std::vector<SequenceId>{1}&&result.alignment.records[0].trace->reversed!=d.sequence(1).trace->reversed&&result.alignment.records[0].trace->original==d.sequence(1).trace->original,"Reversal updates trace orientation, retaining immutable original");
    output[1].name="_R_kleio_2";rejects([&]{job.merge(output,{},true);});
});
run("Unlock selection changes only locked gaps inside the selected rectangle",[]{
    std::istringstream ab1(test::synthetic_abif());Document d;d.load({{"one","-A-~C--"},{"short","-G-"},{"mask","1-2-3--",RowKind::Mask},read_abif(ab1,"trace","gone.ab1")});
    d.replace(4,1,0,"-~-");const auto link=d.sequence(4).trace;const auto traceSymbols=d.sequence(4).symbols.text();
    d.unlock_fragment({1,2,4},1,4);require(d.sequence(1).symbols.text()=="-A~~C--"&&d.sequence(2).symbols.text()=="-G~"&&d.sequence(3).symbols.text()=="1-2-3--","Only selected cells change, no padding or unrelated rows");
    require(d.sequence(4).trace->original==link->original&&d.sequence(4).trace->originalCalls==link->originalCalls,"Unlock retains trace provenance and residue mapping");
    d.undo();require(d.sequence(1).symbols.text()=="-A-~C--"&&d.sequence(2).symbols.text()=="-G-"&&d.sequence(4).symbols.text()==traceSymbols,"One undo restores all selected rows");
    const auto before=d.state_id();rejects([&]{d.unlock_fragment({1,999},1,3);});require(d.state_id()==before,"Invalid row rejects atomically");
});
run("Lock selection preserves outside gaps, ragged ends and original trace evidence",[]{
    std::istringstream ab1(test::synthetic_abif());Document d;d.load({{"one","~A~~C-~"},{"short","~G~"},{"mask","1~2~3~~",RowKind::Mask},read_abif(ab1,"trace","gone.ab1")});
    d.replace(4,1,0,"~~~");const auto trace=d.sequence(4).trace;const auto traceText=d.sequence(4).symbols.text();const auto baseline=d.state_id();
    d.lock_fragment({1,2,4},1,4);require(d.sequence(1).symbols.text()=="~A--C-~"&&d.sequence(2).symbols.text()=="~G-"&&d.sequence(3).symbols.text()=="1~2~3~~","Only selected open gaps become locked; no padding or outside changes");
    require(d.sequence(4).trace->original==trace->original&&d.sequence(4).trace->originalCalls==trace->originalCalls,"Trace evidence and residue mapping unchanged");
    d.undo();require(d.state_id()==baseline&&d.sequence(1).symbols.text()=="~A~~C-~"&&d.sequence(4).symbols.text()==traceText,"One undo restores all rows");d.redo();require(d.sequence(1).symbols.text()=="~A--C-~","Redo restores lock");
    auto before=d.state_id();d.lock_fragment({1,2,4},1,4);require(d.state_id()==before,"Locking an already locked span adds no history");rejects([&]{d.lock_fragment({1,999},1,3);});require(d.state_id()==before,"Invalid selection cannot partially lock");
});
std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;}
