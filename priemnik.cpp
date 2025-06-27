#include "priemnik.h"
#include <QDebug>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <uhp_rx_eth.h>

void socketWork();

Priemnik::Priemnik(QObject *parent) : QObject{parent}
{
}

// bool Priemnik::getIsRunning(){
//     return isRunning;
// }

// void Priemnik::setIsRunning(bool newIsRunning){
//     isRunning = newIsRunning;
// }


// void Priemnik::run(){
//     //qDebug() << "hello"<<Qt::endl;
//     //while (isRunning){
//     //    qDebug() << "hello"<<Qt::endl;
//     //}

//     // while (isRunning){
//     //     std::vector<int> nums;
//     //     for(int i=0; i<1024; i++){
//     //         nums.push_back(rand() % 1025);
//     //     }
//     //     emit sendData(nums);
//     //     emit finished();
//     // }


//     emit finished();
// }
