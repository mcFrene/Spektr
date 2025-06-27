#ifndef PRIEMNIK_H
#define PRIEMNIK_H

#include <QObject>
#include <thread>


class Priemnik : public QObject
{
    Q_OBJECT
public:

    explicit Priemnik(QObject *parent = nullptr);
    bool getIsRunning();

signals:
    void finished();
    void sendData(std::vector<int>);

//public slots:
    //void setIsRunning(bool newIsRunning);
};

#endif // PRIEMNIK_H
