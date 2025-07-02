#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<priemnik.h>
#include<QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Priemnik preim;
    std::thread* socketThread;
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
signals:;
    void priemnikOff();
private slots:
    void on_pushButton_clicked();
    //void on_start();
    //void on_stop();
    void receiveData(std::vector<double>);
    //void on_lineEdit_editingFinished();


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
