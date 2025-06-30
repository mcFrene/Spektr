#ifndef PRIEMNIK_H
#define PRIEMNIK_H

#include <QObject>
#include <thread>

class Priemnik : public QObject
{
    Q_OBJECT
    //std::thread* socketThread;
public:
    explicit Priemnik(QObject *parent = nullptr);

    //bool getIsRunning();

signals:
    //void finished();
    void sendData(double* numbers);

public slots:
    //void priemnikStart();
    void socketWork();
};

#endif // PRIEMNIK_H
