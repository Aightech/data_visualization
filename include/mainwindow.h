#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtDataVisualization>
#include <QSurfaceDataProxy>
/*#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/QSurfaceDataProxy>
#include <QtDataVisualization/QHeightMapSurfaceDataProxy>
#include <QtDataVisualization/QSurface3DSeries>*/

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <Q3DSurface>
#include <QMainWindow>
#include <QtCore/QTimer>

#include <lsl_cpp.h>
#include <vector>
#include <iostream>

#include "include/OTBioLabClient.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connect_stream();
    void change_stream(int);

private:
    void handleTimeout();
    void init3DGraph();
    void init2DGraph();

    Ui::MainWindow *ui;
    QTimer m_timer;
    lsl::stream_inlet * m_inlet;
    int m_t=100;

    int m_time_span=20;
    int m_nb_channels=8;

    QtCharts::QChart *m_chart2D;

    std::vector<std::vector<float>> m_data;
    std::vector<short> m_samples_short;

    QtDataVisualization::Q3DSurface *m_graph;

    QtDataVisualization::QSurface3DSeries *m_chart;
    QtDataVisualization::QSurfaceDataProxy *m_proxy_chart;

    OTBioLabClient m_otb_client;

};

#endif // MAINWINDOW_H
