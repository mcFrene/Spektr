#ifndef PRIEMNIK_H
#define PRIEMNIK_H

#include <QObject>
#include <atomic>

class Priemnik : public QObject
{
    Q_OBJECT
public:
    explicit Priemnik(QObject *parent = nullptr);
    std::atomic<bool> isRunning;
signals:
    void sendData(std::vector<double> numbers);
    void priemnikFinished();
    void tcpError();
public slots:
    void socketWork(std::string ip, int port, int freq);
};

#endif // PRIEMNIK_H
