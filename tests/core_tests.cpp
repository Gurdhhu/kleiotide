#include "core/alignment.hpp"
#include "core/fasta.hpp"
#include "core/overview.hpp"
#include "core/mafft_alignment.hpp"
#include "core/mafft_template.hpp"
#include "core/statistics.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace kleiotide;
namespace {
void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F&& action) {
    bool caught = false;
    try { action(); } catch (const std::exception&) { caught = true; }
    require(caught, "Expected the operation to reject invalid input");
}
std::string snapshot(const Document& d) {
    std::ostringstream out;
    for (const auto& r : d.rows()) out << r.id << ':' << static_cast<int>(r.kind) << ':' << r.name << ':' << r.symbols.text() << '\n';
    return out.str();
}
std::vector<Record> parse(const std::string& text) {
    std::istringstream in(text);
    return read_fasta(in);
}
}
int main() {
    int passed = 0, failed = 0;
    auto test = [&](const char* name, const std::function<void()>& action) {
        try { action(); ++passed; std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { ++failed; std::cerr << "FAIL " << name << ": " << e.what() << '\n'; }
    };
    test("Undo and redo identify residue destinations without metadata jumps", [] {
        Document d;d.load({{"first","AAAACGTT"},{"second","TTTT"}});const auto id=d.rows()[0].id;
        require(!d.undo_residue_location()&&!d.redo_residue_location(),"Empty history has no location");
        d.replace(id,0,8,"AAAATGTT");auto location=d.undo_residue_location();
        require(location&&location->id==id&&location->start==4&&location->width==1,"Skip unchanged prefix and suffix of whole-row replacement");
        d.undo();location=d.redo_residue_location();require(location&&location->start==4&&location->width==1,"Redo destination");d.redo();
        d.replace(id,8,0,"AC");location=d.undo_residue_location();require(location&&location->start==8&&location->width==1,"Undo insertion at sequence end still reveals boundary");d.undo();
        location=d.redo_residue_location();require(location&&location->width==2,"Redo insertion reveals inserted residues");
        d.replace_many({{id,2,3,""},{d.rows()[1].id,1,1,"A"}});location=d.undo_residue_location();require(location&&location->id==id&&location->start==2&&location->width==3,"Batch deletion locates restored span");d.undo();
        d.rename({{id,"renamed"}});require(!d.undo_residue_location(),"Rename cannot move viewport");d.undo();require(!d.redo_residue_location(),"Redo rename cannot move viewport");
        d.reorder({d.rows()[1].id,id});require(!d.undo_residue_location(),"Reorder has no residue target");d.undo();
        d.apply_groups({{1,"group",0x0072b2}},{{id,1}});require(!d.undo_residue_location(),"Grouping has no residue target");
    });
    test("Rigid fragments preserve internal gaps and consume open gaps", [] {
        Document d; d.load({{"a", "A~~~C-G~~T"}, {"b", "A~~~~~~G~T"}});
        const auto a=d.rows()[0].id; const auto original=snapshot(d);
        require(d.slide({a},4,3,2)==2, "Consume adjacent gaps");
        require(d.sequence(a).symbols.text()=="A~~~~~C-GT", "Internal locked dash moves with fragment");
        d.undo(); require(snapshot(d)==original, "Slide undo"); d.redo();
        require(d.sequence(a).symbols.text()=="A~~~~~C-GT", "Slide redo");
        d.undo(); require(d.slide({a},4,3,-99)==-3, "Stop at upstream A");
        require(d.sequence(a).symbols.text()=="AC-G~~~~~T", "Left block");
        d.load({{"a", "A~~-CG~~T"}});
        require(d.slide({d.rows()[0].id},4,2,-2)==-2, "Push external locked gap");
        require(d.rows()[0].symbols.text()=="A-CG~~~~T", "Locked gap preserved and shifted");
    });
    test("Reference 16-column fragment slides 30 columns to downstream CAA", [] {
        const std::string block = "CG~G~GCT~GGG~~GG";
        require(block.size() == 16, "Fixture width");
        const std::string text = std::string(757, '~') + block + std::string(30, '~') + "CAA";
        Document d; d.load({{"reference", text}}); const auto id = d.rows()[0].id;
        require(d.slide({id},757,16,d.adjacent_gap_distance({id},757,16,10000)) == 30, "758-773 becomes 788-803");
        require(d.sequence(id).symbols.text().substr(787,16) == block && d.sequence(id).symbols.text().substr(803)=="CAA", "Downstream unchanged");
        d.undo(); require(d.sequence(id).symbols.text() == text, "Reference undo");
    });
    test("Grouped pushing keeps common displacement and leaves other rows unchanged", [] {
        Document d;d.load({{"a","A~~CG~~~T"},{"b","A~~CG~T~~"},{"c","acgt"}});
        const auto a=d.rows()[0].id,b=d.rows()[1].id;
        auto before=snapshot(d);require(d.slide({a,b},3,2,3)==3,"Group shift");
        require(d.sequence(a).symbols.text()=="A~~~~~CGT" && d.sequence(b).symbols.text()=="A~~~~~CGT", "Same displacement");
        require(d.rows()[2].symbols.text()=="acgt", "Unselected row");d.undo();require(snapshot(d)==before,"Atomic group undo");
    });
    test("Fragment fill and virtual ragged cells are transactional", [] {
        Document d;d.load({{"a","ACGT~~~~"},{"b","T"}});auto before=snapshot(d);
        const auto a=d.rows()[0].id,b=d.rows()[1].id;
        d.fill_fragment({a,b},3,3,'N');
        require(d.sequence(a).symbols.text()=="ACGNNN~~" && d.sequence(b).symbols.text()=="T~~NNN","Virtual padding");
        d.undo();require(snapshot(d)==before,"Fill undo");
        require(d.slide({b},0,1,7)==7,"Slide across virtual gaps");
        require(d.sequence(b).symbols.text()=="~~~~~~~T","Tail materialized");d.undo();require(snapshot(d)==before,"Ragged undo");
        rejects([&]{d.fill_fragment({a,99999},1,1,'N');});require(snapshot(d)==before,"Unknown ID no partial edit");
        rejects([&]{d.replace_many({{a,0,1,"T"},{b,5,1,"C"}});});require(snapshot(d)==before,"Invalid range no partial edit");
    });
    test("Right-click gap operations never delete residues or locked dashes", [] {
        Document d;d.load({{"a","A~C"},{"b","A-C"},{"c","ATC"},{"short","A"}});
        std::vector<SequenceId> ids;for(const auto& r:d.rows())ids.push_back(r.id);
        const auto before=snapshot(d);d.edit_gaps(ids,1,false);
        require(d.rows()[0].symbols.text()=="AC" && d.rows()[1].symbols.text()=="A-C" && d.rows()[2].symbols.text()=="ATC","Only ~ removed");
        d.undo();require(snapshot(d)==before,"Batch gap undo");d.edit_gaps(ids,2,true);
        require(d.rows()[0].symbols.text()=="A~~C" && d.rows()[3].symbols.text()=="A~~","Insert and ragged padding");
        d.undo();require(snapshot(d)==before,"Insert undo");
    });
    test("Gap transforms, case, degap and unalign preserve locked distinction", [] {
        Document d;d.load({{"a","aC~-g"},{"b","A"}});auto id=d.rows()[0].id;auto before=snapshot(d);
        d.transform({id},Document::Transform::LockGaps);require(d.sequence(id).symbols.text()=="aC--g","Lock");d.undo();
        d.transform({id},Document::Transform::UnlockGaps);require(d.sequence(id).symbols.text()=="aC~~g","Unlock");d.undo();
        d.transform({id},Document::Transform::Degap);require(d.sequence(id).symbols.text()=="aC-g","Keep locked gap");d.undo();
        d.transform({id},Document::Transform::Unalign);require(d.sequence(id).symbols.text()=="aCg","Remove both");d.undo();
        d.transform({id},Document::Transform::Uppercase);require(d.sequence(id).symbols.text()=="AC~-G","Upper");d.undo();
        d.transform({id},Document::Transform::Lowercase);require(d.sequence(id).symbols.text()=="ac~-g","Lower");d.undo();
        require(snapshot(d)==before,"Transform undo");
    });
    test("Reverse complement preserves case, IUPAC, RNA and gap types", [] {
        Document d;d.load({{"DNA","AaCGTRYKMSWBDHVN~-?"},{"RNA","aUgc"}});
        auto before=snapshot(d);std::vector<SequenceId> ids;for(const auto& r:d.rows())ids.push_back(r.id);
        d.transform(ids,Document::Transform::ReverseComplement);
        require(d.rows()[0].symbols.text()=="?-~NBDHVWSKMRYACGtT","DNA complement");
        require(d.rows()[1].symbols.text()=="gcAu","RNA complement");
        d.transform(ids,Document::Transform::ReverseComplement);require(snapshot(d)==before,"Involution");
    });
    test("Minimize removes only columns with no residue in any row", [] {
        Document d;d.load({{"a","~A--C~~"},{"b","--G~~~T"},{"ragged","~T"}});auto before=snapshot(d);
        d.minimize();require(d.rows()[0].symbols.text()=="A-C~" && d.rows()[1].symbols.text()=="-G~T" && d.rows()[2].symbols.text()=="T","Global support, ragged lengths");
        d.undo();require(snapshot(d)==before,"Minimize undo");
    });
    test("Right slide pushes locked gaps and residues, consuming downstream gaps", [] {
        Document d;d.load({{"a","A~CG-T~G~~A"}});const auto id=d.rows()[0].id;const auto before=snapshot(d);
        require(d.slide({id},2,2,3)==3,"Full displacement");
        require(d.rows()[0].symbols.text()=="A~~~~CG-TGA","Cascade through locks and residues");
        d.undo();require(snapshot(d)==before,"Exact cascade undo");d.redo();
        require(d.rows()[0].symbols.text()=="A~~~~CG-TGA","Cascade redo");
    });
    test("Left slide consumes upstream open gaps and pushes intervening content", [] {
        Document d;d.load({{"a","A~B-~C~~TG-T"}});const auto id=d.rows()[0].id;
        require(d.slide({id},8,2,-3)==-3,"Left push");
        require(d.rows()[0].symbols.text()=="A~B-CTG~~~-T","Nearest upstream gaps consumed");
        d.undo();require(d.slide({id},8,2,-100)==-4,"Leading content bounds movement");
        require(d.rows()[0].symbols.text()=="AB-CTG~~~~-T","No lost leading residues/locks");
    });
    test("Right slide extends alignment and preserves all unselected content", [] {
        Document d;d.load({{"a","AC-GT"},{"b","TT"}});const auto before=snapshot(d);const auto id=d.rows()[0].id;
        require(d.slide({id},1,2,6)==6,"Move past old boundary");
        require(d.rows()[0].symbols.text()=="A~~~~~~C-GT"&&d.columns()==11,"Alignment extended");
        require(d.rows()[1].symbols.text()=="TT","Other row unchanged");d.undo();require(snapshot(d)==before,"Growth undo");
        rejects([&]{d.slide({id},1,2,std::numeric_limits<std::int64_t>::max());});require(snapshot(d)==before,"Oversized request rejected");
    });
    test("Group left slide is limited only by fit at column one", [] {
        Document d;d.load({{"a","A~~~CGT"},{"b","AB~-CGT"}});std::vector<SequenceId> ids;for(const auto&r:d.rows())ids.push_back(r.id);
        require(d.slide(ids,4,2,-3)==-1,"One available gap in second prefix");
        require(d.rows()[0].symbols.text()=="A~~CG~T"&&d.rows()[1].symbols.text()=="AB-CG~T","Uniform group push");
    });
    test("Random pushing agrees with independent single-step gap edits", [] {
        std::mt19937 rng(9021);
        for(int run=0;run<300;++run){
            Document d;std::vector<Record> rows;
            for(int r=0;r<4;++r){std::string text;for(int c=0;c<40;++c)text += "ACGT~~~--"[rng()%9];rows.push_back({std::to_string(r),text});}
            d.load(rows);const auto before=snapshot(d);auto expected=rows;
            const auto start=static_cast<std::size_t>(rng()%30),width=static_cast<std::size_t>(rng()%10+1);
            const int requested=static_cast<int>(rng()%41)-20;auto at=start;int expectedMove=0;
            for(int n=0;n<std::abs(requested);++n){
                if(requested<0){bool fits=at>0;for(const auto&r:expected)if(at==0||r.symbols.rfind('~',at-1)==std::string::npos)fits=false;if(!fits)break;}
                for(auto&r:expected){
                    if(requested>0){auto gap=r.symbols.find('~',at+width);if(gap!=std::string::npos)r.symbols.erase(gap,1);r.symbols.insert(at,1,'~');}
                    else{auto gap=r.symbols.rfind('~',at-1);r.symbols.erase(gap,1);r.symbols.insert(at+width-1,1,'~');}
                }
                if(requested>0){++at;++expectedMove;}else{--at;--expectedMove;}
            }
            std::vector<SequenceId> ids;for(const auto&row:d.rows())ids.push_back(row.id);
            require(d.slide(ids,start,width,requested)==expectedMove,"Displacement");
            for(std::size_t r=0;r<rows.size();++r)require(d.rows()[r].symbols.text()==expected[r].symbols,"Independent push model");
            if(d.can_undo()){d.undo();require(snapshot(d)==before,"Exact undo");}
        }
    });
    test("Rename validation, redo and mixed editing preserve exact document", [] {
        Document d; d.load({{"one", "a~C-"}, {"two", "GT"}});
        const auto first = d.rows()[0].id, second = d.rows()[1].id;
        const auto original = snapshot(d);
        const auto state = d.state_id();
        rejects([&] { d.rename({{first, "valid"}, {second, " \t"}}); });
        rejects([&] { d.rename({{first, "valid"}, {999999, "unknown"}}); });
        rejects([&] { d.rename({{first, "valid"}, {first, "repeat"}}); });
        rejects([&] { d.rename({{first, "bad\nheader"}}); });
        require(snapshot(d) == original && d.state_id() == state && !d.can_undo(), "Atomic rejection");
        d.rename({{first, "one"}});
        require(d.state_id() == state && !d.can_undo(), "No-op rename");
        d.rename({{first, "Образец α"}, {second, "Образец α"}});
        const auto renamed = snapshot(d);
        require(d.sequence(first).symbols.residue_count() == 2, "Residue coordinates unchanged");
        d.replace(second, 0, 1, "N");
        const auto edited = snapshot(d);
        d.undo(); require(snapshot(d) == renamed, "Undo subsequent edit");
        d.undo(); require(snapshot(d) == original, "Undo rename");
        d.redo(); require(snapshot(d) == renamed, "Redo rename");
        d.redo(); require(snapshot(d) == edited, "Redo subsequent edit");
    });
    test("FASTA preserves full headers, case, duplicate names and ragged lengths", [] {
        auto r = parse(">same description\nacGT--~N\n>same description\nAU\n");
        require(r.size() == 2 && r[0].name == r[1].name, "Headers");
        require(r[0].symbols == "acGT--~N" && r[1].symbols == "AU", "Symbols");
    });
    test("BOM, CRLF, wrapped lines and whitespace", [] {
        auto r = parse("\xEF\xBB\xBF>name\r\n A C\tG\r\nT.\r\n");
        require(r[0].symbols == "ACGT~", "Canonical sequence");
    });
    test("Unicode headers and empty sequences", [] {
        auto r = parse(">Образец α\n>other\nN\n");
        require(r[0].name == "Образец α" && r[0].symbols.empty(), "Header bytes");
    });
    test("FASTA unexpected printable symbols classify the whole row as a mask", [] {
        auto records=parse(">DNA\nACGRYN\n>mask\nAC\nE012*\n");
        require(records[0].kind==RowKind::Nucleotide&&records[1].kind==RowKind::Mask&&records[1].symbols=="ACE012*","Whole-row mask classification");
        rejects([]{parse(std::string(">bad\nAC")+char(0)+"G\n");});
    });
    test("FASTA rejects empty input, empty names and sequence before header", [] {
        rejects([] { (void)parse(""); });
        rejects([] { (void)parse(">   \nA"); });
        rejects([] { (void)parse("ACGT\n"); });
    });
    test("FASTA canonical round trip including open gaps", [] {
        Document d; d.load(parse(">x\naN~-\n>x\n\n"));
        std::ostringstream out; write_fasta(out, d, 2);
        auto r = parse(out.str());
        require(r[0].symbols == "aN~-" && r[1].symbols.empty(), "Round trip");
        std::ostringstream plain;write_fasta(plain,d,0);
        require(plain.str()==">x\naN~-\n>x\n\n","Zero width is unwrapped, including empty rows");
    });
    test("Unique stable IDs with duplicate display names", [] {
        Document d; d.load({{"same", "A"}, {"same", "G"}});
        require(d.rows()[0].id != d.rows()[1].id, "Distinct IDs");
    });
    test("Default FASTA file and record output are unwrapped with arbitrary wrapped input", [] {
        const auto sequence=std::string(241,'a')+"RYN-~";Document d;
        d.load(parse(">long\r\n"+sequence.substr(0,1)+"\r\n\n"+sequence.substr(1,79)+"\n"+sequence.substr(80)+"\n>empty\n\n>mask\n012 34\n"));
        std::ostringstream out;write_fasta(out,d);const auto expected=">long\n"+sequence+"\n>empty\n\n>mask\n01234\n";
        require(out.str()==expected,"Exactly one sequence line per header by default");
        std::ostringstream record;write_fasta_record(record,d.sequence(1).name,d.sequence(1).symbols.text());
        require(record.str()==">long\n"+sequence+"\n","Clipboard and file share the record writer");
        require(parse(out.str())[0].symbols==sequence,"Unwrapped input rereads without changes");
        std::ostringstream broken;broken.setstate(std::ios::badbit);rejects([&]{write_fasta(broken,d);});
    });
    test("Invalid baseline load preserves the existing document", [] {
        Document d; d.load({{"existing", "AC"}});
        auto before = snapshot(d);
        auto state = d.state_id();
        rejects([&] { d.load({{"ok", "AC"}, {"   ", "A"}}); });
        rejects([&] { d.load({{"bad\nname", "A"}}); });
        rejects([&] { d.load({{std::string("bad\0name", 8), "A"}}); });
        require(snapshot(d) == before && d.state_id() == state, "Preserve baseline");
    });
    test("Insert/overwrite/delete and exact undo/redo", [] {
        Document d; d.load({{"x", "ACGT"}});
        const auto id = d.rows()[0].id, saved = d.state_id();
        d.replace(id, 2, 0, "~N");
        require(d.sequence(id).symbols.text() == "AC~NGT", "Insert");
        d.replace(id, 0, 1, "t");
        d.replace(id, 1, 2, "");
        require(d.sequence(id).symbols.text() == "tNGT", "Delete");
        d.undo(); d.undo(); d.undo();
        require(d.sequence(id).symbols.text() == "ACGT" && d.state_id() == saved, "Undo baseline");
        d.redo(); d.redo(); d.redo();
        require(d.sequence(id).symbols.text() == "tNGT", "Redo");
    });
    test("Invalid and no-op edits do not alter state or history", [] {
        Document d; d.load({{"x", "AC"}});
        const auto id = d.rows()[0].id, state = d.state_id();
        d.replace(id, 0, 1, "A");
        rejects([&] { d.replace(id, 0, 1, "E"); });
        rejects([&] { d.replace(id, 3, 0, "A"); });
        rejects([&] { d.replace(id, 1, 2, "A"); });
        require(d.state_id() == state && !d.can_undo(), "Unchanged state");
    });
    test("Edit after undo drops redo branch", [] {
        Document d; d.load({{"x", "A"}});
        auto id = d.rows()[0].id;
        d.replace(id, 1, 0, "C"); d.undo(); d.replace(id, 1, 0, "G");
        require(!d.can_redo() && d.sequence(id).symbols.text() == "AG", "Branch");
    });
    test("Append is atomic and undo retains stable IDs", [] {
        Document d; d.load({{"x", "A"}});
        auto before = snapshot(d);
        rejects([&] { d.append({{"valid", "AC"}, {"bad", std::string(1,char(0))}}); });
        require(snapshot(d) == before, "Atomic validation");
        d.append({{"x", "G"}, {"new", "N"}});
        auto after = snapshot(d);
        d.undo(); require(snapshot(d) == before, "Undo append");
        d.redo(); require(snapshot(d) == after, "Redo append");
    });
    test("Noncontiguous row deletion preserves order across undo/redo", [] {
        Document d; d.load({{"a","A"},{"b","C"},{"c","G"},{"d","T"},{"e","N"}});
        const auto before = snapshot(d);
        auto b = d.rows()[1].id, last = d.rows()[4].id;
        d.erase_rows({last, b, b});
        require(d.rows().size() == 3 && d.rows()[1].name == "c", "Delete");
        const auto after = snapshot(d);
        d.undo(); require(snapshot(d) == before, "Restore exact IDs/order");
        d.redo(); require(snapshot(d) == after, "Delete again");
        rejects([&] { d.erase_rows({999999}); });
    });
    test("Delete all rows and recover them", [] {
        Document d; d.load({{"a","A"},{"b","C"}});
        auto before = snapshot(d);
        d.erase_rows({d.rows()[0].id, d.rows()[1].id});
        require(d.rows().empty() && d.columns() == 0, "Empty document");
        d.undo(); require(snapshot(d) == before, "Restore all");
    });
    test("Flush materializes terminal gaps in one undoable transaction", [] {
        Document d; d.load({{"a","ACG"},{"b","a"},{"c",""}});
        const auto before = snapshot(d);
        d.flush();
        require(d.rows()[1].symbols.text() == "a~~" && d.rows()[2].symbols.text() == "~~~", "Padding");
        d.undo(); require(snapshot(d) == before && !d.can_undo(), "Single undo");
        d.redo(); require(d.rows()[1].symbols.text() == "a~~", "Redo flush");
        rejects([&] { d.flush('N'); });
    });
    test("Non-gap rank index at block boundaries and after edits", [] {
        Document d;
        std::string s(2500, 'A');
        for (std::size_t i = 0; i < s.size(); i += 3) s[i] = '~';
        d.load({{"x",s}});
        auto id = d.rows()[0].id;
        auto check = [&] {
            const auto& b = d.sequence(id).symbols;
            std::size_t count = 0;
            for (std::size_t i = 0; i <= b.size(); ++i) {
                require(b.residue_rank(i) == count, "Rank at " + std::to_string(i));
                if (i < b.size()) count += !is_gap(b.text()[i]);
            }
            require(b.residue_count() == count, "Total count");
        };
        check(); d.replace(id, 1020, 8, "a--~N"); check(); d.undo(); check();
        rejects([&] { (void)d.sequence(id).symbols.residue_rank(99999); });
    });
    test("History budget evicts old entries but preserves latest operation", [] {
        Document d; d.load({{"x","A"}}); d.set_history_budget(1);
        auto id = d.rows()[0].id;
        d.replace(id, 1, 0, "C"); d.replace(id, 2, 0, "G");
        d.undo();
        require(d.sequence(id).symbols.text() == "AC" && !d.can_undo(), "Latest retained");
        d.redo(); require(d.sequence(id).symbols.text() == "ACG", "Latest redo");
    });
    test("Random edit history agrees with independent string operations", [] {
        Document d; d.load({{"x", "ACGT"}});
        auto id = d.rows()[0].id;
        std::mt19937 rng(1234);
        std::vector<std::string> states{"ACGT"};
        for (int i = 0; i < 1000; ++i) {
            auto next = states.back();
            const auto start = static_cast<std::size_t>(rng()) % (next.size() + 1);
            const auto count = next.empty() ? 0 : static_cast<std::size_t>(rng()) % (next.size() - start + 1);
            std::string replacement;
            for (unsigned j = 0, n = rng() % 5; j < n; ++j) replacement += "AcGTN~-?"[rng() % 8];
            auto prior = next.substr(start, count);
            if (prior == replacement) continue;
            next.replace(start, count, replacement);
            d.replace(id, start, count, replacement);
            require(d.sequence(id).symbols.text() == next, "Independent result");
            states.push_back(std::move(next));
        }
        for (std::size_t i = states.size() - 1; i > 0; --i) {
            d.undo(); require(d.sequence(id).symbols.text() == states[i - 1], "Random undo");
        }
        for (std::size_t i = 1; i < states.size(); ++i) {
            d.redo(); require(d.sequence(id).symbols.text() == states[i], "Random redo");
        }
    });
    test("Random append/delete/edit/flush transactions undo exact rows and IDs", [] {
        Document d; d.load({{"initial","ACGT"}});
        std::mt19937 rng(42);
        std::vector<std::string> states{snapshot(d)};
        for (int i = 0; i < 250; ++i) {
            auto before = d.state_id();
            switch (rng() % 4) {
            case 0: d.append({{"duplicate","AcG~N"}}); break;
            case 1:
                if (!d.rows().empty()) {
                    std::vector<SequenceId> ids;
                    for (const auto& row : d.rows()) if (rng() % 3 == 0) ids.push_back(row.id);
                    d.erase_rows(ids);
                }
                break;
            case 2:
                if (!d.rows().empty()) {
                    const auto& r = d.rows()[rng() % d.rows().size()];
                    d.replace(r.id, r.symbols.size(), 0, "A");
                }
                break;
            case 3: d.flush(); break;
            }
            if (d.state_id() != before) states.push_back(snapshot(d));
        }
        for (std::size_t i = states.size() - 1; i > 0; --i) {
            d.undo(); require(snapshot(d) == states[i - 1], "Transaction undo");
        }
        for (std::size_t i = 1; i < states.size(); ++i) {
            d.redo(); require(snapshot(d) == states[i], "Transaction redo");
        }
    });
    test("Content revisions distinguish residues from renaming and restore on undo", [] {
        Document d;d.load({{"x","AC"}});auto id=d.rows()[0].id;const auto original=d.content_id();
        d.rename({{id,"renamed"}});require(d.content_id()==original,"Name keeps content revision");
        d.undo();require(d.content_id()==original,"Name undo keeps content revision");d.redo();
        d.replace(id,1,0,"G");const auto edited=d.content_id();require(edited!=original,"Edit changes revision");
        d.undo();require(d.content_id()==original,"Edit undo restores revision");d.redo();require(d.content_id()==edited,"Redo restores revision");
        d.undo();d.replace(id,1,0,"T");require(d.content_id()!=edited&&d.content_id()!=original,"Branch is unique");
        const auto branch=d.content_id();d.replace(id,0,1,"A");require(d.content_id()==branch,"No-op keeps revision");
    });
    test("Overview residue fractions include ragged cells while N is uninformative", [] {
        Document d;d.load({{"one","aCUN~~"},{"two","ACtA-"},{"three","AGtN"},{"empty",""}});
        OverviewScan scan(d);require(!scan.step(d,0)&&scan.progress()==0,"Zero budget");rejects([&]{scan.bins(6);});
        while(!scan.step(d,2)){}require(scan.progress()==1,"Complete progress");const auto bins=scan.bins(100);
        require(bins.size()==6,"No redundant bins");
        auto near=[](double a,double b){return std::abs(a-b)<1e-12;};
        require(near(bins[0].residues,.75)&&near(bins[0].conservation,1),"Case insensitive A");
        require(near(bins[1].conservation,2.0/3)&&near(bins[2].conservation,1),"Mixed C/G and U/T");
        require(near(bins[3].conservation,1)&&near(bins[3].residues,.75),"N does not reduce conservation");
        require(bins[4].residues==0&&bins[4].conservation==0&&!bins[4].hasInformation,"All gaps neutral");
        require(bins[5].residues==0&&!bins[5].hasInformation,"Missing cells are gaps");
        const auto compressed=scan.bins(2);require(compressed[0].begin==0&&compressed[0].end==3&&near(compressed[0].conservation,8.0/9),"Compression means");
        require(scan.bins(0).empty(),"Zero bins");Document empty;OverviewScan zero(empty);require(zero.complete()&&zero.bins(5).empty(),"Empty document");
    });
    test("N and gaps do not dilute color within compressed overview bins", [] {
        Document d;d.load({{"one","AN~NAR?"},{"two","AN-NNN?"},{"three","AN~NCNN"},{"four","NN-NCNN"}});
        OverviewScan scan(d);while(!scan.step(d,3)){}const auto columns=scan.bins(7);
        require(columns[0].conservation==1&&columns[0].hasInformation,"A with N remains conserved");
        require(columns[1].residues==1&&!columns[1].hasInformation,"All N has full height and no information");
        require(columns[2].residues==0&&!columns[2].hasInformation,"All gaps have zero height");
        require(std::abs(columns[4].conservation-2.0/3)<1e-12,"Mixed calls ignore N");
        require(columns[5].hasInformation&&columns[5].conservation==.5&&!columns[6].hasInformation,"R contributes fractional information");
        const auto one=scan.bins(1).front();require(std::abs(one.conservation-13.0/18)<1e-12,"Only informative columns averaged");
    });
    test("Overview cooperative scans reject changed or different sources", [] {
        Document d;d.load({{"x","ACGT"}});OverviewScan scan(d);scan.step(d,1);
        auto id=d.rows()[0].id;d.rename({{id,"new"}});require(scan.step(d,20),"Rename allowed during scan");
        OverviewScan stale(d);stale.step(d,1);d.replace(id,1,0,"N");rejects([&]{stale.step(d,20);});
        Document other;other.load({{"x","ACGT"}});rejects([&]{scan.step(other,20);});
    });
    test("Random overview bins agree with an independent column model", [] {
        std::mt19937 rng(1701);
        for(int trial=0;trial<100;++trial){
            std::vector<Record> rows;for(unsigned r=0,n=1+rng()%12;r<n;++r){std::string s;for(unsigned c=0,m=rng()%70;c<m;++c)s+="AaCcGgTtUuNnR?~-"[rng()%16];rows.push_back({std::to_string(r),s});}
            Document d;d.load(rows);OverviewScan scan(d);while(!scan.step(d,1+rng()%17)){}
            const auto bins=scan.bins(1+rng()%15);
            for(const auto& bin:bins){double residues=0,conservation=0;unsigned informative=0;
                for(auto col=bin.begin;col<bin.end;++col){unsigned nongap=0;unsigned counts[4]={};
                    for(const auto& r:rows){if(col>=r.symbols.size()||r.symbols[col]=='~'||r.symbols[col]=='-')continue;++nongap;auto c=r.symbols[col];if(c>='a'&&c<='z')c-=32;auto at=std::string("ACGT").find(c=='U'?'T':c);if(at<4)counts[at]+=6;else if(c=='R'){counts[0]+=3;counts[2]+=3;}}
                    residues+=static_cast<double>(nongap)/rows.size();const auto calls=counts[0]+counts[1]+counts[2]+counts[3];if(calls){++informative;conservation+=static_cast<double>(*std::max_element(counts,counts+4))/calls;}
                }
                const auto expected=informative?conservation/informative:0;
                require(std::abs(bin.residues-residues/(bin.end-bin.begin))<1e-12&&std::abs(bin.conservation-expected)<1e-12&&bin.hasInformation==(informative>0),"Independent statistics");
            }
        }
    });
    test("IUPAC two/three-base votes and masks preserve overview meaning", [] {
        Document d;d.load({{"one","ARRRRYSWKMBDHVN?"},{"two","AGRNNYSWKMBDHVN?"},{"mask","0123456789ABCDEF"}});
        OverviewScan scan(d);while(!scan.step(d,5)){}const auto b=scan.bins(100);
        require(b[0].residues==1&&b[0].conservation==1,"Mask excluded from height and calls");
        require(b[1].conservation==.75&&b[2].conservation==.5&&b[3].conservation==.5&&b[4].conservation==.5,"R with G/R/N");
        for(std::size_t i=5;i<10;++i)require(b[i].conservation==.5,"Two-base calls");
        for(std::size_t i=10;i<14;++i)require(std::abs(b[i].conservation-1.0/3)<1e-12,"Three-base calls");
        require(!b[14].hasInformation&&!b[15].hasInformation,"N/unknown absent information");
        const std::string symbols="ACGTURYSWKMBDHVN";const std::vector<std::string> sets={"A","C","G","T","T","AG","CT","CG","AT","GT","AC","CGT","AGT","ACT","ACG",""};
        for(std::size_t i=0;i<symbols.size();++i){const auto votes=nucleotide_votes(symbols[i]);for(std::size_t at=0;at<4;++at)require(votes[at]==(sets[i].find("ACGT"[at])==std::string::npos?0:6/sets[i].size()),"Exact IUPAC mapping");}
        Document masks;masks.load({{"mask","0123"}});OverviewScan none(masks);while(!none.step(masks,2)){}for(const auto& bin:none.bins(10))require(bin.residues==0&&!bin.hasInformation,"Mask-only overview empty");
    });
    test("Masks edit, slide, transform and save without becoming nucleotides", [] {
        Document d;d.load(parse(">DNA\nA~CG~~T\n>mask\n1~23~~4\n"));const auto dna=d.rows()[0].id,mask=d.rows()[1].id;const auto before=snapshot(d);
        require(d.nucleotide_ids()==std::vector<SequenceId>{dna},"Alignment eligibility excludes masks");
        require(d.slide({dna,mask},2,2,2)==2&&d.sequence(mask).symbols.text()=="1~~~234","Mixed block slide");d.undo();require(snapshot(d)==before,"Mixed slide exact undo");
        d.replace(mask,2,2,"AC");require(d.sequence(mask).kind==RowKind::Mask,"Mask kind persists after DNA edit");d.undo();
        d.replace(mask,2,0,"*xy9");require(d.sequence(mask).symbols.text()=="1~*xy923~~4","Mask insertion");d.undo();
        d.replace_many({{dna,0,1,"T"},{mask,0,1,"#"}});d.undo();require(snapshot(d)==before,"Mixed transaction undo");
        d.transform({dna,mask},Document::Transform::ReverseComplement);require(d.sequence(mask).symbols.text()=="1~23~~4","RC skips mask");d.undo();require(snapshot(d)==before,"RC undo");
        d.erase_rows({mask});d.undo();require(snapshot(d)==before,"Mask deletion undo keeps type");d.redo();d.undo();
        std::ostringstream output;write_fasta(output,d);Document reopened;reopened.load(parse(output.str()));require(reopened.rows()[1].kind==RowKind::Mask&&reopened.rows()[1].symbols.text()=="1~23~~4","Mask FASTA round trip");
    });
    test("Row permutations validate before mutation and compose with edit/delete history", [] {
        Document d;d.load({{"a","AC"},{"b","AC"},{"mask","012"},{"c","G"}});const auto before=snapshot(d);auto order=d.sorted_order(Document::Sort::Reverse);
        rejects([&]{d.reorder({order[0],order[0],order[2],order[3]});});rejects([&]{d.reorder({999,order[1],order[2],order[3]});});require(snapshot(d)==before&&!d.can_undo(),"Atomic ordering rejection");
        d.reorder(order);const auto reversed=snapshot(d);require(d.rows()[0].id==order[0]&&d.history_bytes()<1000,"Compact row-order history");
        d.replace(order[0],0,1,"T");const auto edited=snapshot(d);d.erase_rows({order[1]});const auto deleted=snapshot(d);
        d.undo();require(snapshot(d)==edited,"Delete undo");d.undo();require(snapshot(d)==reversed,"Edit undo");d.undo();require(snapshot(d)==before,"Order undo");
        d.redo();d.redo();d.redo();require(snapshot(d)==deleted,"Composed redo");
        const auto state=d.state_id();std::vector<SequenceId> same;for(const auto& r:d.rows())same.push_back(r.id);d.reorder(same);require(d.state_id()==state,"No-op ordering");
    });
    test("Noncontiguous row moves preserve selected and unselected relative order", [] {
        Document d;d.load({{"a","A"},{"b","C"},{"c","G"},{"d","T"},{"e","N"}});std::vector<SequenceId> ids;for(const auto& r:d.rows())ids.push_back(r.id);const auto before=snapshot(d);
        d.move_rows({ids[3],ids[1]},5);require(d.rows()[0].id==ids[0]&&d.rows()[1].id==ids[2]&&d.rows()[2].id==ids[4]&&d.rows()[3].id==ids[1]&&d.rows()[4].id==ids[3],"Move down in original order");d.undo();
        d.move_rows({ids[1],ids[3]},0);require(d.rows()[0].id==ids[1]&&d.rows()[1].id==ids[3]&&d.rows()[2].id==ids[0],"Move to beginning");d.undo();require(snapshot(d)==before,"Move undo");
        const auto state=d.state_id();d.move_rows({ids[1],ids[2]},2);require(d.state_id()==state,"Drop inside contiguous group is no-op");rejects([&]{d.move_rows({ids[0]},6);});rejects([&]{d.move_rows({999},0);});
    });
    test("All row sort methods have stable ties and specified ordering", [] {
        Document d;d.load({{"z","AAA~~"},{"b","CC"},{"A","aaa~~"},{"a","A--"},{"x","G"}});
        auto check=[&](Document::Sort method,std::vector<std::size_t> expected){auto order=d.sorted_order(method);for(std::size_t i=0;i<order.size();++i)require(order[i]==d.rows()[expected[i]].id,"Sort order");};
        check(Document::Sort::Title,{2,3,1,4,0});check(Document::Sort::LengthDescending,{0,2,3,1,4});check(Document::Sort::LengthAscending,{4,1,3,0,2});
        check(Document::Sort::TrueLengthDescending,{0,2,1,3,4});check(Document::Sort::TrueLengthAscending,{3,4,1,0,2});
        check(Document::Sort::Identical,{0,2,1,3,4});check(Document::Sort::Reverse,{4,3,2,1,0});
        rejects([&]{d.sorted_order(Document::Sort::ResidueFrequency);});
        Document f;f.load({{"uncommon","CC"},{"majority","AA"},{"majority2","AA"},{"ambiguous","RR"},{"unknown","NN"},{"mask","00"}});
        const auto scores=f.sorted_order(Document::Sort::ResidueFrequency,Document::ColumnRange{0,2});const std::vector<std::size_t> ranks={1,2,3,0,4,5};for(std::size_t i=0;i<ranks.size();++i)require(scores[i]==f.rows()[ranks[i]].id,"Frequency ranks and mask exclusion");
    });
    test("Random row orders undo and redo exact IDs, types and data", [] {
        Document d;std::vector<Record> input;for(int i=0;i<50;++i)input.push_back({std::to_string(i),i%3?"ACGT":"0123"});d.load(input);std::mt19937 rng(401);std::vector<std::string> states{snapshot(d)};
        for(int n=0;n<150;++n){std::vector<SequenceId> order;for(const auto& r:d.rows())order.push_back(r.id);std::shuffle(order.begin(),order.end(),rng);d.reorder(order);states.push_back(snapshot(d));}
        for(std::size_t i=states.size()-1;i>0;--i){d.undo();require(snapshot(d)==states[i-1],"Random ordering undo");}for(std::size_t i=1;i<states.size();++i){d.redo();require(snapshot(d)==states[i],"Random ordering redo");}
    });
    test("MAFFT whole results preserve names, IDs, masks and exact residue case", [] {
        Document d;d.load({{"same Я","a~C-gU?R"},{"same Я","ACgu?R"},{"mask","ACGT01"},{"empty",""},{"gaps","-~~"}});
        const auto before=snapshot(d);MafftAlignment job(d);std::ostringstream input;job.write_input(input);
        require(input.str()==">kleio_1\naCgU?R\n>kleio_2\nACgu?R\n"&&job.mask_count()==1,"Unique temporary names and excluded masks/empty rows");
        auto merged=job.merge(parse(">kleio_2\n-acgT?r-\n>kleio_1\n-acgT?r-\n"));
        auto result=d.derive(std::move(merged.records));require(result.rows()[0].symbols.text()=="~aCgU?R~"&&result.rows()[1].symbols.text()=="~ACgu?R~","Restore case and RNA U");
        require(result.rows()[2].symbols.text()=="ACGT01"&&result.rows()[2].kind==RowKind::Mask&&result.rows()[3].symbols.text()=="~~~~~~~~","Masks exact and empty rows restored");
        for(std::size_t i=0;i<d.rows().size();++i)require(result.rows()[i].id==d.rows()[i].id&&result.rows()[i].name==d.rows()[i].name,"Stable IDs and duplicate Unicode names");
        require(snapshot(d)==before&&!result.can_undo(),"New baseline leaves source untouched");result.replace(result.rows()[0].id,0,1,"N");result.undo();require(result.rows()[0].symbols.text()=="~aCgU?R~","Result edits still undo");
    });
    test("MAFFT rejects malformed output without modifying the source", [] {
        Document d;d.load({{"one","AC"},{"two","AG"}});MafftAlignment job(d);const auto before=snapshot(d);
        const std::vector<std::string> bad={">kleio_1\nAC\n",">kleio_1\nAC\n>kleio_1\nAC\n",">kleio_1\nAC\n>kleio_999\nAG\n",">kleio_1\nAT\n>kleio_2\nAG\n",">kleio_1\nA-\n>kleio_2\nAG\n",">kleio_1\nACC\n>kleio_2\nAG-\n",">kleio_1\nAC-\n>kleio_2\nAG\n",">kleio_1\nAE\n>kleio_2\nAG\n",">kleio_1 suffix\nAC\n>kleio_2\nAG\n"};
        for(const auto& text:bad)rejects([&]{job.merge(parse(text));});
        require(snapshot(d)==before&&!d.can_undo(),"Failed jobs have no source/history effects");
    });
    test("MAFFT fragment growth preserves prefixes, suffixes and mask cells", [] {
        Document d;d.load({{"one","A-~aC~~Tg-"},{"two","TT~ACG~CC"},{"empty fragment","N--~~~~GG"},{"mask","0123456789"},{"ragged","A"}});
        const auto before=snapshot(d);MafftAlignment job(d,Document::ColumnRange{3,4});auto result=job.merge(parse(">kleio_2\nA-CG-\n>kleio_1\na--C-\n"));
        require(result.fragmentWidth==5&&result.records[0].symbols=="A-~a~~C~Tg-"&&result.records[1].symbols=="TT~A~CG~CC","Prefix/fragment/suffix splice");
        require(result.records[2].symbols=="N--~~~~~GG"&&result.records[3].symbols=="0123456~789"&&result.records[4].symbols=="A~~~~~~~","Empty/ragged fragments and exact mask annotations");
        require(snapshot(d)==before,"Fragment source unchanged");
    });
    test("MAFFT fragment shrinking retains outside alignment context", [] {
        Document d;d.load({{"one","A-~aC~~Tg-"},{"two","TT~ACG~CC"},{"empty fragment","N--~~~~GG"}});MafftAlignment job(d,Document::ColumnRange{3,4});
        const auto result=job.merge(parse(">kleio_1\na-C\n>kleio_2\nACG\n"));require(result.fragmentWidth==3&&result.records[0].symbols=="A-~a~CTg-"&&result.records[2].symbols=="N--~~~GG","Shrink changes only the chosen column region");
        d.append({{"mask","0123456789"}});MafftAlignment withMask(d,Document::ColumnRange{3,4});const auto protectedResult=withMask.merge(parse(">kleio_1\na-C\n>kleio_2\nACG\n"));require(protectedResult.fragmentWidth==4&&protectedResult.records[3].symbols=="0123456789","Retain mask columns rather than truncate annotations");
    });
    test("MAFFT whole-row orientation is explicit and preserves DNA/RNA case", [] {
        Document d;d.load({{"dna","aCgT"},{"rna","uAcG"}});MafftAlignment job(d);const auto output=parse(">_R_kleio_1\naCGt\n>_R_kleio_2\ncgtA\n");rejects([&]{job.merge(output);});
        const auto result=job.merge(output,true);require(result.records[0].symbols=="AcGt"&&result.records[1].symbols=="CgUa"&&result.reversed==std::vector<SequenceId>{1,2},"Correct strand, original case and RNA complement");
        MafftAlignment fragment(d,Document::ColumnRange{0,4});rejects([&]{fragment.merge(output,true);});
    });
    test("MAFFT preparation rejects insufficient scope, cancellation and stale sources", [] {
        Document d;d.load({{"one","AC"},{"two","AG"},{"mask","01"}});rejects([&]{MafftAlignment(d,Document::ColumnRange{0,0});});rejects([&]{MafftAlignment(d,Document::ColumnRange{0,99});});rejects([&]{MafftAlignment(d,{},[]{return true;});});
        MafftAlignment job(d);std::ostringstream input;rejects([&]{job.write_input(input,[]{return true;});});rejects([&]{job.merge(parse(">kleio_1\nAC\n>kleio_2\nAG\n"),false,[]{return true;});});
        std::istringstream fasta(">x\nAC\n");rejects([&]{read_fasta(fasta,[]{return true;});});d.rename({{1,"changed"}});rejects([&]{job.write_input(input);});rejects([&]{job.merge(parse(">kleio_1\nAC\n>kleio_2\nAG\n"));});
        Document single;single.load({{"one","AC"},{"empty","--"},{"mask","01"}});rejects([&]{(void)MafftAlignment(single);});
    });
    test("MAFFT options validate presets and biological scoring settings", [] {
        for(int i=0;i<8;++i){MafftOptions options;options.algorithm=static_cast<MafftAlgorithm>(i);const auto args=options.arguments();require(std::find(args.begin(),args.end(),"--nuc")!=args.end()&&std::find(args.begin(),args.end(),"--preservecase")!=args.end(),"Forced nucleotide/case settings");}
        MafftOptions options;options.algorithm=MafftAlgorithm::LargeGaps;options.offset=0;options.direction=MafftDirection::Accurate;options.composition=true;options.memorySaving=true;
        const auto args=options.arguments();for(const auto& flag:{"--genafpair","--adjustdirectionaccurately","--fmodel","--memsave","--threadit"})require(std::find(args.begin(),args.end(),flag)!=args.end(),"Effective advanced settings");
        options.threads=0;rejects([&]{options.arguments();});options.threads=1;options.gapOpen=std::numeric_limits<double>::quiet_NaN();rejects([&]{options.arguments();});options.gapOpen=1.53;options.direction=static_cast<MafftDirection>(99);rejects([&]{options.arguments();});
    });
    test("Derived result baselines retain nonconsecutive IDs without copying history", [] {
        Document d;d.load({{"deleted","A"},{"one","AC"},{"two","AG"},{"mask","01"}});d.erase_rows({1});MafftAlignment job(d);auto result=d.derive(job.merge(parse(">kleio_2\nA-C\n>kleio_3\nAG-\n")).records);
        require(result.rows()[0].id==2&&result.rows()[1].id==3&&result.rows()[2].id==4&&!result.can_undo()&&d.can_undo(),"IDs preserved, history independent");result.append({{"new","A"}});require(result.rows().back().id==5,"Next ID does not collide");auto invalid=job.merge(parse(">kleio_2\nA-C\n>kleio_3\nAG-\n")).records;invalid[0].name="other";rejects([&]{d.derive(invalid);});
    });
    test("Random MAFFT output order and gap layouts retain exact source residues", [] {
        std::mt19937 rng(5101);for(int trial=0;trial<100;++trial){Document d;std::vector<Record> rows;std::vector<std::string> ungapped;
            for(int r=0;r<8;++r){std::string text,letters;for(int c=0;c<20+r;++c){char x="aCcGtURYn?~-"[rng()%12];text+=x;if(!is_gap(x))letters+=x;}rows.push_back({"duplicate",text});ungapped.push_back(letters);}rows.push_back({"mask","012345"});d.load(rows);MafftAlignment job(d);
            std::vector<Record> output;std::size_t width=0;for(const auto& s:ungapped)width=std::max(width,s.size()+8);
            for(std::size_t i=0;i<ungapped.size();++i){std::string s=ungapped[i];for(int g=0;g<8;++g)s.insert(rng()%(s.size()+1),1,'-');s.resize(width,'-');output.push_back({"kleio_"+std::to_string(d.rows()[i].id),s});}std::shuffle(output.begin(),output.end(),rng);auto result=job.merge(output);
            for(std::size_t i=0;i<ungapped.size();++i){auto s=result.records[i].symbols;std::erase_if(s,[](char c){return is_gap(c);});require(s==ungapped[i],"Exact random residue sequence");}require(result.records.back().symbols=="012345","Mask unchanged in random results");
        }
    });
    test("Random column realignment preserves prefixes and suffixes exactly", [] {
        std::mt19937 rng(5102);for(int trial=0;trial<80;++trial){Document d;std::vector<Record> rows;for(int r=0;r<6;++r){std::string s="AcG";for(int c=0;c<12;++c)s+="aCGTN~-"[rng()%7];rows.push_back({std::to_string(r),s});}d.load(rows);MafftAlignment job(d,Document::ColumnRange{2,8});std::vector<Record> output;std::size_t width=0;
            for(const auto& row:d.rows()){auto s=row.symbols.text().substr(2,8);std::erase_if(s,[](char c){return is_gap(c);});s.insert(rng()%(s.size()+1),3,'-');width=std::max(width,s.size());output.push_back({"kleio_"+std::to_string(row.id),s});}for(auto& row:output)row.symbols.resize(width,'-');const auto result=job.merge(output);
            for(std::size_t i=0;i<rows.size();++i){const auto& s=result.records[i].symbols;require(s.substr(0,2)==rows[i].symbols.substr(0,2)&&s.substr(2+width)==rows[i].symbols.substr(10),"Exact context outside random fragments");}
        }
    });
    test("IQ-TREE site counts retain ambiguous compatibility and missing-data patterns", [] {
        const std::vector<std::string> columns={"AACC","AAAC","ARRN","NN~-","~-~.","RYYR","UUtt","CY??","BDVH"};
        std::vector<Record> records;
        for(std::size_t r=0;r<4;++r){std::string text;for(const auto& c:columns)text+=c[r];records.push_back({std::to_string(r),text});}
        records.push_back({"mask","01234567890123456789"});Document d;d.load(records);const auto before=snapshot(d);const auto s=alignment_statistics(d);
        require(s.sequences==4&&s.uniqueSequences==4&&s.columns==9&&s.patterns==8,"Masks excluded, N/gaps share pattern state");
        require(s.informative==1&&s.singleton==3&&s.constant==5&&s.gapOnly==1&&s.nOnly==1,"IQ-TREE classification including ambiguity-only incompatible sites");
        require(snapshot(d)==before&&!d.can_undo(),"Statistics are read-only");
        const auto text=statistics_text(s);require(text.find("Parsimony-informative sites: 1")!=std::string::npos&&text.find('\n')==std::string::npos,"One-line paste-ready report");
    });
    test("Unique sequences normalize case, U/T, missing states and ragged ends", [] {
        Document d;d.load({{"a","AcU~N"},{"duplicate","ACT??"},{"short","act"},{"partial","ACTR"},{"mask","111111111"}});
        const auto s=alignment_statistics(d);require(s.sequences==4&&s.uniqueSequences==2&&s.columns==5&&s.patterns==5,"Normalized states retain partial ambiguity but combine missing calls");
        require(s.constant==5&&s.nOnly==1&&s.gapOnly==0&&s.informative==0&&s.singleton==0,"Partial R is information, missing-only final column");
        d.load({{"empty",""},{"empty 2",""}});const auto empty=alignment_statistics(d);
        require(empty.sequences==2&&empty.uniqueSequences==1&&empty.columns==0&&empty.patterns==0,"Empty nucleotide rows count once as unique");
        d.load({{"mask","123"}});const auto masks=alignment_statistics(d);require(masks.sequences==0&&masks.columns==0&&masks.uniqueSequences==0,"Mask-only document has no nucleotide statistics");
    });
    test("Statistics classify invariant ambiguities and check cancellation in long scans", [] {
        Document d;d.load({{"a","RNY-A"},{"b","V?T~A"},{"c","D-T.A"},{"d","ANN-A"}});const auto s=alignment_statistics(d);
        require(s.constant==5&&s.singleton==0&&s.informative==0&&s.gapOnly==1&&s.nOnly==1,"Shared nucleotide remains constant, even with different ambiguity codes");
        rejects([&]{alignment_statistics(d,[]{return true;});});
        d.load({{"a",std::string(20000,'N')},{"b",std::string(20000,'A')}});int checks=0;
        rejects([&]{alignment_statistics(d,[&]{return ++checks==4;});});require(checks==4,"Cancellation within a row");
    });
    test("Rectangle deletion is one exact undoable transaction on ragged nucleotide and mask rows", [] {
        Document d;d.load({{"a","AC-G~T"},{"b","AC"},{"mask","12-3~4"}});const auto before=snapshot(d);
        d.replace_many({{1,1,4,""},{2,1,1,""},{3,1,4,""}});
        require(d.rows()[0].symbols.text()=="AT"&&d.rows()[1].symbols.text()=="A"&&d.rows()[2].symbols.text()=="14","Block content removed; stored suffixes shift left");
        d.undo();require(snapshot(d)==before&&!d.can_undo(),"One undo restores exact gap states and all rows");d.redo();require(d.rows()[2].symbols.text()=="14","Redo removes mask block again");
    });
    test("Template addition preserves the complete axis and exact non-target gap states", [] {
        Document d;d.load({{"dup","a~GC~u"},{"dup","~A--C~"},{"dup","~a~~C~"},{"untouched","gT--NN"},{"mask","0123456789"},{"empty",""}});
        const auto before=snapshot(d);MafftTemplateAlignment job(d,{{1},{2,3},false});std::ostringstream templates,targets;job.write_templates(templates);job.write_targets(targets);
        require(templates.str()==">kleio_2\nAC\n>kleio_3\naC\n"&&targets.str()==">kleio_1\naGCu\n","Synthetic stable identifiers and compressed template ledger");
        const auto result=job.merge(parse(">kleio_3\n-a---c-\n>kleio_1\na-G--Ct\n>kleio_2\n-A---C-\n"));
        require(result.insertedColumns==3&&result.alignment.fragmentWidth==13&&job.restored_columns()==8,"Original axis plus leading/internal/trailing target insertions");
        require(result.originalColumns==std::vector<std::size_t>{0,2,3,4,6,7,8,9,10,11},"Original gap intervals precede new insertions; trailing insertions follow mask tail");
        for(std::size_t i=1;i<d.rows().size();++i){const auto& old=d.rows()[i].symbols.text();const auto& now=result.alignment.records[i].symbols;for(std::size_t c=0;c<old.size();++c)require(now[result.originalColumns[c]]==old[c],"Exact non-target projection including case/locked gaps/masks");}
        auto text=result.alignment.records[0].symbols;std::erase_if(text,[](char c){return is_gap(c);});require(text=="aGCu","Target letters and RNA case restored exactly");
        auto derived=d.derive(result.alignment.records);require(derived.rows()[0].id==1&&!derived.can_undo()&&snapshot(d)==before,"Independent new baseline and immutable source");
    });
    test("Template anchor validation detects splits, merges, missing residues and unknown rows", [] {
        Document d;d.load({{"target","ACG"},{"template","AC"},{"template","AC"}});MafftTemplateAlignment job(d,{{1},{2,3},false});
        rejects([&]{job.merge(parse(">kleio_1\nACG\n>kleio_2\nA-C\n>kleio_3\n-AC\n"));});
        rejects([&]{job.merge(parse(">kleio_1\nACG\n>kleio_2\nAC-\n>kleio_3\nA-C\n"));});
        rejects([&]{job.merge(parse(">kleio_1\nATG\n>kleio_2\nAC-\n>kleio_3\nAC-\n"));});
        rejects([&]{job.merge(parse(">kleio_1\nACG\n>kleio_2\nAC-\n>kleio_99\nAC-\n"));});
        rejects([&]{job.merge(parse(">kleio_1\nACG\n>kleio_2\nAC-\n>kleio_2\nAC-\n"));});
        rejects([&]{job.merge(parse(">_R_kleio_1\nACG\n>kleio_2\nAC-\n>kleio_3\nAC-\n"));});
        rejects([&]{job.merge(parse(">kleio_1\nACG\n>kleio_2\nAC\n>kleio_3\nAC-\n"));});
        rejects([&]{job.merge(parse(">kleio_1\nACG\n>kleio_2\nAC-\n"));});
        // Two distinct original columns cannot be collapsed onto a shared residue column.
        Document partial;partial.load({{"target","AG"},{"left","A~"},{"right","~G"}});MafftTemplateAlignment separate(partial,{{1},{2,3},false});
        rejects([&]{separate.merge(parse(">kleio_1\nAG\n>kleio_2\nA-\n>kleio_3\nG-\n"));});
    });
    test("Template requests reject overlap, empty or mask inputs, cancellation and stale revisions", [] {
        Document d;d.load({{"one","AC"},{"two","AG"},{"mask","01"},{"empty","~~"}});
        for(const auto& request:std::vector<MafftTemplateRequest>{{{1},{1},false},{{1,1},{2},false},{{99},{2},false},{{3},{2},false},{{1},{4},false},{{},{2},false},{{1},{},false}})rejects([&]{MafftTemplateAlignment job(d,request);});
        rejects([&]{MafftTemplateAlignment job(d,{{1},{2},false},[]{return true;});});MafftTemplateAlignment job(d,{{1},{2},false});std::ostringstream out;
        rejects([&]{job.write_targets(out,[]{return true;});});rejects([&]{job.write_templates(out,[]{return true;});});rejects([&]{job.merge(parse(">kleio_1\nAC\n>kleio_2\nAG\n"),[]{return true;});});
        d.rename({{1,"changed"}});rejects([&]{job.write_targets(out);});rejects([&]{job.merge(parse(">kleio_1\nAC\n>kleio_2\nAG\n"));});
    });
    test("Random template projections preserve repeated motifs and ragged untouched rows", [] {
        std::mt19937 rng(7101);
        for(int trial=0;trial<120;++trial){Document d;std::vector<Record> rows;std::vector<std::size_t> anchors;
            const std::size_t oldWidth=40;std::string one(oldWidth,'~'),two(oldWidth,'-');
            for(std::size_t c=0;c<oldWidth;++c)if(rng()%3){anchors.push_back(c);one[c]="aCAT"[rng()%4];if(rng()%2)two[c]=one[c];}
            rows.push_back({"duplicate",""});rows.push_back({"duplicate",one});rows.push_back({"duplicate",two});rows.push_back({"other","Ac~-CGNN"});rows.push_back({"mask",std::string(45,'9')});
            std::string target,outOne,outTwo,outTarget;for(auto c:anchors){int insert=rng()%3;for(int n=0;n<insert;++n){outOne+='-';outTwo+='-';char x="aCGTuN"[rng()%6];outTarget+=x;target+=x;}outOne+=one[c];outTwo+=two[c];char x="ACGT"[rng()%4];outTarget+=x;target+=x;}
            outOne+="--";outTwo+="--";outTarget+="uA";target+="uA";rows[0].symbols=target;d.load(rows);
            // The target can be longer than every other row; that old tail also survives.
            MafftTemplateAlignment job(d,{{1},{2,3},true});const auto result=job.merge({{"kleio_3",outTwo},{"kleio_1",outTarget},{"kleio_2",outOne}});
            require(result.originalColumns.size()==d.columns()&&result.alignment.fragmentWidth==d.columns()+result.insertedColumns,"Never shrink the old column axis");
            for(std::size_t i=1;i<rows.size();++i)for(std::size_t c=0;c<rows[i].symbols.size();++c)require(result.alignment.records[i].symbols[result.originalColumns[c]]==rows[i].symbols[c],"Exact randomized old projection");
            auto restored=result.alignment.records[0].symbols;std::erase_if(restored,[](char c){return is_gap(c);});require(restored==target,"Exact randomized target residues");
        }
    });
    test("Bulk memory refusal is atomic, reversible and skips unchanged rows", [] {
        Document d;d.load({{"long",std::string(1024,'A')},{"short","a-~C"}});d.rename({{1,"renamed"}});
        const auto before=snapshot(d);const auto state=d.state_id(),history=d.history_bytes();d.set_bulk_edit_memory_limit(8);
        for(auto operation:{0,1,2}){rejects([&]{if(operation==0)d.flush();else if(operation==1)d.transform({1,2},Document::Transform::Unalign);else d.minimize();});require(snapshot(d)==before&&d.state_id()==state&&d.history_bytes()==history,"Refusal preserves rows/state/history");}
        d.transform({1},Document::Transform::Unalign);require(d.state_id()==state,"Already ungapped row needs no large copy");
        d.set_bulk_edit_memory_limit(1024*1024);d.flush();require(d.sequence(2).symbols.size()==1024,"Smaller bounded flush succeeds");d.undo();require(snapshot(d)==before,"Flush undo exact");d.redo();d.undo();d.undo();require(d.sequence(1).name=="long","Earlier Undo remains intact after refusals");
    });
    test("Huge virtual rectangles reject padding before allocation", [] {
        Document d;std::vector<Record> rows;rows.push_back({"long",std::string(1000000,'A')});for(int i=0;i<10000;++i)rows.push_back({std::to_string(i),""});d.load(std::move(rows));
        const auto state=d.state_id();rejects([&]{d.flush();});require(d.state_id()==state&&!d.can_undo()&&d.sequence(10001).symbols.size()==0,"Ten-billion-cell flush refuses without materializing gaps");
    });
    test("Small edits in long ordinary rows avoid full-row working copies", [] {
        Document d;d.load({{"genome",std::string(1000000,'A')}});d.set_bulk_edit_memory_limit(64);d.replace(1,500000,1,"T");require(d.sequence(1).symbols.text()[500000]=='T',"Small patch within tiny working budget");d.undo();require(d.sequence(1).symbols.text()[500000]=='A',"Exact small-patch undo");
    });
    std::cout << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}

