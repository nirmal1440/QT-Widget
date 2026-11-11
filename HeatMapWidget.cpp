#include "HeatmapWidget.h"

#include <QPainter>
#include <QLinearGradient>
#include <QSizePolicy>
#include <QImage>
#include <algorithm>
#include <cmath>

namespace {
QVector<QColor> stopsFor(const QString &name)
{
    auto make = [](std::initializer_list<const char *> hex) {
        QVector<QColor> v;
        for (auto h : hex) v.push_back(QColor(h));
        return v;
    };
    if (name == "plasma")   return make({"#0d0887", "#7e03a8", "#cc4778", "#f89540", "#f0f921"});
    if (name == "inferno")  return make({"#000004", "#57106e", "#bc3754", "#f98e09", "#fcffa4"});
    if (name == "magma")    return make({"#000004", "#51127c", "#b73779", "#fc8961", "#fcfdbf"});
    if (name == "coolwarm") return make({"#3b4cc0", "#aac7fd", "#dddddd", "#f4987a", "#b40426"});
    if (name == "RdYlBu")   return make({"#a50026", "#f46d43", "#ffffbf", "#74add1", "#313695"});
    if (name == "hot")      return make({"#000000", "#ff0000", "#ffff00", "#ffffff"});
    return make({"#440154", "#3b528b", "#21918c", "#5ec962", "#fde725"}); // viridis
}
} // namespace

HeatmapWidget::HeatmapWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(true);
}

void HeatmapWidget::setData(const QVector<double> &values, int size)
{
    m_values = values;
    m_n = size;
    if (!values.isEmpty()) {
        auto mm = std::minmax_element(values.begin(), values.end());
        m_min = *mm.first;
        m_max = *mm.second;
        if (m_max - m_min < 1e-12) m_max = m_min + 1.0;
    }
    update();
}

void HeatmapWidget::setColormap(const QString &name) { m_cmap = name; update(); }
void HeatmapWidget::setShowColorbar(bool on)          { m_colorbar = on; update(); }
void HeatmapWidget::setTitle(const QString &title)    { m_title = title; update(); }

QColor HeatmapWidget::colorAt(double t) const
{
    static QString cachedName;
    static QVector<QColor> stops;
    if (cachedName != m_cmap) { stops = stopsFor(m_cmap); cachedName = m_cmap; }

    t = std::clamp(t, 0.0, 1.0);
    const double pos = t * (stops.size() - 1);
    const int i = std::min(int(pos), int(stops.size()) - 2);
    const double f = pos - i;
    const QColor &a = stops[i], &b = stops[i + 1];
    return QColor::fromRgbF(a.redF()   + f * (b.redF()   - a.redF()),
                            a.greenF() + f * (b.greenF() - a.greenF()),
                            a.blueF()  + f * (b.blueF()  - a.blueF()));
}

void HeatmapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().window());
    p.setPen(palette().windowText().color());

    // Margins in pixels; plot rect is whatever is left -> scales with the window
    const int L = 48, T = 30, B = 38, R = m_colorbar ? 84 : 16;
    const QRect plot(L, T, width() - L - R, height() - T - B);
    if (plot.width() < 20 || plot.height() < 20 || m_n == 0) return;

    // Title
    QFont tf = font(); tf.setBold(true);
    p.setFont(tf);
    p.drawText(QRect(L, 0, plot.width(), T), Qt::AlignCenter, m_title);
    p.setFont(font());

    // Heatmap image (origin at bottom-left)
    QImage img(m_n, m_n, QImage::Format_RGB32);
    for (int r = 0; r < m_n; ++r)
        for (int c = 0; c < m_n; ++c) {
            const double t = (m_values[r * m_n + c] - m_min) / (m_max - m_min);
            img.setPixelColor(c, m_n - 1 - r, colorAt(t));
        }
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(plot, img);
    p.drawRect(plot);

    // Axis ticks (5 per axis)
    const QFontMetrics fm(font());
    for (int i = 0; i < 5; ++i) {
        const double v = i * (m_n - 1) / 4.0;
        const QString s = QString::number(int(std::round(v)));
        const int x = plot.left() + int((v + 0.5) / m_n * plot.width());
        const int y = plot.bottom() - int((v + 0.5) / m_n * plot.height());
        p.drawLine(x, plot.bottom(), x, plot.bottom() + 4);
        p.drawText(x - fm.horizontalAdvance(s) / 2, plot.bottom() + 6 + fm.ascent(), s);
        p.drawLine(plot.left() - 4, y, plot.left(), y);
        p.drawText(plot.left() - 7 - fm.horizontalAdvance(s), y + fm.ascent() / 2, s);
    }

    // Colorbar
    if (m_colorbar) {
        const QRect bar(plot.right() + 16, plot.top(), 16, plot.height());
        QLinearGradient g(bar.bottomLeft(), bar.topLeft());
        for (int i = 0; i <= 16; ++i) g.setColorAt(i / 16.0, colorAt(i / 16.0));
        p.fillRect(bar, g);
        p.drawRect(bar);
        for (int i = 0; i < 3; ++i) {
            const double f = i / 2.0;
            const int y = bar.bottom() - int(f * bar.height());
            p.drawLine(bar.right(), y, bar.right() + 4, y);
            p.drawText(bar.right() + 7, y + fm.ascent() / 2,
                       QString::number(m_min + f * (m_max - m_min), 'f', 2));
        }
    }
}
