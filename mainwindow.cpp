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

bool flag = false;
const int xSize = 1024;
const int ySize = 128;
const int gridXQty = 16;
const int gridYQty = 16;
const int gridStepX = xSize/gridXQty; // шаг сетки по X
const int gridStepY = ySize/gridYQty; // шаг сетки по Y

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&p, SIGNAL(sendData(std::vector<double>)), this, SLOT(drawSpectr(std::vector<double>)));
    connect(&p, &Priemnik::priemnikFinished, this, &MainWindow::clearScene);
    connect(&p, &Priemnik::throwError, this, &MainWindow::catchError);
}

MainWindow::~MainWindow()
{
    if(socketThread->joinable()){
        socketThread->join();
    }
    delete socketThread;
    delete scene;
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    switchGUI(flag);
    if(flag){
        p.isRunning = false;
        socketThread->join();
        delete socketThread;
    }
    else{
        initScene();
        std::string ip = ui->lineIP->text().toStdString();
        int port = ui->linePort->text().toInt();
        int freq = ui->lineFreq->text().toInt();

        p.isRunning = true;
        socketThread = new std::thread(&MainWindow::threadExec, this, ip, port, freq);
    }
    flag = !flag;
}

void MainWindow::switchGUI(bool flag)
{
    flag ? ui->pushButton->setText("Включить") : ui->pushButton->setText("Выключить");
    ui->lineIP->setEnabled(flag);
    ui->linePort->setEnabled(flag);
    ui->lineFreq->setEnabled(flag);
    ui->errorLabel->setText("");
}

void MainWindow::threadExec(std::string ip, int port, int freq)
{
    p.socketWork(ip, port, freq);
}

void MainWindow::clearScene(){
    ui->graphicsView->setScene(nullptr); // отвязываем сцену от view
    delete scene;
    scene = nullptr;
    graphPathItem = nullptr;
}

void MainWindow::initScene() {
    const int width = ui->graphicsView->width() - 3*gridStepX;
    const int height = ui->graphicsView->height() - 3*gridStepY;

    if (scene) clearScene();

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
    const int width = ui->graphicsView->width() - 3*gridStepX;
    const int height = ui->graphicsView->height() - 3*gridStepY;

    QPainterPath path;
    path.moveTo(0, (ySize - numbers[0]) * height / ySize);

    for(int i = 1; i < (int)numbers.size(); i++){
        double px = (double)i * width / xSize;
        double py = (ySize - numbers[i]) * height / ySize;
        path.lineTo(px, py);
    }

    graphPathItem->setPath(path);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if(flag) initScene();
}

void MainWindow::catchError(std::string error){
    clearScene();
    switchGUI(true);
    flag = false;

    QPalette palette = ui->errorLabel->palette();
    palette.setColor(ui->errorLabel->foregroundRole(), Qt::red);
    ui->errorLabel->setPalette(palette);

    ui->errorLabel->setText(QString::fromStdString(error));
}
