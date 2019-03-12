#include "include/mainwindow.h"
#include "ui_mainwindow.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_inlet(nullptr)
{
    ui->setupUi(this);

    ui->comboBox_stream->addItem("LSL Stream", 0);
    ui->comboBox_stream->addItem("IP Stream", 1);
    ui->comboBox_stream->addItem("Serial Stream", 2);
    //setup the objects to connect the lsl layer (button + line edit)
    ui->lineEdi_stream->setText("EEG");
    connect(ui->pushButton_connect_stream, SIGNAL (released()), this, SLOT (connect_stream()));
    connect(ui->comboBox_stream, SIGNAL (currentIndexChanged(int)), this, SLOT(change_stream(int)));

    ui->checkBox_3D->setChecked(true);

    //init the timer object used to update the graph
    QObject::connect(&m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer.setInterval(100);
    m_timer.start();

    std::vector<float> v(m_nb_channels);
    for(int t =0; t < m_time_span; t++)
        m_data.push_back(v);

    init3DGraph();

    init2DGraph();

    // Set the colors from the light theme as default ones
    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Window, QRgb(0xf0f0f0));
    pal.setColor(QPalette::WindowText, QRgb(0x404044));
    qApp->setPalette(pal);



}

void MainWindow::init2DGraph()
{
    m_chart2D = new QtCharts::QChart();
    //chart->setTitle("Line chart");

    //store the m_data to a graphic object
    for (int n = 0 ; n < m_nb_channels; n++) {
        QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D);
        for (unsigned int t = 0; t < m_data.size(); t++)
            series->append(QPointF(t,m_data[t][n]));
        series->setName(QString::number(n));
        m_chart2D->addSeries(series);
    }

    //set up the chart
    m_chart2D->createDefaultAxes();
    m_chart2D->axes(Qt::Horizontal).first()->setRange(-1, m_time_span);
    m_chart2D->axes(Qt::Vertical).first()->setRange(-10, 10);
    QtCharts::QChartView *chartView = new QtCharts::QChartView(m_chart2D);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->horizontalLayout_2D->addWidget(chartView);

}

void MainWindow::init3DGraph()
{
    m_graph = new QtDataVisualization::Q3DSurface();
    QWidget *container = QWidget::createWindowContainer(m_graph);
    container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->horizontalLayout_3D->addWidget(container);


    //store the m_data to a graphic object
    m_proxy_chart = new QtDataVisualization::QSurfaceDataProxy();
    m_chart = new QtDataVisualization::QSurface3DSeries(m_proxy_chart);
    QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
    dataArray->reserve(m_time_span);
    for (int t = 0 ; t < m_time_span ; t++) {
        QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(m_nb_channels);
        for (int n = 0; n < m_nb_channels; n++) {
            (*newRow)[n].setPosition(QVector3D(t, m_data[t][n], n));
        }
        *dataArray << newRow;
    }
    m_proxy_chart->resetArray(dataArray);

    //set up the graph and the chart
    m_chart->setDrawMode(QtDataVisualization::QSurface3DSeries::DrawSurfaceAndWireframe);
    m_chart->setFlatShadingEnabled(true);
    m_graph->addSeries(m_chart);
    m_graph->axisX()->setRange(-1, m_time_span);
    m_graph->axisY()->setRange(-5, 5);
    m_graph->axisZ()->setRange(-1, m_nb_channels);
    m_graph->axisX()->setLabelAutoRotation(30);
    m_graph->axisY()->setLabelAutoRotation(90);
    m_graph->axisZ()->setLabelAutoRotation(30);

    //great a gradient to see the heat map
    QLinearGradient gr;
    gr.setColorAt(0.0, Qt::darkGreen);
    gr.setColorAt(0.5, Qt::yellow);
    gr.setColorAt(0.8, Qt::red);
    gr.setColorAt(1.0, Qt::darkRed);
    m_graph->seriesList().at(0)->setBaseGradient(gr);
    m_graph->seriesList().at(0)->setColorStyle(QtDataVisualization::Q3DTheme::ColorStyleRangeGradient);

     m_graph->scene()->activeCamera()->setCameraPosition(0, 90);
     m_graph->scene()->activeCamera()->setZoomLevel(300);


}

void MainWindow::connect_stream()
{
    ui->pushButton_connect_stream->setDisabled(true);
    ui->lineEdi_stream->setDisabled(true);
    ui->comboBox_stream->setDisabled(true);

    if(ui->comboBox_stream->currentIndex()==0)
    {//LSL stream
        std::string stream_label = ui->lineEdi_stream->text().toStdString();
        std::cout << "Trying to connect to \"" << stream_label << "\""<< std::endl;
        std::vector<lsl::stream_info> results = lsl::resolve_stream("name",stream_label);
        m_inlet = new lsl::stream_inlet(results[0]);
    }
    if(ui->comboBox_stream->currentIndex()==1)
    {// TODO IP stream
        std::string host = ui->lineEdi_stream->text().toStdString();
        m_otb_client.connect(host);
        m_otb_client.start();
    }
    if(ui->comboBox_stream->currentIndex()==2)
    {// TODO Serial stream
    }


}

void MainWindow::handleTimeout()
{
    //if an lsl stream is connected
    if(m_inlet)
    {
        std::vector<float> sample;
        m_inlet->pull_sample(sample);//get the sample
        m_nb_channels = sample.size();
        //store it in our data array
        for(unsigned int n = 0 ; n < sample.size(); n++)
            m_data[(m_t)%m_time_span][n] = sample[n];

        m_t++;
    }

    if(m_otb_client.isRunning())
    {
        m_otb_client.readChannels(m_samples_short);
        m_nb_channels = m_samples_short.size();
        //store it in our data array
        for(unsigned int n = 0 ; n < m_samples_short.size(); n++)
            m_data[(m_t)%m_time_span][n] = m_samples_short[n];
        m_t++;
    }

    //update the 3D graph
    if(ui->checkBox_3D->isChecked())
    {
        //allocate a new surface (in x axis)
        QtDataVisualization::QSurfaceDataArray *dataArray = new QtDataVisualization::QSurfaceDataArray;
        dataArray->reserve(m_time_span);
        for (int t = 0 ; t < m_time_span ; t++) {
            //allocate a new surface (in Z axis
            QtDataVisualization::QSurfaceDataRow *newRow = new QtDataVisualization::QSurfaceDataRow(m_nb_channels);
            for (int n = 0; n < m_nb_channels; n++) {
                //draw a point at the coresponding point
                //the data  array is use as a roundrobin
                (*newRow)[n].setPosition(QVector3D(t+m_t, m_data[((m_t)%m_time_span+t+1)%m_time_span][n], n));
            }
            *dataArray << newRow;
        }

        //update the time window of the graph
        m_graph->axisX()->setRange(m_t-2, m_time_span + m_t+1);
        m_proxy_chart->resetArray(dataArray);
    }

    //update the 2D graph
    if(ui->checkBox_2D->isChecked())
    {
        m_chart2D->removeAllSeries();
        for (int n = 0 ; n < m_nb_channels; n++) {
            QtCharts::QLineSeries *series = new QtCharts::QLineSeries(m_chart2D);
            for (unsigned int t = 0; t < m_data.size(); t++)
                series->append(QPointF(t+m_t, m_data[((m_t)%m_time_span+t+1)%m_time_span][n]));
            series->setName(QString::number(n));
            m_chart2D->addSeries(series);
        }
        m_chart2D->axes(Qt::Horizontal).first()->setRange(m_t-2, m_time_span + m_t+1);
    }



}

void MainWindow::change_stream(int index)
{
    if(index == 0)
    {// Stream
        ui->lineEdi_stream->setPlaceholderText("LSL Stream Label");
        ui->lineEdi_stream->setText("");
        ui->lineEdit_stream_port->setText("");
        ui->lineEdit_stream_port->setPlaceholderText("None");
    }
    else if(index == 1 )
    {// Stream
        ui->lineEdi_stream->setPlaceholderText("IP address hosting OTB software. Ex: localhost");
        ui->lineEdi_stream->setText("");
        ui->lineEdit_stream_port->setText("31000");
    }
    else if(index == 2)
    {// Stream
        ui->lineEdi_stream->setPlaceholderText("Serial Port Name");
        ui->lineEdi_stream->setText("");
        ui->lineEdit_stream_port->setText("");
        ui->lineEdit_stream_port->setPlaceholderText("None");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
