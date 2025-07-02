#ifndef PRIEMNIK_H
#define PRIEMNIK_H

#include <QObject>
#include <atomic>

class Priemnik : public QObject
{
    Q_OBJECT
    //bool isRunning;
    std::atomic<bool> isRunning;

public:
    explicit Priemnik(QObject *parent = nullptr);
    bool getIsRunning();
signals:
    void sendData(std::vector<double> numbers);

public slots:
    void priemnikFinish();
    void socketWork(std::string ip, int port, int freq);
};

#endif // PRIEMNIK_H
