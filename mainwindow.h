#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>

#include "ffmpegplayer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QImage mImage;
    FFmpegPlayer *myplayer;

private:
    void paintEvent(QPaintEvent *event);

private slots:
    void slotGetOneFrame(QImage img);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
