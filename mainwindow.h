#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <priemnik.h>
#include <QThread>
#include <QGraphicsScene>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Priemnik preim{this};
    std::thread* socketThread;
    QGraphicsScene* scene = nullptr;
    QGraphicsPathItem* graphPathItem = nullptr;
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void threadExec(std::string ip, int port, int freq);
    void drawSpectr(std::vector<double> numbers);
    void initScene();
protected:
    void resizeEvent(QResizeEvent* event) override;
signals:
    void priemnikOff();
private slots:
    void on_pushButton_clicked();
    void clearScene();
    //void on_start();
    //void on_stop();
    void receiveData(std::vector<double>);
    //void on_lineEdit_editingFinished();


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
