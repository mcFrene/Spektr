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
    Q_OBJECT
    Priemnik priemnik;
    QThread thread;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_start();
    void on_stop();

    //void on_lineEdit_editingFinished();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
