#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <priemnik.h>
#include <QtMath>
#include <QPainterPath>
#include <QGraphicsTextItem>
#include <vector>
#include <QDebug>
#include <thread>

void drawSpectr(Ui::MainWindow* ui, std::vector<double> numbers);
void clearScene(Ui::MainWindow* ui);

bool flag = false; // Вкл/выкл
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
    drawSpectr(ui, numbers);
}

void drawSpectr(Ui::MainWindow* ui, std::vector<double> numbers){
    //return;
    QGraphicsScene* scene = new QGraphicsScene();

    const int xSize = 1024;
    const int ySize = 128;
    const int gridXQty = 16;
    const int gridYQty = 16;

    const int gridStepX = xSize/gridXQty; // шаг сетки по X
    const int gridStepY = ySize/gridYQty; // шаг сетки по Y

    const int width = ui->graphicsView->width() - 3*gridStepX;
    const int height = ui->graphicsView->height() - 3*gridStepY;

    QPen gridPen(Qt::lightGray);
    gridPen.setStyle(Qt::DashLine);

    // Вертикальные линии сетки
    for(int x = 0; x <= xSize; x+=gridStepX){
        qreal scaledX = x * width / xSize;
        scene->addLine(scaledX, 0, scaledX, height, gridPen);
        // Добавим подписи по X
        QGraphicsTextItem* text = scene->addText(QString::number(x));
        text->setPos(scaledX, height + 2);
        text->setDefaultTextColor(Qt::darkGray);
        text->setScale(0.7);
    }

    // Горизонтальные линии сетки
    for(int y = 0; y <= ySize; y += gridStepY){
        qreal scaledY = y * height / ySize;
        scene->addLine(0, scaledY, width, scaledY, gridPen);
        // Подписи по Y (инвертируем координату, чтобы 0 было внизу)
        int val = ySize - y;
        QGraphicsTextItem* text = scene->addText(QString::number(val));
        text->setPos(-30, scaledY - 10);
        text->setDefaultTextColor(Qt::darkGray);
        text->setScale(0.7);
    }

    // Рисуем оси
    QPen axisPen(Qt::black);
    axisPen.setWidth(2);

    // Ось X (по центру по вертикали)
    scene->addLine(0, height, width, height, axisPen);

    // Ось Y (слева)
    scene->addLine(0, 0, 0, height, axisPen);

    // Рисуем график
    QPainterPath path;
    path.moveTo(0, (ySize-numbers[0]) * height / ySize);

    for(int i = 1; i < xSize; i+=4){
        double px = (double)i * width / xSize;
        double py = (ySize - numbers[i]) * height / ySize; //numbers[i] * height / ySize;
        path.lineTo(px, py);
    }


    scene->addPath(path, QPen(Qt::darkGreen, 2));

    scene->setSceneRect(-40, 0, width + gridStepX, height + gridStepY); // с запасом для подписей
    ui->graphicsView->setScene(scene);
}

void MainWindow::clearScene(){
    QGraphicsScene* scene = new QGraphicsScene();
    ui->graphicsView->setScene(scene);
}
