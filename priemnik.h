#ifndef PRIEMNIK_H
#define PRIEMNIK_H

#include <QObject>
#include <atomic>

class Priemnik : public QObject
{
    Q_OBJECT
    //bool isRunning;


public:
    explicit Priemnik(QObject *parent = nullptr);
    bool getIsRunning();
     std::atomic<bool> isRunning;
signals:
    void sendData(std::vector<double> numbers);
    void clearScene();

public slots:
    void priemnikFinish();
    void socketWork(std::string ip, int port, int freq);
};

#endif // PRIEMNIK_H
