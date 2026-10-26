#pragma once
#include "core/alignment.hpp"
#include "core/translation.hpp"
#include "gui/residue_style.hpp"
#include "core/motif_index.hpp"
#include <QAbstractScrollArea>
#include <QCache>
#include <QHash>
#include <QPixmap>
#include <array>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <unordered_set>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QTimer;

namespace kleiotide {
class AlignmentView final : public QAbstractScrollArea {
public:
    enum class Mode { View=0, Insert=1, Rewrite=2 };
    enum class RightClick { Menu, InsertGap, DeleteGap, InsertOthers, DeleteOthers };
    struct Fragment { std::vector<SequenceId> ids; std::size_t start = 0, width = 0;bool columns=false;
        bool operator==(const Fragment&) const = default; };
    void set_right_click(RightClick mode) { rightClick_ = mode_==Mode::View?RightClick::Menu:mode; }
    const std::optional<Fragment>& fragment() const { return fragment_; }
    void select_fragment(Fragment fragment);
    void select_trace_fragment(SequenceId id,std::size_t start,std::size_t width){select_fragment({{id},start,width});}
    void slide_fragment(std::int64_t distance);
    void fill_fragment(char symbol);
    void unlock_fragment();
    void lock_fragment();
    std::optional<QString> selected_residue_text() const;
    bool paste_residues(const QString& text);
    std::optional<std::vector<std::string>> selected_residue_rows()const;
    bool cut_residues();
    bool paste_residue_rows(const std::vector<std::string>&);
    std::pair<std::size_t, std::size_t> visible_column_range() const;
    void navigate_to(std::size_t column);
    void navigate_to_cell(SequenceId id,std::size_t column);
    void set_search_match(std::optional<Fragment>);
    const std::optional<Fragment>& search_match()const{return searchMatch_;}
    void set_search_results(std::shared_ptr<const MotifIndex> index){
        if(searchResults_!=index&&residueDisplay_==ResidueDisplay::Backgrounds)tiles_.clear();
        searchResults_=std::move(index);searchRevision_=document_?document_->state_id():0;viewport()->update();}
    std::shared_ptr<const MotifIndex> search_results()const{return searchResults_;}
    std::optional<std::pair<SequenceId,std::size_t>> caret() const {return caretId_?std::optional<std::pair<SequenceId,std::size_t>>({*caretId_,caretColumn_}):std::nullopt;}
    std::function<void()> viewport_changed;
    explicit AlignmentView(QWidget* parent = nullptr);
    void set_document(Document* document);
    void set_mode(Mode mode);
    Mode mode() const noexcept { return mode_; }
    void refresh();
    void reveal_residues(const Document::ResidueLocation& location);
    void set_identity_dots(bool enabled);
    void set_translation(std::shared_ptr<const TranslationResult>,bool enabled,bool amino,bool ruler);
    void set_protein_profiles(std::shared_ptr<const ProteinProfiles> p){proteinProfiles_=std::move(p);tiles_.clear();viewport()->update();}
    bool insertable_selection()const;
    bool can_copy_translation()const;
    std::optional<Fragment> expanded_translation_fragment()const;
    bool complete_selection()const;
    std::function<void()> add_translation_requested,copy_translation_requested;
    std::function<void(SequenceId)> annotate_mask_requested;

    bool identity_dots() const noexcept { return identityDots_; }
    void set_residue_palette(ResiduePalette palette);
    ResiduePalette residue_palette() const noexcept {return residuePalette_;}
    void set_residue_display(ResidueDisplay display);
    ResidueDisplay residue_display() const noexcept {return residueDisplay_;}
    void set_site_highlights(std::shared_ptr<const std::vector<unsigned char>> columns,QColor color,RowKind molecule,const std::vector<SequenceId>* rows=nullptr){
        siteHighlights_=std::move(columns);siteColor_=color;siteMolecule_=molecule;siteRows_.reset();if(rows)siteRows_.emplace(rows->begin(),rows->end());siteRevision_=document_?document_->content_id():0;viewport()->update();
    }
    std::shared_ptr<const std::vector<unsigned char>> site_highlights()const{return siteHighlights_;}
    QColor site_highlight_color()const{return siteColor_;}
    void set_name_area_width(int width) {resize_name_area(width);}
    int name_area_width() const noexcept { return nameWidth_; }
    int ruler_height() const noexcept { return rulerHeight_; }
    int row_height() const noexcept { return rowHeight_; }
    int cell_width() const noexcept { return cellWidth_; }
    std::vector<SequenceId> selected_ids() const;
    void select_all();
    void unselect_all();
    void invert_selection();
    void select_ids(const std::vector<SequenceId>& ids);
    std::function<void()> changed;
    std::function<void()> selection_changed;
    std::function<void()> paste_requested;
    std::function<void()> copy_rows_requested,cut_rows_requested;
    std::function<void()> copy_fragment_requested,cut_fragment_requested,paste_fragment_requested;
    std::function<void(SequenceId)> rename_requested;
    std::function<void()> delete_rows_requested;
    std::function<void()> add_group_requested,remove_group_requested;
    std::function<void(const QString&)> message;
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void changeEvent(QEvent*) override;
    void scrollContentsBy(int dx, int dy) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private:
    struct Cell { std::size_t row, column; };
    std::optional<Cell> cell_at(const QPoint& point) const;
    int visible_rows() const;
    int visible_columns() const;
    void ensure_caret_visible();
    void update_row_drop(const QPoint& point);
    void clear_row_drag();
    void notify_selection();
    void update_metrics();
    void resize_name_area(int width);
    bool on_name_border(int x) const { return std::abs(x-(nameWidth_-1))<=4; }
    void ensure_render_cache();
    void clear_render_cache();
    const QPixmap& row_tile(const Sequence& row, std::size_t tile,bool selectedStyle=false);
    Document* document_ = nullptr;
    Mode mode_ = Mode::View;
    bool identityDots_ = false;
    bool translationEnabled_=false,aminoLetters_=false,codonRuler_=false;
    std::shared_ptr<const TranslationResult> translation_;
    std::shared_ptr<const ProteinProfiles> proteinProfiles_;

    ResiduePalette residuePalette_=ResiduePalette::Classic;
    ResidueDisplay residueDisplay_=ResidueDisplay::Letters;
    RightClick rightClick_ = RightClick::Menu;
    std::optional<Fragment> fragment_;
    std::optional<Fragment> searchMatch_;
    std::shared_ptr<const MotifIndex> searchResults_;
    std::uint64_t searchRevision_=0;
    std::shared_ptr<const std::vector<unsigned char>> siteHighlights_;
    QColor siteColor_{198,173,242};
    std::optional<std::unordered_set<SequenceId>> siteRows_;
    RowKind siteMolecule_=RowKind::Nucleotide;
    std::uint64_t siteRevision_=0;
    std::optional<Cell> dragAnchor_, selectionOrigin_;
    std::vector<SequenceId> dragRows_;
    bool draggingBlock_ = false, rulerDrag_ = false;
    std::int64_t dragOffset_ = 0;
    void update_fragment_drag(const QPoint& point);
    bool fragment_contains(const Cell& cell) const;
    void clear_fragment();
    void notify_edit();
    void remember_fragment();
    void sync_fragment_caret();
    bool blue_fragment() const;
    std::pair<QRegion,QRegion> selection_paint_regions() const;
    bool write_residues(std::string_view text);
    void delete_residues(bool backspace);
    std::map<std::uint64_t, Fragment> fragmentHistory_;
    QTimer* dragScroll_ = nullptr;
    QPoint dragPoint_;
    std::set<SequenceId> selected_;
    std::unordered_set<SequenceId> liveIds_;
    std::optional<SequenceId> caretId_;
    std::size_t caretColumn_ = 0;
    std::size_t selectionAnchor_ = 0;
    std::optional<SequenceId> selectionAnchorId_;
    std::optional<QPoint> rowDragStart_;
    std::optional<SequenceId> collapseNameSelection_;
    std::vector<SequenceId> rowDragIds_;
    bool draggingRows_ = false;
    std::size_t rowDropIndex_ = 0;
    int nameWidth_ = 230, cellWidth_ = 12, rowHeight_ = 22, rulerHeight_ = 22;
    bool resizingNames_ = false;
    int nameResizeOffset_ = 0;
    static constexpr std::size_t tileColumns_ = 128;
    QCache<QString, QPixmap> tiles_{32 * 1024 * 1024};
    QHash<quint64, QString> names_;
    std::array<QPixmap, 128> glyphs_;
    std::array<QPixmap, 128> maskGlyphs_;
    std::array<QPixmap, 128> plainGlyphs_,dotGlyphs_,searchGlyphs_;
    QPixmap oversizedTile_;
    qreal cacheDpr_ = 0;
    std::uint64_t cacheState_ = 0;
    // Selection undo must not depend on palette/font/tile cache invalidation.
    std::uint64_t selectionState_ = 0;
};
}

