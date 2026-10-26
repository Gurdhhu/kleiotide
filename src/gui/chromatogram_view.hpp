#pragma once
#include "core/alignment.hpp"
#include "core/trace_coordinates.hpp"
#include "gui/residue_style.hpp"
#include <QAbstractScrollArea>
#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <functional>
class QComboBox;
class QKeyEvent;
class QLabel;
class QSpinBox;
namespace kleiotide {
class TraceCanvas final : public QAbstractScrollArea {
public:
    explicit TraceCanvas(QWidget* parent=nullptr);
    void set_sequence(const Sequence* row);
    void focus_column(std::size_t column,bool center=true);
    void center_column(std::size_t column);
    void set_zoom(int percent);
    void set_gain(int percent);
    void set_residue_palette(ResiduePalette palette){if(palette_==palette)return;palette_=palette;invalidate_render_cache();}
    ResiduePalette residue_palette()const{return palette_;}
    void set_alignment_spacing(bool enabled);
    bool alignment_spacing() const {return aligned_;}
    void set_alignment_columns(std::size_t columns);
    void set_selection(std::size_t start,std::size_t width,bool blue);
    bool interaction_active() const {return interacting_;}
    bool accepts_editing() const {return active_&&(!pickedCall_||trace_column(row_,*pickedCall_).has_value());}
    double column_x(std::size_t column) const;
    TracePosition position(std::size_t column) const;
    void step_call(int delta);
    struct Label {std::size_t column;double sample;char symbol;bool inserted;bool edited=false;};
    std::vector<Label> current_labels() const;
    SequenceId sequence_id() const {return active_?row_.id:0;}
    std::size_t focused_column() const {return column_;}
    std::pair<double,double> visible_samples() const;
    std::function<void(SequenceId,std::size_t)> cell_requested;
    std::function<void(const QString&)> evidence_selected;
    std::function<void(SequenceId,std::size_t,std::size_t)> range_requested;
    std::function<void(QKeyEvent*)> key_requested;
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void changeEvent(QEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
private:
    double displayed(double sample) const;
    double original(double sample) const;
    double raw_from_oriented(double sample) const;
    double pixels_per_unit() const {return aligned_?20.*zoom_:zoom_;}
    double origin() const {return aligned_?-.5:0.;}
    double sample_x(double sample) const;
    int label_gutter() const;
    std::optional<std::size_t> column_at(double x) const;
    void update_scrollbar();
    void scroll_to(double position);
    void select_call(std::size_t call);
    void rebuild_positions();
    void invalidate_render_cache();
    void render_trace_layers();
    Sequence row_;
    ResiduePalette palette_=ResiduePalette::Classic;
    bool active_=false;
    std::size_t column_=0;
    std::optional<std::size_t> pickedCall_;
    double zoom_=1.,gain_=1.,maximum_=1.;
    bool aligned_=false,interacting_=false,blue_=false;
    std::size_t alignmentColumns_=0,selectionStart_=0,selectionWidth_=0;
    std::optional<std::size_t> dragAnchor_,selectionOrigin_;
    std::optional<TraceCoordinates> coordinates_;
    std::vector<std::size_t> currentColumns_;
    // Rebuilt only when sequence text/trace mapping changes. Samples retain the
    // model's interpolation for inserted calls; they are never new evidence.
    std::vector<double> currentSamples_;
    std::vector<std::size_t> anchorOrdinals_;
    std::vector<std::optional<std::size_t>> originalColumns_;
    QPixmap traceLayer_,currentLayer_;
    // Only viewport-sized layers; selection and caret are drawn independently.
    bool renderDirty_=true;
};
class ChromatogramPanel final : public QWidget {
public:
    explicit ChromatogramPanel(QWidget* parent=nullptr);
    void set_document(Document* document);
    bool has_traces() const;
    void follow_selection(const std::vector<SequenceId>& ids,std::optional<std::size_t> column);
    void follow_viewport(std::size_t begin,std::size_t end);
    void choose_sequence(SequenceId id);
    void set_residue_palette(ResiduePalette palette);
    TraceCanvas* canvas() const {return canvas_;}
    std::function<void(SequenceId,std::size_t)> cell_requested;
    std::function<void(SequenceId,std::size_t,std::size_t)> range_requested;
    std::function<void(QKeyEvent*)> key_requested;
private:
    const Sequence* current_row() const;
    void update_row();
    void update_position(std::size_t column);
    Document* document_=nullptr;
    QComboBox* choices_=nullptr;
    QLabel *source_=nullptr,*position_=nullptr,*legend_=nullptr;
    TraceCanvas* canvas_=nullptr;
    QHash<quint64,int> traceChoices_;
    std::uint64_t state_=0;
};
}
