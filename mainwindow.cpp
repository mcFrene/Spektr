#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtMath>
#include <QPainterPath>
#include <QGraphicsTextItem>
#include <vector>
#include <QDebug>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <uhp_rx_eth.h>
#include <uhp_iq_stream.h>

void drawSpectr(Ui::MainWindow* ui, std::vector<int> numbers);
void clearScene(Ui::MainWindow* ui);
int randomInt(int a, int b);
void socketWork();

void workerFunction() {
    qDebug() << "Hello";
}


bool flag = false; // Вкл/выкл

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

        socketWork();
        //socketThread = new std::thread(workerFunction);
    }
    flag = !flag;
}

void drawSpectr(Ui::MainWindow* ui, std::vector<int> numbers){
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

    // Рисуем график
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

void clearScene(Ui::MainWindow* ui){
    QGraphicsScene* scene = new QGraphicsScene();
    ui->graphicsView->setScene(scene);
}

void socketWork(){
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        qDebug() << "Failed to find Winsock 2.2 or better." << Qt::endl;
        WSACleanup();
        return;
    }

    SOCKET socketTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKET socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


    if (socketTCP == INVALID_SOCKET) {
        qDebug() << "Failed to create tcp_socket: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    if (socketUDP == INVALID_SOCKET) {
        qDebug() << "Failed to create udp_socket: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET; //AF_UNSPEC;
    tcp_addr.sin_addr.s_addr = inet_addr("192.168.21.1");
    tcp_addr.sin_port = htons(5050);


    sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET; //AF_UNSPEC;
    udp_addr.sin_addr.s_addr = 0;
    udp_addr.sin_port = htons(42000);

    bind(socketUDP, (sockaddr *) &udp_addr, sizeof(udp_addr));

    if(connect(socketTCP, (sockaddr *) &tcp_addr, sizeof(tcp_addr)) == SOCKET_ERROR){
        qDebug() << "Failed to connect (tcp): " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    ULONG i_mode = 1;
    if(ioctlsocket(socketTCP, FIONBIO, &i_mode) == SOCKET_ERROR){
        qDebug() << "Failed to configure socket (tcp): " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }


    // head.cmd_type = 0x2;
    // head.messid = 0;
    // head.size = sizeof(head);

    // ETH_RX_CTRL::set_freq setFreq;
    // setFreq.head = head;
    // setFreq.carrier_freq_Hz = 3000000; // минимальное значение


    // if(sendto(socketTCP, (char*)&setFreq, sizeof(setFreq), 0, (sockaddr *) &tcp_addr, sizeof(tcp_addr)) == SOCKET_ERROR){
    //     qDebug() << "Failed status request: " << WSAGetLastError() << Qt::endl;
    //     WSACleanup();
    //     return;
    // }

    fd_set read_s;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;





    ETH_RX_CTRL::ctrl_iq_stream_now stream;
    ETH_RX_CTRL::header_req head;
    head.messid = 0;
    head.cmd_type = 0xC;
    head.size = sizeof(stream);

    stream.head = head;
    stream.IP_stream = 0; //inet_addr("192.168.21.1");
    stream.port_stream = htons(42000);
    stream.preset_num = ETH_RX_CTRL::pres_default;

    if(ioctlsocket(socketUDP, FIONBIO, &i_mode) == SOCKET_ERROR){
        qDebug() << "Failed to configure socket (udp): " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    if(sendto(socketTCP, (char*)&stream, sizeof(stream), 0, (sockaddr *) &tcp_addr, sizeof(tcp_addr)) == SOCKET_ERROR){
        qDebug() << "Failed status request: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    ETH_RX_CTRL::status status_answer;
    UDP_IQ::header_t udpHead;

    int fromlen_tcp = sizeof(tcp_addr);
    int fromlen_udp = sizeof(udp_addr);
    std::vector<char> buffer(2048);
    //char buffer[2048];

    while(true){
        FD_ZERO(&read_s);
        FD_SET(socketTCP, &read_s);
        FD_SET(socketUDP, &read_s);
        select(0, &read_s, NULL, NULL, &tv);

        if(FD_ISSET(socketTCP, &read_s)){
            recvfrom(socketTCP, (char *) &status_answer, sizeof(status_answer), 0, (sockaddr*)&tcp_addr, &fromlen_tcp);
            qDebug() << "recvfrom socketTCP";
        }

        if(FD_ISSET(socketUDP, &read_s)){
            int len = recvfrom(socketUDP, (char *) &buffer, 2048, 0, (sockaddr*)&udp_addr, &fromlen_udp);

            if(len == SOCKET_ERROR){
                qDebug() << "Failed to recvfromUDP: " << WSAGetLastError() << Qt::endl;
                WSACleanup();
                return;
            }

            if (len > 0) {
                qDebug() << "recvfrom socketUDP";
            }
        }
    }

    WSACleanup();
}
