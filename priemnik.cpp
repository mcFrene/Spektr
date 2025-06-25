#include "priemnik.h"
#include <QDebug>

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
    while (isRunning){
        qDebug() << "hello"<<Qt::endl;
    }
    emit finished();
}
