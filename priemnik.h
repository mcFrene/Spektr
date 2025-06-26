#ifndef PRIEMNIK_H
#define PRIEMNIK_H

#include <QObject>

class Priemnik : public QObject
{
    Q_OBJECT
    bool isRunning;
public:
    explicit Priemnik(QObject *parent = nullptr);
    bool getIsRunning();

signals:
    void finished();
    void sendData(std::vector<int>);

public slots:
    void run();
    void setIsRunning(bool newIsRunning);
};

#endif // PRIEMNIK_H
