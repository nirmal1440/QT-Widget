#include "MainWindow.h"
#include "HeatmapWidget.h"
#include "VideoArea.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QFileDialog>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAbstractAxis>

#include <algorithm>
#include <cmath>
#include <random>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Qt Dashboard");
    setMinimumSize(1000, 620);

    // Design size is 1920x1080, but never bigger than the available screen.
    QRect avail(0, 0, 1920, 1080);
    if (screen()) avail = screen()->availableGeometry();
    resize(std::min(1920, avail.width()), std::min(1080, avail.height()));

    auto *central = new QWidget;
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // ---- Row 1: C1 | C2 | C3 (equal width, compact height) ----
    auto *top = new QHBoxLayout;
    top->setSpacing(8);
    top->addWidget(buildC1(), 1);
    top->addWidget(buildC2(), 1);
    top->addWidget(buildC3(), 1);
    root->addLayout(top);

    // ---- Row 2: C5 (left, wide) | C4 (right, tall) ----
    auto *split = new QSplitter(Qt::Horizontal);
    split->setChildrenCollapsible(false);
    split->addWidget(buildC5());
    split->addWidget(buildC4());
    split->setStretchFactor(0, 3);   // C5 gets most of the extra space
    split->setStretchFactor(1, 1);
    split->setSizes({1450, 440});
    root->addWidget(split, 1);       // row 2 takes all remaining height

    statusBar();
    setIndicator(m_ind1, "Link OK", true);
    setIndicator(m_ind2, "Sensor OK", true);
    refresh();
}

// ----------------------------------------------------------- C1 / C2 / C3
// Placeholder content - replace with your real widgets.
QWidget *MainWindow::buildC1()
{
    auto *g = new QGroupBox("C1");
    auto *f = new QFormLayout(g);
    f->addRow("Temperature:", new QLabel("25.3 °C"));
    f->addRow("Pressure:",    new QLabel("1.01 bar"));
    return g;
}

QWidget *MainWindow::buildC2()
{
    auto *g = new QGroupBox("C2");
    auto *l = new QHBoxLayout(g);
    for (const char *name : {"Start", "Stop", "Reset"}) {
        auto *b = new QPushButton(name);
        connect(b, &QPushButton::clicked, this,
                [this, n = QString(name)] { statusBar()->showMessage(n + " pressed", 2000); });
        l->addWidget(b);
    }
    return g;
}

QWidget *MainWindow::buildC3()
{
    auto *g = new QGroupBox("C3");
    auto *l = new QVBoxLayout(g);
    l->addWidget(new QLabel("Task progress"));
    auto *bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setValue(40);
    l->addWidget(bar);
    return g;
}

// -------------------------------------------------------------------- C5
QWidget *MainWindow::buildC5()
{
    auto *frame = new QFrame;
    frame->setFrameShape(QFrame::StyledPanel);
    auto *lay = new QVBoxLayout(frame);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(8);

    // Indicators row (compact, left aligned)
    auto *indRow = new QHBoxLayout;
    for (QLabel **ind : {&m_ind1, &m_ind2}) {
        *ind = new QLabel;
        (*ind)->setTextFormat(Qt::RichText);
        (*ind)->setStyleSheet("QLabel { border:1px solid palette(mid);"
                              " border-radius:11px; padding:2px 12px; }");
        (*ind)->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        indRow->addWidget(*ind);
    }
    indRow->addStretch(1);
    lay->addLayout(indRow);

    // Video area with floating line graph / heatmap
    m_videoArea = new VideoArea;
    m_chartView = new QChartView(new QChart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setRubberBand(QChartView::RectangleRubberBand);   // drag to zoom
    m_heatmap = new HeatmapWidget;
    m_videoArea->addOverlayWidget(m_chartView);   // index 0
    m_videoArea->addOverlayWidget(m_heatmap);     // index 1
    lay->addWidget(m_videoArea, 1);               // takes all spare height

    // Message bar
    auto *msgRow = new QHBoxLayout;
    msgRow->addWidget(new QLabel("Message:"));
    m_msgEdit = new QLineEdit;
    m_msgEdit->setReadOnly(true);
    msgRow->addWidget(m_msgEdit, 1);              // stretches with the window
    m_ackBtn = new QPushButton("ACK");
    m_ackBtn->setEnabled(false);
    m_ackBtn->setMinimumWidth(80);
    connect(m_ackBtn, &QPushButton::clicked, this, &MainWindow::acknowledge);
    msgRow->addWidget(m_ackBtn);
    lay->addLayout(msgRow);
    return frame;
}

// -------------------------------------------------------------------- C4
QWidget *MainWindow::buildC4()
{
    auto *panel = new QWidget;
    auto *lay = new QVBoxLayout(panel);
    lay->setContentsMargins(10, 10, 10, 10);
    lay->setSpacing(12);

    // Overlay selection
    auto *g0 = new QGroupBox("Video area overlay");
    auto *f0 = new QFormLayout(g0);
    f0->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_overlayCombo = new QComboBox;
    m_overlayCombo->addItems({"Line graph", "Heatmap", "None"});
    f0->addRow("Show:", m_overlayCombo);
    lay->addWidget(g0);

    // Line graph
    auto *g1 = new QGroupBox("Line graph");
    auto *f1 = new QFormLayout(g1);
    f1->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_funcCombo = new QComboBox;
    m_funcCombo->addItems({"Sine", "Cosine", "Damped sine", "Random walk"});
    f1->addRow("Signal:", m_funcCombo);
    m_seriesSpin = new QSpinBox;
    m_seriesSpin->setRange(1, 8);
    m_seriesSpin->setValue(3);
    f1->addRow("Series:", m_seriesSpin);
    m_pointsSpin = new QSpinBox;
    m_pointsSpin->setRange(20, 5000);
    m_pointsSpin->setSingleStep(50);
    m_pointsSpin->setValue(300);
    f1->addRow("Points:", m_pointsSpin);
    m_noiseSlider = new QSlider(Qt::Horizontal);
    m_noiseSlider->setRange(0, 100);
    m_noiseSlider->setValue(10);
    m_noiseLabel = new QLabel("0.10");
    connect(m_noiseSlider, &QSlider::valueChanged, this,
            [this](int v) { m_noiseLabel->setText(QString::number(v / 100.0, 'f', 2)); });
    auto *noiseRow = new QHBoxLayout;
    noiseRow->addWidget(m_noiseSlider, 1);
    noiseRow->addWidget(m_noiseLabel);
    f1->addRow("Noise:", noiseRow);
    m_gridCheck = new QCheckBox("Show grid");
    m_gridCheck->setChecked(true);
    f1->addRow(m_gridCheck);
    m_legendCheck = new QCheckBox("Show legend");
    f1->addRow(m_legendCheck);
    lay->addWidget(g1);

    // Heatmap
    auto *g2 = new QGroupBox("Heatmap");
    auto *f2 = new QFormLayout(g2);
    f2->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_cmapCombo = new QComboBox;
    m_cmapCombo->addItems({"viridis", "plasma", "inferno", "magma", "coolwarm", "RdYlBu", "hot"});
    f2->addRow("Colormap:", m_cmapCombo);
    m_gridSizeSpin = new QSpinBox;
    m_gridSizeSpin->setRange(5, 200);
    m_gridSizeSpin->setValue(40);
    f2->addRow("Grid size:", m_gridSizeSpin);
    m_blobsSpin = new QSpinBox;
    m_blobsSpin->setRange(1, 10);
    m_blobsSpin->setValue(3);
    f2->addRow("Hot spots:", m_blobsSpin);
    m_colorbarCheck = new QCheckBox("Show colorbar");
    m_colorbarCheck->setChecked(true);
    f2->addRow(m_colorbarCheck);
    lay->addWidget(g2);

    // Actions
    auto *g3 = new QGroupBox("Actions");
    auto *l3 = new QVBoxLayout(g3);
    auto *bUpdate = new QPushButton("Update plots");
    auto *bZoom   = new QPushButton("Reset graph zoom");
    auto *bEvent  = new QPushButton("Simulate alarm event");
    auto *bShot   = new QPushButton("Save screenshot…");
    for (auto *b : {bUpdate, bZoom, bEvent, bShot}) l3->addWidget(b);
    lay->addWidget(g3);
    lay->addStretch(1);

    connect(bUpdate, &QPushButton::clicked, this, &MainWindow::refresh);
    connect(bZoom, &QPushButton::clicked, this, [this] { m_chartView->chart()->zoomReset(); });
    connect(bEvent, &QPushButton::clicked, this, &MainWindow::simulateEvent);
    connect(bShot, &QPushButton::clicked, this, &MainWindow::saveScreenshot);

    // Auto-update on change
    connect(m_overlayCombo, &QComboBox::currentIndexChanged, this, &MainWindow::applyOverlay);
    connect(m_funcCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refresh);
    connect(m_cmapCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refresh);
    for (QSpinBox *s : {m_seriesSpin, m_pointsSpin, m_gridSizeSpin, m_blobsSpin})
        connect(s, &QSpinBox::valueChanged, this, &MainWindow::refresh);
    for (QCheckBox *c : {m_gridCheck, m_legendCheck, m_colorbarCheck})
        connect(c, &QCheckBox::toggled, this, &MainWindow::refresh);
    connect(m_noiseSlider, &QSlider::sliderReleased, this, &MainWindow::refresh);

    // Scroll instead of squash when the window is short
    auto *scroll = new QScrollArea;
    scroll->setWidget(panel);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::StyledPanel);
    scroll->setMinimumWidth(260);
    scroll->setMaximumWidth(460);
    return scroll;
}

// ------------------------------------------------------------- behaviour
void MainWindow::setIndicator(QLabel *label, const QString &name, bool ok)
{
    label->setText(QString("<span style='color:%1;'>&#9679;</span> %2")
                       .arg(ok ? "#2ecc71" : "#e74c3c", name));
}

void MainWindow::showMessage(const QString &text)
{
    m_msgEdit->setText(text);
    m_ackBtn->setEnabled(!text.isEmpty());
}

void MainWindow::simulateEvent()
{
    setIndicator(m_ind2, "Sensor FAULT", false);
    showMessage("Alarm: sensor B out of range - please acknowledge");
}

void MainWindow::acknowledge()
{
    setIndicator(m_ind2, "Sensor OK", true);
    showMessage(QString());
    statusBar()->showMessage("Message acknowledged", 2000);
}

void MainWindow::applyOverlay()
{
    m_videoArea->showOverlay(m_overlayCombo->currentIndex() == 2
                                 ? -1 : m_overlayCombo->currentIndex());
}

void MainWindow::refresh()
{
    plotLines();
    plotHeatmap();
    applyOverlay();
}

void MainWindow::saveScreenshot()
{
    const QString path = QFileDialog::getSaveFileName(this, "Save screenshot",
                                                      "dashboard.png", "PNG (*.png)");
    if (!path.isEmpty()) {
        grab().save(path);
        statusBar()->showMessage("Saved " + path, 3000);
    }
}

// ------------------------------------------------------------------ data
void MainWindow::plotLines()
{
    std::mt19937 rng{std::random_device{}()};
    const int n = m_pointsSpin->value();
    const double noise = m_noiseSlider->value() / 100.0;
    const QString kind = m_funcCombo->currentText();
    std::normal_distribution<double> noiseDist(0.0, std::max(noise, 1e-9));
    std::normal_distribution<double> stepDist(0.0, 0.15);

    QChart *chart = m_chartView->chart();
    chart->removeAllSeries();
    for (auto *ax : chart->axes()) chart->removeAxis(ax);

    for (int s = 0; s < m_seriesSpin->value(); ++s) {
        auto *series = new QLineSeries;
        series->setName(QString("Series %1").arg(s + 1));
        const double phase = s * 0.6;
        double walk = 0.0;
        QList<QPointF> pts;
        pts.reserve(n);
        for (int i = 0; i < n; ++i) {
            const double x = 4.0 * M_PI * i / (n - 1);
            double y;
            if      (kind == "Sine")        y = std::sin(x + phase);
            else if (kind == "Cosine")      y = std::cos(x + phase);
            else if (kind == "Damped sine") y = std::exp(-x / 6.0) * std::sin(2 * x + phase);
            else { walk += stepDist(rng);   y = walk; }
            if (noise > 0.0) y += noiseDist(rng);
            pts.append({x, y});
        }
        series->replace(pts);
        chart->addSeries(series);
    }

    chart->setTitle("Line graph - " + kind);
    chart->createDefaultAxes();
    for (auto *ax : chart->axes()) ax->setGridLineVisible(m_gridCheck->isChecked());
    chart->legend()->setVisible(m_legendCheck->isChecked());
    chart->legend()->setAlignment(Qt::AlignBottom);
}

void MainWindow::plotHeatmap()
{
    std::mt19937 rng{std::random_device{}()};
    const int n = m_gridSizeSpin->value();
    std::uniform_real_distribution<double> pos(0.15 * n, 0.85 * n);
    std::uniform_real_distribution<double> sigma(0.06 * n, 0.2 * n);
    std::uniform_real_distribution<double> amp(0.5, 1.0);
    std::normal_distribution<double> noise(0.0, 0.03);

    QVector<double> z(n * n, 0.0);
    for (int b = 0; b < m_blobsSpin->value(); ++b) {
        const double cx = pos(rng), cy = pos(rng), s = sigma(rng), a = amp(rng);
        for (int r = 0; r < n; ++r)
            for (int c = 0; c < n; ++c) {
                const double dx = c - cx, dy = r - cy;
                z[r * n + c] += a * std::exp(-(dx * dx + dy * dy) / (2 * s * s));
            }
    }
    for (double &v : z) v += noise(rng);

    m_heatmap->setColormap(m_cmapCombo->currentText());
    m_heatmap->setShowColorbar(m_colorbarCheck->isChecked());
    m_heatmap->setData(z, n);
}
