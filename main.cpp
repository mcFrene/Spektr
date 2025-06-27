#include "mainwindow.h"

#include <QApplication>

int gl = 20;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
