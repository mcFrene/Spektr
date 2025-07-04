#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <priemnik.h>
#include <QtMath>
#include <QPainterPath>
#include <QGraphicsTextItem>
#include <vector>
#include <QDebug>
#include <thread>
#include <QPainter>

void drawSpectr(Ui::MainWindow* ui, std::vector<double> numbers);

bool flag = false;
const int xSize = 1024;
const int ySize = 128;
const int gridXQty = 16;
const int gridYQty = 16;






// Вкл/выкл
//std::thread* sThread;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(this, SIGNAL(priemnikOff()), &preim, SLOT(priemnikFinish()));
    connect(&preim, SIGNAL(sendData(std::vector<double>)), this, SLOT(receiveData(std::vector<double>)));
    connect(&preim, &Priemnik::clearScene, this, &MainWindow::clearScene);
}

MainWindow::~MainWindow()
{
    if(socketThread->joinable()){
        socketThread->join();
    }
    delete socketThread;
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    if(flag){
        ui->pushButton->setText("Включить");
        ui->lineIP->setEnabled(true);
        ui->linePort->setEnabled(true);
        ui->lineFreq->setEnabled(true);

        preim.isRunning = false;
        socketThread->join();
        delete socketThread;
    }
    else{
        ui->pushButton->setText("Выключить");
        ui->lineIP->setEnabled(false);
        ui->linePort->setEnabled(false);
        ui->lineFreq->setEnabled(false);


        initScene();
        std::string ip = ui->lineIP->text().toStdString();
        int port = ui->linePort->text().toInt();
        int freq = ui->lineFreq->text().toInt();
        preim.isRunning = true;
        socketThread = new std::thread(&MainWindow::threadExec, this, ip, port, freq);
    }
    flag = !flag;
}

void MainWindow::threadExec(std::string ip, int port, int freq)
{
    preim.socketWork(ip, port, freq);
}

void MainWindow::receiveData(std::vector<double> numbers){
    drawSpectr(numbers);
}

void MainWindow::clearScene(){
    ui->graphicsView->setScene(nullptr); // отвязываем сцену от view
    delete scene;
    scene = nullptr;
    graphPathItem = nullptr;
}

void MainWindow::initScene() {
    const int gridStepX = xSize/gridXQty; // шаг сетки по X
    const int gridStepY = ySize/gridYQty; // шаг сетки по Y

    const int width = ui->graphicsView->width() - 3*gridStepX;
    const int height = ui->graphicsView->height() - 3*gridStepY;

    if (scene) {
        ui->graphicsView->setScene(nullptr); // отвязываем сцену от view
        delete scene;
        scene = nullptr;
        graphPathItem = nullptr;
    }

    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    QPen gridPen(Qt::lightGray);
    gridPen.setStyle(Qt::DashLine);

    // Вертикальные линии сетки + подписи
    for(int x = 0; x <= xSize; x += gridStepX){
        qreal scaledX = x * width / xSize;
        scene->addLine(scaledX, 0, scaledX, height, gridPen);

        QGraphicsTextItem* text = scene->addText(QString::number(x));
        text->setPos(scaledX, height + 2);
        text->setDefaultTextColor(Qt::darkGray);
        text->setScale(0.7);
    }

    // Горизонтальные линии сетки + подписи
    for(int y = 0; y <= ySize; y += gridStepY){
        qreal scaledY = y * height / ySize;
        scene->addLine(0, scaledY, width, scaledY, gridPen);

        int val = ySize - y;
        QGraphicsTextItem* text = scene->addText(QString::number(val));
        text->setPos(-30, scaledY - 10);
        text->setDefaultTextColor(Qt::darkGray);
        text->setScale(0.7);
    }

    // Оси
    QPen axisPen(Qt::black);
    axisPen.setWidth(2);
    scene->addLine(0, height, width, height, axisPen); // X
    scene->addLine(0, 0, 0, height, axisPen);          // Y

    scene->setSceneRect(-40, 0, width + gridStepX, height + gridStepY);

    // Создаём пустой график
    graphPathItem = scene->addPath(QPainterPath(), QPen(Qt::darkGreen, 1));
}

void MainWindow::drawSpectr(std::vector<double> numbers){
    const int gridStepX = xSize/gridXQty; // шаг сетки по X
    const int gridStepY = ySize/gridYQty; // шаг сетки по Y

    const int width = ui->graphicsView->width() - 3*gridStepX;
    const int height = ui->graphicsView->height() - 3*gridStepY;

    QPainterPath path;
    path.moveTo(0, (ySize - numbers[0]) * height / ySize);

    for(int i = 1; i < xSize && i < numbers.size(); i++){
        double px = (double)i * width / xSize;
        double py = (ySize - numbers[i]) * height / ySize;
        path.lineTo(px, py);
    }

    //graphPathItem->prepareGeometryChange();
    graphPathItem->setPath(path);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event); // обязательно вызвать базовый обработчик

    // Пересоздаём сцену и сетку с новыми размерами
    if(flag){
        initScene();
    }
}

