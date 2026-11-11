#pragma once
#include <QWidget>
#include <QVector>
#include <QString>

// Lightweight heatmap widget (Qt Charts has no heatmap type).
// Draws an n x n grid, axis ticks, title and optional colorbar.
// Everything is computed from the widget's current size in paintEvent,
// so it re-scales cleanly when the window is resized.
class HeatmapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HeatmapWidget(QWidget *parent = nullptr);

    void setData(const QVector<double> &values, int size);   // row-major, size*size
    void setColormap(const QString &name);
    void setShowColorbar(bool on);
    void setTitle(const QString &title);

    QSize minimumSizeHint() const override { return {250, 180}; }
    QSize sizeHint() const override { return {500, 350}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor colorAt(double t) const;

    QVector<double> m_values;
    int m_n = 0;
    double m_min = 0.0, m_max = 1.0;
    QString m_cmap = "viridis";
    QString m_title = "Heatmap";
    bool m_colorbar = true;
};
