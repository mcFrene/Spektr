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
    Q_OBJECT
    Ui::MainWindow *ui;
    Priemnik p{this};
    std::thread* socketThread;
    QGraphicsScene* scene = nullptr;
    QGraphicsPathItem* graphPathItem = nullptr;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void threadExec(std::string ip, int port, int freq);
    void switchGUI(bool flag);
    void initScene();
protected:
    void resizeEvent(QResizeEvent* event) override;
private slots:
    void on_pushButton_clicked();
    void clearScene();
    void drawSpectr(std::vector<double> numbers);
    void catchError(std::string);
};
#endif // MAINWINDOW_H
