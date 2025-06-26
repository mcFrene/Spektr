#include "priemnik.h"
#include <QDebug>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <uhp_rx_eth.h>



void socketWork();

Priemnik::Priemnik(QObject *parent) : QObject{parent}
{
}

bool Priemnik::getIsRunning(){
    return isRunning;
}

void Priemnik::setIsRunning(bool newIsRunning){
    isRunning = newIsRunning;
}


void Priemnik::run(){
    //qDebug() << "hello"<<Qt::endl;
    //while (isRunning){
    //    qDebug() << "hello"<<Qt::endl;
    //}





    std::vector<int> nums;
    for(int i=0; i<1024; i++){
        nums.push_back(rand() % 1025);
    }

    socketWork();

    emit sendData(nums);
    emit finished();
}


void socketWork(){
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        qDebug() << "Failed to find Winsock 2.2 or better." << Qt::endl;
        WSACleanup();
        return;
    }

    SOCKET socketTCP = socket(AF_INET, SOCK_STREAM, 0);

    if (socketTCP == INVALID_SOCKET) {
        qDebug() << "Failed to create raw socket: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    sockaddr_in s_tcp_addr;
    memset(&s_tcp_addr, 0, sizeof(s_tcp_addr));
    s_tcp_addr.sin_family = AF_INET; //AF_UNSPEC;
    s_tcp_addr.sin_addr.s_addr = inet_addr("192.168.1.2");
    s_tcp_addr.sin_port = htons(5052);

    ULONG i_mode = 1;
    if(ioctlsocket(socketTCP, FIONBIO, &i_mode) == SOCKET_ERROR){
        qDebug() << "Failed to configure socket: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    if(connect(socketTCP, (sockaddr *) &s_tcp_addr, sizeof(s_tcp_addr)) == SOCKET_ERROR){
        qDebug() << "Failed to connect: " << WSAGetLastError() << Qt::endl;
        WSACleanup();
        return;
    }

    ETH_RX_CTRL::header_req head;
    head.cmd_type = 0x1;
    head.messid = 0;
    head.size = sizeof(head);

    sendto(socketTCP, (char*)&head, head.size, 0, (sockaddr *) &s_tcp_addr, sizeof(s_tcp_addr));
}
