
#pragma once
#include <QMainWindow>
#include <QtCharts/QChartView>
 
class QComboBox;
class QSpinBox;
class QSlider;
class QLabel;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QWidget;
class QGroupBox;
class HeatmapWidget;
class VideoArea;
 
// Layout (matches the sketch):
//
//  +---------+---------+---------+
//  |   C1    |   C2    |   C3    |
//  +---------+---------+---------+
//  | C5                | C4      |
//  |  [ind1] [ind2]    |         |
//  |  Video area /     | (side   |
//  |  graphs overlay   | controls|
//  |  Message:  [ACK]  |         |
//  +-------------------+---------+
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
 
public slots:
    void showMessage(const QString &text);   // fills the message bar, enables ACK
 
private slots:
    void refresh();
    void applyOverlay();
    void acknowledge();
    void simulateEvent();
    void saveScreenshot();
 
private:
    QWidget *buildC1();
    QWidget *buildC2();
    QWidget *buildC3();
    QWidget *buildC4();   // right column
    QWidget *buildC5();   // indicators + video/graphs + message bar
 
    void setIndicator(QLabel *label, const QString &name, bool ok);
    void plotLines();
    void plotHeatmap();
 
    // C5
    QLabel *m_ind1 = nullptr, *m_ind2 = nullptr;
    VideoArea *m_videoArea = nullptr;
    QChartView *m_chartView = nullptr;
    HeatmapWidget *m_heatmap = nullptr;
    QLineEdit *m_msgEdit = nullptr;
    QPushButton *m_ackBtn = nullptr;
    // C4
    QComboBox *m_overlayCombo = nullptr, *m_funcCombo = nullptr, *m_cmapCombo = nullptr;
    QSpinBox *m_seriesSpin = nullptr, *m_pointsSpin = nullptr,
             *m_gridSizeSpin = nullptr, *m_blobsSpin = nullptr;
    QSlider *m_noiseSlider = nullptr;
    QLabel *m_noiseLabel = nullptr;
    QCheckBox *m_gridCheck = nullptr, *m_legendCheck = nullptr, *m_colorbarCheck = nullptr;
};
