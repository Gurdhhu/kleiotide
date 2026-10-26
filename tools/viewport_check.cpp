#include "gui/alignment_view.hpp"
#include <QApplication>
#include <QBackingStore>
#include <QElapsedTimer>
#include <QImage>
#include <QScrollBar>
#include <iostream>
#include <stdexcept>
using namespace kleiotide;

// Read the actual raster backing store, without forcing a fresh render first.
// This catches stale pixels left by partial scrolling or cache reuse.
QImage pixels(AlignmentView& view) {
    auto* device = view.backingStore()->paintDevice();
    if (device->devType() != QInternal::Image)
        throw std::runtime_error("This check requires a raster backing store (use Qt offscreen on Linux).");
    auto* image = static_cast<QImage*>(device);
    const auto origin = view.viewport()->mapTo(&view, QPoint(0, 0));
    const auto dpr = image->devicePixelRatio();
    return image->copy(QRect(qRound(origin.x() * dpr), qRound(origin.y() * dpr),
                            qRound(view.viewport()->width() * dpr), qRound(view.viewport()->height() * dpr)));
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    auto arguments=app.arguments().mid(1);const bool identityDots=arguments.removeAll("--identity-dots")>0;
    const bool backgrounds=arguments.removeAll("--backgrounds")>0,monochrome=arguments.removeAll("--monochrome")>0,selected=arguments.removeAll("--selected")>0;
    const bool searchHighlight=arguments.removeAll("--search-highlight")>0;
    const bool allMatches=arguments.removeAll("--all-matches")>0;
    const bool groups=arguments.removeAll("--groups")>0;
    const bool sites=arguments.removeAll("--site-highlights")>0;
    auto palette=ResiduePalette::Classic;for(const auto& argument:QStringList(arguments))if(argument.startsWith("--palette=")){palette=palette_from_id(argument.mid(10));arguments.removeAll(argument);}
    const auto style=monochrome?ResidueDisplay::Monochrome:backgrounds?ResidueDisplay::Backgrounds:ResidueDisplay::Letters;
    try {
        Document document;
        std::vector<Record> records;
        const std::string alphabet = "ACGTN~-acgtRY";
        for (int r = 0; r < 95; ++r) {
            std::string symbols;
            for (int c = 0; c < 1792 - r; ++c) symbols += alphabet[(c + r) % alphabet.size()];
            records.push_back({"Sequence " + std::to_string(r), symbols});
        }
        document.load(std::move(records));
        if(groups){std::vector<std::pair<SequenceId,GroupId>> members;for(const auto& row:document.rows())if(row.id%3)members.emplace_back(row.id,row.id%3);document.apply_groups({{1,"Blue",0x0072b2},{2,"Orange",0xe69f00}},members);}
        auto apply_sites=[&](AlignmentView& target){if(!sites)return;auto columns=std::make_shared<std::vector<unsigned char>>(document.columns());for(std::size_t c=0;c<columns->size();++c)(*columns)[c]=c%5<3;target.set_site_highlights(columns,QColor(198,173,242),RowKind::Nucleotide);};
        AlignmentView view;
        view.set_identity_dots(identityDots);
        view.set_residue_palette(palette);view.set_residue_display(style);
        view.resize(3440, 1440); view.set_document(&document);apply_sites(view); view.show(); app.processEvents();
        std::vector<SequenceId> ids;for(const auto& row:document.rows())ids.push_back(row.id);
        if(selected){view.select_fragment({ids,0,document.columns()});app.processEvents();}
        if(searchHighlight){view.set_search_match(AlignmentView::Fragment{{ids[10]},100,1500});app.processEvents();}
        if(allMatches){auto index=std::make_shared<MotifIndex>();scan_motifs(document,"N",{true,false,true},[&](auto hit){index->add(hit);});index->finish();view.set_search_results(index);app.processEvents();}
        auto check = [&](const char* label) {
            const auto before = pixels(view);
            view.viewport()->repaint();
            const auto after = pixels(view);
            if (before != after) {
                before.save("viewport_before_failure.png"); after.save("viewport_after_failure.png");
                throw std::runtime_error(label);
            }
        };
        QElapsedTimer timer;
        timer.start();
        for (int i = 0; i < 200; ++i) { view.horizontalScrollBar()->setValue(i + 1); app.processEvents(); }
        std::cout << "horizontal ms/frame " << timer.nsecsElapsed() / 200.0e6 << '\n';
        check("horizontal incremental scroll mismatch");
        timer.restart();
        for (int i = 0; i < 200; ++i) {
            const auto maximum = view.verticalScrollBar()->maximum();
            view.verticalScrollBar()->setValue(maximum > 0 ? i % (maximum + 1) : 0); app.processEvents();
        }
        std::cout << "vertical ms/frame " << timer.nsecsElapsed() / 200.0e6 << '\n';
        check("vertical incremental scroll mismatch");
        // Both directions, large jumps, and exposure of short row ends.
        for (int position : {1000, 1791, 0, 800, 1}) {
            view.horizontalScrollBar()->setValue(position); app.processEvents(); check("horizontal jump mismatch");
            view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum()); app.processEvents(); check("vertical jump mismatch");
            view.verticalScrollBar()->setValue(0); app.processEvents(); check("vertical return mismatch");
        }
        if(searchHighlight)view.set_search_match({});
        if(allMatches)view.set_search_results({});
        view.horizontalScrollBar()->setValue(0); view.verticalScrollBar()->setValue(0); app.processEvents();
        const auto original = pixels(view);
        document.replace(document.rows()[0].id, 0, 1, "T"); view.refresh();apply_sites(view); app.processEvents();
        check("edit repaint mismatch");
        const auto edited = pixels(view);
        if (edited == original) throw std::runtime_error("edited residue was not redrawn");
        AlignmentView fresh;
        fresh.set_identity_dots(identityDots);
        fresh.set_residue_palette(palette);fresh.set_residue_display(style);
        fresh.resize(view.size()); fresh.set_document(&document);apply_sites(fresh); fresh.show(); app.processEvents();
        if(selected){fresh.select_fragment({ids,0,document.columns()});app.processEvents();}
        if (edited != pixels(fresh)) throw std::runtime_error("edited cache differs from fresh document view");
        document.undo(); view.refresh();apply_sites(view); app.processEvents(); check("undo repaint mismatch");
        if (pixels(view) != original) {original.save("viewport_undo_expected.png");pixels(view).save("viewport_undo_actual.png");throw std::runtime_error("undo did not restore original pixels");}
        document.redo(); view.refresh();apply_sites(view); app.processEvents();
        if (pixels(view) != edited) throw std::runtime_error("redo did not restore edited pixels");
        view.select_all(); app.processEvents(); check("selection mismatch"); view.unselect_all(); app.processEvents();
        QFont font = view.font(); font.setPointSize(14); view.setFont(font); app.processEvents(); check("font mismatch");
        fresh.unselect_all();fresh.setFont(font); app.processEvents();
        if (pixels(view) != pixels(fresh)) throw std::runtime_error("font cache differs from fresh view");
        if (!arguments.isEmpty() && !view.viewport()->grab().save(arguments.front()))
            throw std::runtime_error("Could not save viewport image");
        std::cout << "PASS viewport checks\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
