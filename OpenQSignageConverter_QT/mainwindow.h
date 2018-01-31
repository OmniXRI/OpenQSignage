#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <qevent.h>
#include <QCoreApplication>
#include <QPainter>
#include <QFontDialog>
#include <QColorDialog>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <time.h>

#include <opencv2/opencv.hpp>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:

private slots:
    void on_btnDownload_clicked();
    void on_btnPortOpen_clicked();
    void on_btnPortClose_clicked();
    void on_btnCheckComPort_clicked();
    void on_btnLoadF1_clicked();
    void on_btnLoadF2_clicked();
    void on_btnLoadF3_clicked();
    void on_btnLED0_clicked();
    void on_btnLED1_clicked();
    void on_btnGoB1_clicked();
    void on_btnFontB1_clicked();
    void on_btnBGColorB1_clicked();
    void on_btnFGColorB1_clicked();
    void on_btnBGColorB2_clicked();
    void on_btnFGColorB2_clicked();
    void on_btnBGColorB3_clicked();
    void on_btnFGColorB3_clicked();
    void on_btnGoB2_clicked();
    void on_btnFontB2_clicked();
    void on_btnGoB3_clicked();
    void on_btnFontB3_clicked();
    void on_spbF1_valueChanged(double arg1);
    void on_spbF2_valueChanged(double arg1);
    void on_spbF3_valueChanged(double arg1);
    void on_rdbFixB1_clicked();
    void on_rdbBlinkB1_clicked();
    void on_rdbNoneB1_clicked();
    void on_rdbFixB2_clicked();
    void on_rdbBlinkB2_clicked();
    void on_rdbNoneB2_clicked();
    void on_rdbFixB3_clicked();
    void on_rdbBlinkB3_clicked();
    void on_rdbNoneB3_clicked();

private:
    void SetLabelColor(QLabel *lab, QColor bg_color);
    void ShowFrame(cv::Mat imgS, QLabel *imgT);
    void ColorBGR8882RGB565(cv::Mat &imgSrc, cv::Mat &imgTrg);

    Ui::MainWindow *ui;
    QSerialPort *myport; // UART通訊埠

    QString fileName; // 載入影像之檔名
    cv::Mat imgSrc1;  // 原始影像(BGR888)
    cv::Mat imgSrc2;
    cv::Mat imgSrc3;
    cv::Mat imgF1;    // 原始影像縮小後之圖框影像(BGR888)
    cv::Mat imgF2;
    cv::Mat imgF3;
    cv::Mat imgB1;    // 橫幅原始影像(BGR888)
    cv::Mat imgB2;
    cv::Mat imgB3;
    cv::Mat imgLCDF1; // LCD用圖框影像(RGB565)
    cv::Mat imgLCDF2;
    cv::Mat imgLCDF3;
    cv::Mat imgLCDB1; // LCD用橫幅影像(RGB565)
    cv::Mat imgLCDB2;
    cv::Mat imgLCDB3;

    QFont fontB1; // 橫幅字體
    QFont fontB2;
    QFont fontB3;

    QColor BGColorB1; // 橫幅背景色
    QColor BGColorB2;
    QColor BGColorB3;
    QColor FGColorB1; // 橫幅前景(文字)色
    QColor FGColorB2;
    QColor FGColorB3;

    int disp_timerF1; // 顯示停留時間
    int disp_timerB1;
    int disp_timerF2;
    int disp_timerB2;
    int disp_timerF3;
    int disp_timerB3;

    int disp_modeF1;  // 圖框顯示模式
    int disp_modeB1;  // 橫幅顯示模式
    int disp_modeF2;
    int disp_modeB2;
    int disp_modeF3;
    int disp_modeB3;
};

#endif // MAINWINDOW_H
