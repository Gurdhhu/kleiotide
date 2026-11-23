#include "core/double_peaks.hpp"
#include "core/version.hpp"
#include "gui/alignment_view.hpp"
#include "gui/chromatogram_view.hpp"
#include "gui/file_output.hpp"
#include "gui/main_window.hpp"
#include "gui/table_export.hpp"

#include <QAbstractTableModel>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>

namespace kleiotide {
namespace {
struct PreviewColumn {
    const char* name;
    const char* tooltip;
    int width;
};
constexpr std::array columns{
    PreviewColumn{"Apply", "Check to apply this substitution.", 52},
    PreviewColumn{"Sequence", "Name of the sequence with this chromatogram.", 180},
    PreviewColumn{"Alignment column", "Position in the alignment, counting gaps and starting at 1.", 115},
    PreviewColumn{"Current", "Base or ambiguity symbol now in the alignment.", 66},
    PreviewColumn{"Proposed", "IUPAC ambiguity symbol for the two detected bases.", 80},
    PreviewColumn{"Peak pair", "The major base first and the secondary base second, in the alignment strand.", 88},
    PreviewColumn{"Secondary / major (%)", "Height of the secondary peak as a percentage of the major peak. Both are measured from the same zero line.", 145},
    PreviewColumn{"Third / major (%)", "Height of the third peak as a percentage of the major peak. Zero means no third peak met the shape and spacing criteria.", 128},
    PreviewColumn{"Displacement (samples)", "Horizontal distance between the two peak tops, in recorded signal points.", 155},
    PreviewColumn{"Displacement (% of spacing)", "Distance between the peak tops as a percentage of the typical spacing between neighboring bases near this site.", 174},
    PreviewColumn{"Review", "Proposals whose peak pair excludes the current symbol start unchecked.", 315}
};

QString number(double value) {
    return QString::number(value, 'f', 2);
}

TextTable calculation_rules(const DoublePeakPlan& plan) {
    const auto& settings = plan.settings;
    return {{"Setting", "Value"},
        {"Kleiotide version", QString::fromLatin1(version)},
        {"Scope", settings.allTraces ? "All sequences with chromatograms" : "Selected sequences with chromatograms"},
        {"Exclude edited bases", settings.excludeEdited ? "Yes" : "No"},
        {"Edited bases", "Manual substitutions, case changes and inserted calls. For sites without recorded edit history, compare the current letter to the strand-oriented imported letter, including case. A call without an original index counts as edited."},
        {"Anchored edited bases skipped", QString::number(qulonglong(plan.editedExcluded))},
        {"Secondary peak threshold (%)", QString::number(settings.secondaryPercent)},
        {"Third peak exclusion threshold (%)", QString::number(settings.thirdPercent)},
        {"Maximum peak displacement (% of local spacing)", QString::number(settings.displacementPercent)},
        {"Positions examined", "Current residues with imported, inferred or manually placed signal anchors. Gaps and interpolated positions are skipped."},
        {"Local spacing", "Median of nearby positive differences between imported calls, with neighboring current anchors as a fallback."},
        {"Site window", "Within 40% of local spacing on each side of the current anchor, stopping before the midpoint to a neighboring current anchor."},
        {"Major peak", "Find the tallest signal in the site window across all channels. Skip the site if none of the tallest signals forms a qualifying peak."},
        {"Peak shape", "A local maximum must rise on both sides by at least one signal unit and at least 20% of its rise above the lower flank minimum. Examine up to half the local spacing on each flank."},
        {"Peak height", "Height above the common zero line of the displayed chromatogram. The two flanks are used to check peak shape."},
        {"Secondary and third peak ratios", "For each other channel, take the strongest qualifying peak within the displacement limit from the major apex. Divide by the major peak height."},
        {"Threshold comparisons", "The secondary peak must meet or exceed its threshold. The third peak must fall below its threshold. Maximum displacement is inclusive."},
        {"Strand and case", "Peak pairs are reported in the alignment strand. Proposed IUPAC symbols use the case of the current letter."},
        {"Review", "A proposal starts unchecked if the current symbol is outside the detected pair or is ambiguous and differs from the proposal."},
        {"Phred quality", "Quality scores are ignored."},
        {"Sequences examined", QString::number(qulonglong(plan.rows))},
        {"Current residues in scope", QString::number(qulonglong(plan.sites))},
        {"Residues without an anchor", QString::number(qulonglong(plan.unanchored))},
        {"Sites without a clear major peak", QString::number(qulonglong(plan.unresolvedMajor))},
        {"Sites excluded by a third peak", QString::number(qulonglong(plan.thirdPeakExcluded))}};
}

class PeakTableModel final : public QAbstractTableModel {
public:
    explicit PeakTableModel(const Document& source, QObject* parent)
        : QAbstractTableModel(parent), source_(source) {}

    int rowCount(const QModelIndex& parent = {}) const override {
        return !parent.isValid() && plan ? static_cast<int>(plan->calls.size()) : 0;
    }

    int columnCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : static_cast<int>(columns.size());
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (orientation == Qt::Horizontal && section >= 0 && section < columnCount()) {
            const auto& column = columns[static_cast<std::size_t>(section)];
            if (role == Qt::DisplayRole) return QString::fromUtf8(column.name);
            if (role == Qt::ToolTipRole) return QString::fromUtf8(column.tooltip);
        }
        return {};
    }

    QString cell(int row, int column) const {
        const auto& call = plan->calls[static_cast<std::size_t>(row)];
        switch (column) {
        case 0: return included[static_cast<std::size_t>(row)] ? "Yes" : "No";
        case 1: return QString::fromStdString(source_.sequence(call.id).name);
        case 2: return QString::number(qulonglong(call.column + 1));
        case 3: return QString(QChar::fromLatin1(call.current));
        case 4: return QString(QChar::fromLatin1(call.proposed));
        case 5: return QString("%1 + %2").arg(QChar::fromLatin1(call.major)).arg(QChar::fromLatin1(call.minor));
        case 6: return number(call.secondaryPercent);
        case 7: return number(call.thirdPercent);
        case 8: return QString::number(qulonglong(call.displacementSamples));
        case 9: return number(call.displacementPercent);
        case 10: return call.needsReview ? "Detected peak pair does not include the current symbol" : QString();
        default: return {};
        }
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || !plan) return {};
        if (role == Qt::CheckStateRole && index.column() == 0)
            return included[static_cast<std::size_t>(index.row())] ? Qt::Checked : Qt::Unchecked;
        if (role == Qt::DisplayRole && index.column() != 0)
            return cell(index.row(), index.column());
        if (role == Qt::ToolTipRole) {
            if (index.column() == 6) {
                const auto& call = plan->calls[static_cast<std::size_t>(index.row())];
                return QString("Secondary peak %1 is %2 of %3 signal units above zero (%4%).")
                    .arg(QChar::fromLatin1(call.minor)).arg(call.secondaryHeight).arg(call.majorHeight)
                    .arg(number(call.secondaryPercent));
            }
            const auto value = cell(index.row(), index.column());
            if (index.column() == 1 || index.column() == 10) {
                if (!value.isEmpty()) return value;
            }
            return headerData(index.column(), Qt::Horizontal, Qt::ToolTipRole);
        }
        return {};
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override {
        auto result = QAbstractTableModel::flags(index);
        if (index.isValid() && index.column() == 0) result |= Qt::ItemIsUserCheckable;
        return result;
    }

    bool setData(const QModelIndex& index, const QVariant& value, int role) override {
        if (!plan || !index.isValid() || index.column() != 0 || role != Qt::CheckStateRole) return false;
        auto& checked = included[static_cast<std::size_t>(index.row())];
        const bool next = value.toInt() == Qt::Checked;
        if ((checked != 0) == next) return true;
        if (next) ++checkedCount;
        else --checkedCount;
        checked = next;
        const auto& call = plan->calls[static_cast<std::size_t>(index.row())];
        overrides_[{call.id, call.column, call.proposed}] = next;
        emit dataChanged(index, index, {Qt::CheckStateRole, Qt::ToolTipRole});
        return true;
    }

    void show_plan(std::optional<DoublePeakPlan> next = {}) {
        std::vector<unsigned char> checks;
        if (next) {
            checks.reserve(next->calls.size());
            for (const auto& call : next->calls) {
                const auto override = overrides_.find({call.id, call.column, call.proposed});
                checks.push_back(override == overrides_.end() ? !call.needsReview : override->second);
            }
        }
        beginResetModel();
        plan = std::move(next);
        included = std::move(checks);
        checkedCount = std::count(included.begin(), included.end(), 1);
        endResetModel();
    }

    TableReport report() const {
        auto rules = calculation_rules(*plan);
        rules.push_back({"Proposals", QString::number(rowCount())});
        rules.push_back({"Proposals checked for application", QString::number(qulonglong(checkedCount))});
        TextTable rows;
        rows.reserve(plan->calls.size() + 1);
        QStringList headers;
        for (const auto& column : columns) headers.push_back(QString::fromUtf8(column.name));
        rows.push_back(std::move(headers));
        for (int i = 0; i < rowCount(); ++i) {
            QStringList fields;
            fields.reserve(columnCount());
            for (int j = 0; j < columnCount(); ++j) fields.push_back(cell(i, j));
            rows.push_back(std::move(fields));
        }
        return {{"Calculation methods", std::move(rules)},
                {"Double peaks", std::move(rows), {2, 6, 7, 8, 9}}};
    }

    std::optional<DoublePeakPlan> plan;
    std::vector<unsigned char> included;
    std::size_t checkedCount = 0;

private:
    const Document& source_;
    std::map<std::tuple<SequenceId, std::size_t, char>, bool> overrides_;
};

struct PreviewResult {
    std::optional<DoublePeakPlan> plan;
    QString error;
};

class DoublePeakDialog final : public QDialog {
public:
    DoublePeakDialog(std::shared_ptr<Document> source, std::vector<SequenceId> selected,
                     ResiduePalette palette, const CustomResidueColors& colors, QWidget* parent)
        : QDialog(parent), source_(std::move(source)), selected_(std::move(selected)) {
        setObjectName("doublePeakDialog");
        setWindowTitle("Double peak calling");
        resize(1040, 680);

        auto* layout = new QVBoxLayout(this);
        auto* form = new QFormLayout;
        form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        layout->addLayout(form);
        scope_ = new QComboBox(this);
        scope_->setObjectName("doublePeakScope");
        const auto linkedSelected = std::count_if(selected_.begin(), selected_.end(), [&](SequenceId id) {
            return bool(source_->sequence(id).trace);
        });
        scope_->addItem(QString("Selected sequences with chromatograms (%1)").arg(linkedSelected));
        scope_->addItem("All sequences with chromatograms");
        if (!linkedSelected) scope_->setCurrentIndex(1);
        form->addRow("Scope", scope_);
        scope_->setToolTip("Analyze the selected sequences with chromatograms or all sequences with chromatograms.");
        form->labelForField(scope_)->setToolTip(scope_->toolTip());

        const auto percent = [&](const char* name, const QString& label, unsigned value, const QString& tip) {
            auto* spin = new QSpinBox(this);
            spin->setObjectName(name);
            spin->setRange(0, 100);
            spin->setSuffix("%");
            spin->setValue(static_cast<int>(value));
            spin->setToolTip(tip);
            form->addRow(label, spin);
            form->labelForField(spin)->setToolTip(tip);
            return spin;
        };
        const DoublePeakSettings defaults;
        secondary_ = percent("doublePeakSecondary", "Minimum secondary peak height", defaults.secondaryPercent,
            "The secondary peak must reach this percentage of the major peak's height above zero.");
        third_ = percent("doublePeakThird", "Exclude if third peak reaches", defaults.thirdPercent,
            "Skip the site if a third peak reaches this percentage of the major peak's height above zero.");
        displacement_ = percent("doublePeakDisplacement", "Maximum peak displacement", defaults.displacementPercent,
            "The two peak tops must be within this percentage of the typical spacing between nearby bases.");

        excludeEdited_ = new QCheckBox("Exclude hand-edited bases", this);
        excludeEdited_->setObjectName("doublePeakExcludeEdited");
        excludeEdited_->setChecked(defaults.excludeEdited);
        excludeEdited_->setToolTip("Protect manual substitutions, case changes and added calls. Automatic calls can be reviewed again.");
        form->addRow(excludeEdited_);

        auto* ruleToggle = new QToolButton(this);
        ruleToggle->setCheckable(true);
        ruleToggle->setText("Calculation rules");
        ruleToggle->setArrowType(Qt::RightArrow);
        ruleToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        layout->addWidget(ruleToggle, 0, Qt::AlignLeft);
        auto* rules = new QLabel("At each anchored Current residue, the tallest signal must form a clear peak. "
            "Peak heights are measured from the same zero line you see in the trace. "
            "The displacement limit uses the spacing between nearby bases. Phred scores are ignored.", this);
        rules->setWordWrap(true);
        rules->hide();
        layout->addWidget(rules);
        connect(ruleToggle, &QToolButton::toggled, this, [rules, ruleToggle](bool show) {
            rules->setVisible(show);
            ruleToggle->setArrowType(show ? Qt::DownArrow : Qt::RightArrow);
        });

        table_ = new QTableView(this);
        table_->setObjectName("doublePeakPreview");
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::SingleSelection);
        table_->setAlternatingRowColors(true);
        model_ = new PeakTableModel(*source_, table_);
        table_->setModel(model_);
        table_->verticalHeader()->hide();
        table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        table_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        for (int column = 0; column < model_->columnCount(); ++column)
            table_->setColumnWidth(column, columns[static_cast<std::size_t>(column)].width);
        layout->addWidget(table_, 1);

        trace_ = new TraceCanvas(this);
        trace_->setObjectName("doublePeakTrace");
        trace_->setFixedHeight(145);
        trace_->set_editable(false);
        trace_->set_residue_palette(palette, colors);
        trace_->set_zoom(200);
        trace_->setToolTip("Select a proposal above to inspect its chromatogram.");
        layout->addWidget(trace_);
        status_ = new QLabel(this);
        status_->setWordWrap(true);
        status_->setObjectName("doublePeakStatus");
        layout->addWidget(status_);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
        apply_ = buttons->button(QDialogButtonBox::Apply);
        apply_->setObjectName("doublePeakApply");
        apply_->setToolTip("Substitute the current symbols on checked rows.");
        copy_ = buttons->addButton("Copy table", QDialogButtonBox::ActionRole);
        copy_->setObjectName("doublePeakCopy");
        copy_->setToolTip("Copy calculation rules and the full preview as TSV.");
        save_ = buttons->addButton("Save table…", QDialogButtonBox::ActionRole);
        save_->setObjectName("doublePeakSave");
        save_->setToolTip("Save calculation rules and the full preview as XLSX, TSV or CSV.");
        layout->addWidget(buttons);

        timer_.setSingleShot(true);
        timer_.setInterval(120);
        connect(&timer_, &QTimer::timeout, this, [this] { calculate(); });
        connect(excludeEdited_, &QCheckBox::toggled, this, [this] { schedule(); });
        connect(scope_, &QComboBox::currentIndexChanged, this, [this] { schedule(); });
        for (auto* spin : {secondary_, third_, displacement_})
            connect(spin, &QSpinBox::valueChanged, this, [this] { schedule(); });
        connect(model_, &QAbstractItemModel::dataChanged, this, [this] { update_status(); });
        connect(table_->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& index) { show_trace(index); });
        connect(buttons, &QDialogButtonBox::rejected, this, &DoublePeakDialog::reject);
        connect(apply_, &QPushButton::clicked, this, [this] {
            try {
                const auto before = source_->state_id();
                apply_double_peaks(*source_, *model_->plan, model_->included);
                changed = source_->state_id() != before;
                accept();
            } catch (const std::exception& error) {
                status_->setText(QString::fromUtf8(error.what()));
            }
        });
        connect(copy_, &QPushButton::clicked, this, [this] { export_table(false); });
        connect(save_, &QPushButton::clicked, this, [this] { export_table(true); });
        schedule();
    }

    bool changed = false;

protected:
    void reject() override {
        timer_.stop();
        if (!cancel_) {
            QDialog::reject();
            return;
        }
        closing_ = true;
        cancel_->store(true);
        status_->setText("Cancelling double peak preview…");
    }

private:
    DoublePeakSettings settings() const {
        return {scope_->currentIndex() == 1, unsigned(secondary_->value()),
                unsigned(third_->value()), unsigned(displacement_->value()), excludeEdited_->isChecked()};
    }

    void schedule() {
        if (cancel_) cancel_->store(true);
        model_->show_plan();
        trace_->set_sequence(nullptr);
        for (auto* button : {apply_, copy_, save_}) button->setEnabled(false);
        status_->setText("Calculating double peak preview…");
        timer_.start();
    }

    void calculate() {
        if (cancel_) return;
        cancel_ = std::make_shared<std::atomic_bool>(false);
        auto* watcher = new QFutureWatcher<PreviewResult>(this);
        const auto source = source_;
        const auto selected = selected_;
        const auto options = settings();
        const auto cancel = cancel_;
        connect(watcher, &QFutureWatcher<PreviewResult>::finished, this, [this, watcher, cancel] {
            auto result = watcher->future().takeResult();
            watcher->deleteLater();
            cancel_.reset();
            if (closing_) {
                QDialog::reject();
                return;
            }
            if (cancel->load()) {
                if (!timer_.isActive()) timer_.start();
                return;
            }
            if (!result.plan) {
                status_->setText(result.error);
                return;
            }
            if (result.plan->calls.size() > std::size_t(std::numeric_limits<int>::max())) {
                status_->setText("There are too many proposals to display. Select fewer sequences.");
                return;
            }
            model_->show_plan(std::move(result.plan));
            if (model_->rowCount()) table_->selectRow(0);
            update_status();
        });
        watcher->setFuture(QtConcurrent::run([source, selected, options, cancel] {
            PreviewResult result;
            try {
                result.plan = plan_double_peaks(*source, options, selected, [cancel] { return cancel->load(); });
            } catch (const std::exception& error) {
                result.error = QString::fromUtf8(error.what());
            }
            return result;
        }));
    }

    void show_trace(const QModelIndex& index) {
        if (!index.isValid() || !model_->plan) return;
        const auto& call = model_->plan->calls[static_cast<std::size_t>(index.row())];
        trace_->set_sequence(&source_->sequence(call.id));
        trace_->set_selection(call.column, 1, true);
        trace_->focus_column(call.column);
    }

    void update_status() {
        const auto& plan = *model_->plan;
        auto summary = QString("%1 sequences · %2 current residues · %3 proposed · %4 checked · %5 without an anchor · %6 excluded by a third peak")
            .arg(plan.rows).arg(plan.sites).arg(plan.calls.size()).arg(model_->checkedCount)
            .arg(plan.unanchored).arg(plan.thirdPeakExcluded);
        if (plan.editedExcluded) summary += QString(" · %1 edited bases skipped").arg(plan.editedExcluded);
        if (plan.unresolvedMajor)
            summary += QString(" · %1 without a clear major peak").arg(plan.unresolvedMajor);
        status_->setText(summary);
        apply_->setEnabled(model_->checkedCount != 0);
        copy_->setEnabled(true);
        save_->setEnabled(true);
    }

    void export_table(bool save) {
        if (!model_->plan) return;
        try {
            if (!save) {
                QApplication::clipboard()->setText(QString::fromUtf8(report_delimited(model_->report())));
                status_->setText("Copied calculation rules and preview as TSV.");
                return;
            }
            const auto path = choose_table_path(this, "double-peaks.xlsx", source_.get());
            if (path.isEmpty()) return;
            const auto bytes = report_bytes(path, model_->report());
            const auto error = write_export_atomic(path, OutputFormat::Table, source_.get(),
                [&](auto& output) { output.write(bytes.constData(), bytes.size()); });
            status_->setText(error.isEmpty() ? "Double peak preview saved." : error);
        } catch (const std::exception& error) {
            status_->setText(QString::fromUtf8(error.what()));
        }
    }

    std::shared_ptr<Document> source_;
    std::vector<SequenceId> selected_;
    QTimer timer_;
    std::shared_ptr<std::atomic_bool> cancel_;
    bool closing_ = false;
    QComboBox* scope_;
    QCheckBox* excludeEdited_;
    QSpinBox *secondary_, *third_, *displacement_;
    QTableView* table_;
    PeakTableModel* model_;
    TraceCanvas* trace_;
    QLabel* status_;
    QPushButton *apply_, *copy_, *save_;
};
}

void MainWindow::call_double_peaks() {
    if (busy_ || !traces_->has_traces()) return;
    const auto selected = view_->selected_ids();
    set_busy(true, "Reviewing double peaks…");
    DoublePeakDialog dialog(document_, selected, view_->residue_palette(),
                            view_->custom_residue_colors(), this);
    dialog.exec();
    if (dialog.changed) view_->refresh();
    set_busy(false);
}
}
