#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <QPainterPath>
#include <QGraphicsTextItem>
#include <vector>

bool flag = false; // Вкл/выкл

void drawSpectr(Ui::MainWindow* ui);
int randomInt(int a, int b);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&thread, &QThread::started, &priemnik, &Priemnik::run);
    connect(&priemnik, &Priemnik::finished, &thread, &QThread::terminate);
}

void MainWindow::on_start(){
    priemnik.setIsRunning(true);
    thread.start();
}

void MainWindow::on_stop(){
    priemnik.setIsRunning(false);
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    if(flag){
        ui->pushButton->setText("Включить");
        ui->lineIP->setEnabled(true);
        ui->linePort->setEnabled(true);
        ui->lineFreq->setEnabled(true);
    }
    else{
        ui->pushButton->setText("Выключить");
        ui->lineIP->setEnabled(false);
        ui->linePort->setEnabled(false);
        ui->lineFreq->setEnabled(false);

        on_start();
        drawSpectr(ui);
        on_stop();
    }
    flag = !flag;
}



void drawSpectr(Ui::MainWindow* ui){
    QGraphicsScene* scene = new QGraphicsScene();

    const int pointsQty = 1024;
    const int gridXQty = 16;
    const int gridYQty = 16;

    const int gridStepX = pointsQty/gridXQty; // шаг сетки по X
    const int gridStepY = pointsQty/gridYQty; // шаг сетки по Y

    const int width = ui->graphicsView->width() - 2*gridStepX;
    const int height = ui->graphicsView->height() - 2*gridStepY;

    QPen gridPen(Qt::lightGray);
    gridPen.setStyle(Qt::DashLine);

    // Вертикальные линии сетки
    for(int x = 0; x <= pointsQty; x+=gridStepX){
        qreal scaledX = x * width / pointsQty;
        scene->addLine(scaledX, 0, scaledX, height, gridPen);
        // Добавим подписи по X
        QGraphicsTextItem* text = scene->addText(QString::number(x));
        text->setPos(scaledX, height + 2);
        text->setDefaultTextColor(Qt::darkGray);
        text->setScale(0.7);
    }

    // Горизонтальные линии сетки
    for(int y = 0; y <= pointsQty; y += gridStepY){
        qreal scaledY = y * height / pointsQty;
        scene->addLine(0, scaledY, width, scaledY, gridPen);
        // Подписи по Y (инвертируем координату, чтобы 0 было внизу)
        int val = pointsQty - y;
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

    std::vector<int> numbers;

    for(int i=0; i<pointsQty; i++){
        numbers.push_back(randomInt(0, 1024));
    }

    // Рисуем график синуса
    QPainterPath path;
    path.moveTo(0, numbers[0]);

    for(int i = 1; i < pointsQty; i++){
        double px = (double)i * width / pointsQty;
        double py = numbers[i] * height / pointsQty;
        path.lineTo(px, py);
    }

    scene->addPath(path, QPen(Qt::blue, 2));

    scene->setSceneRect(-40, 0, width + 50, height + 30); // с запасом для подписей
    ui->graphicsView->setScene(scene);
}


int randomInt(int a, int b){
    return rand() % (b - a + 1) + a;
}
