#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;
using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);

    ui->pbxF1->setScaledContents(true); // 影像自動縮放至Label尺寸
    ui->pbxF2->setScaledContents(true); // 影像自動縮放至Label尺寸
    ui->pbxF3->setScaledContents(true); // 影像自動縮放至Label尺寸

    fontB1 = fontB2 = fontB3 = QFont("Times", 14, QFont::Bold); // 橫幅字體預設值為Times 14 粗體字
    BGColorB1 = BGColorB2 = BGColorB3 = Qt::white;              // 橫幅背景色預設為白色
    FGColorB1 = FGColorB2 = FGColorB3 = Qt::black;              // 橫幅前景(文字)色預設為黑色
    SetLabelColor(ui->labBGColorB1, Qt::white); SetLabelColor(ui->labFGColorB1, Qt::black); // 預設橫幅一標籤為白底黑字
    SetLabelColor(ui->labBGColorB2, Qt::white); SetLabelColor(ui->labFGColorB2, Qt::black); // 預設橫幅二標籤為白底黑字
    SetLabelColor(ui->labBGColorB3, Qt::white); SetLabelColor(ui->labFGColorB3, Qt::black); // 預設橫幅三標籤為白底黑字

    disp_timerF1 = disp_timerF2 = disp_timerF3 = disp_timerB1 = disp_timerB2 = disp_timerB3 = 30; // 預設排程停留時間3.0 sec
    disp_modeF1 = disp_modeF2 = disp_modeF3 = disp_modeB1 = disp_modeB2 = disp_modeB3 = 0;        // 預設顯示模式皆為固定顯示模式
}

MainWindow::~MainWindow()
{
    if(myport->isOpen()) // 若COM埠還開著
        myport->close(); // 就關閉 

    delete ui;
}

// 將cv::Mat BGR888轉換為RGB888再顯示在QLabel上
// imgS 輸入影像
// imgT 輸出標籤
void MainWindow::ShowFrame(cv::Mat imgS, QLabel *imgT)
{
    Mat imgC;

    cvtColor(imgS,imgC,cv::COLOR_BGR2RGB); // 轉換色彩由BGR888到RGB888

    QImage tmp(imgC.data,
               imgC.cols,
               imgC.rows,
               //imgC.step,
               QImage::Format_RGB888);
    imgT->setPixmap(QPixmap::fromImage(tmp)); // 設定QImage影像到QLabel
}

// 設定標籤背景及文字顏色(色彩反相)
void MainWindow::SetLabelColor(QLabel *lab, QColor bg_color)
{
    QPalette palette;
    QColor fg_color;
    int r,g,b;

    bg_color.getRgb(&r,&g,&b);                         // 取得標籤目前背景顏色
    fg_color = QColor(255-r,255-g,255-b);              // 產生標籤前景(文色)反相顏色
    palette = lab->palette();                          // 為標籤指定色盤
    palette.setColor(lab->backgroundRole(), bg_color); // 設定色盤背景色
    palette.setColor(lab->foregroundRole(), fg_color); // 設定色盤前景色
    lab->setAutoFillBackground(true);                  // 設定自動填滿背景色(一定要設)
    lab->setPalette(palette);                          // 設定調色盤，變更標籤前背景色
}

// 將來源影像cv::Mat(BGR888)轉成LCD顯示用影像(RGB565)
// imrSrc: BGR888_24bit:[B7][B6][B5][B4][B3][B2][B1][B0]
// (CV_8UC3)            [G7][G6][G5][G4][G3][G2][G1][G0]
//                      [R7][R6][R5][R4][R3][R2][R1][R0]
// imgTrg: RGB565_16bit:[R7][R6][R5][R4][R3][G7][G6][G5]
// (CV_8UC2)            [G4][G3][G2][B7][B6][B5][B4][B3]
void MainWindow::ColorBGR8882RGB565(Mat &imgSrc, Mat &imgTrg)
{ 
 unsigned char *ptrS;
 unsigned char *ptrT;
 int posS, posT;
 
 if((imgSrc.cols != imgTrg.cols) || (imgSrc.rows != imgTrg.rows)){ // 確認輸入和輸出影像尺寸要相符
     QMessageBox::critical(this,"Error","Image size different!");
     return;
 } 

 for(int i=0; i<imgSrc.rows; i++){        // 設定迴圈數為影像高度
     ptrS = imgSrc.ptr<unsigned char>(i); // 取得來源影像第i列(row)起始位置指標
     ptrT = imgTrg.ptr<unsigned char>(i); // 取得目標影像第i列(row)起始位置指標
     
     for(int j=0,posS=0,posT=0; j<imgSrc.cols; j++,posS+=3, posT+=2){
         ptrT[posT] = (ptrS[posS+2] & 0xF8) | (ptrS[posS+1] >> 5);        // RGB565高位元組(R5+G3)
         ptrT[posT+1] = ((ptrS[posS+1] & 0x1C) << 3) | (ptrS[posS] >> 3); // RGB565低位元組(G3+B5)
     }   
 }
}

// 以UART傳送點亮LED命令
void MainWindow::on_btnLED1_clicked()
{
    char cmd = 0xA5;     // 點亮LED命令
    myport->write(&cmd); // 透過UART送出命令
}

// 以UART傳送熄滅LED命令
void MainWindow::on_btnLED0_clicked()
{
    char cmd = 0xB4;     // 熄滅LED命令
    myport->write(&cmd); // 透過UART送出命令
}

// 以UART將圖框排程及影像內容傳送到Arduino
void MainWindow::on_btnDownload_clicked()
{
 double t0, t1, t2;
 QString str;
 vector<cv::Mat> vecS;
 vector<cv::Mat> vecT;
 int total_frame;
 int frame_w = ui->txbFrameW->text().toInt(); // 取得圖框寬度
 int frame_h = ui->txbFrameH->text().toInt(); // 取得圖框高度
 int d_mode[6] = {disp_modeF1, disp_modeB1, disp_modeF2, disp_modeB2, disp_modeF3, disp_modeB3}; // 圖框(橫幅)顯示模式
 int d_timer[6] = {disp_timerF1, disp_timerB1, disp_timerF2, disp_timerB2, disp_timerF3, disp_timerB3}; // 圖框(橫幅)顯示停留時間

 t0 = (double)clock();    // 啟動計時器
 str = "display mode = ";

 for(int i=0; i<6; i++){
     str += (QString::number(d_mode[i]) + ", ");
 }
 ui->txbRxData->append(str); // 將待傳送「顯示模式」內容顯示在狀態框

 str = "dwell time = ";
 for(int i=0; i<6; i++){
     str += (QString::number(d_timer[i]) + ", ");
 }
 ui->txbRxData->append(str); // 將待傳送「停留時間」內容顯示在狀態框

 cv::resize(imgSrc1,imgF1,imgF1.size()); // 重新取得圖框內容並縮放到LCD尺寸(像素)
 cv::resize(imgSrc2,imgF2,imgF2.size()); // 避免橫幅蓋掉圖框
 cv::resize(imgSrc3,imgF3,imgF3.size());

 vecS.push_back(imgF1);    // 將圖框(橫幅)推入影像陣列vecS
 vecS.push_back(imgB1);
 vecS.push_back(imgF2);
 vecS.push_back(imgB2);
 vecS.push_back(imgF3);
 vecS.push_back(imgB3);

 vecT.push_back(imgLCDF1); // 將LCD轉換結果影像推入影像陣列vecT
 vecT.push_back(imgLCDB1);
 vecT.push_back(imgLCDF2);
 vecT.push_back(imgLCDB2);
 vecT.push_back(imgLCDF3);
 vecT.push_back(imgLCDB3);

 total_frame = vecT.size(); // 取得圖框(橫幅)數量
 t1 = (double)clock();      // 取得時間1
 
 // 計算所有傳輸資料量(位元組)
 long total_size = (imgF1.cols * imgF1.rows * 2 * (total_frame/2)) + (imgB1.cols * imgB1.rows * 2 * (total_frame/2));
 long curr_size = 0;  // 目前已傳輸資料量(位元組)
 char cmd = 0xC3;     // UART開始傳送資料命令
 char ack;            // Arduino回應

 myport->clear();     // 清除通訊埠(COM)
 myport->write(&cmd); // 透過UART送出命令

 for(int f=0; f<total_frame; f++){  // 迴圈設為欲傳送之圖框數量    
    char frame_format[10];          // 配置圖框排程資料區

    frame_format[0] = vecT[f].cols >> 8;     // 圖框(橫幅)寬度高位元組
    frame_format[1] = vecT[f].cols & 0x00ff; // 圖框(橫幅)寬度低位元組
    frame_format[2] = vecT[f].rows >> 8;     // 圖框(橫幅)高度高位元組
    frame_format[3] = vecT[f].rows & 0x00ff; // 圖框(橫幅)高度低位元組
    frame_format[4] = (frame_w - vecT[f].cols) >> 8;     // 顯示起始位置sx高位元組
    frame_format[5] = (frame_w - vecT[f].cols) & 0x00ff; // 顯示起始位置sx低位元組
    frame_format[6] = (frame_h - vecT[f].rows) >> 8;     // 顯示起始位置sy高位元組
    frame_format[7] = (frame_h - vecT[f].rows) & 0x00ff; // 顯示起始位置sy低位元組
    frame_format[8] = d_mode[f];  // 顯示模式， 0:固定顯示， 1:交替閃爍， 2:不顯示
    frame_format[9] = d_timer[f]; // 顯示停留時間，0.0 ~ 25.5 sec轉換成 0 ~ 255

    myport->write(frame_format, 10); // 透過UART寫入10位元組

    ack = 0; // 清除回應值

    if(myport->waitForReadyRead(3000)){  // 等待回應(最多等3秒)
        myport->read(&ack,1);            // 取得回應值
    }

    char *ptrS;

    ColorBGR8882RGB565(vecS[f], vecT[f]);   // 轉換RGB888成為RGB565

    for(int i=0; i<vecT[f].rows; i++){      // 設定迴圈數量為影像高度
        ptrS = vecT[f].ptr<char>(i);        // 取得LCD影像第i列(row)起始位置指標
        myport->write(ptrS,vecT[f].cols*2); // 一次送出一列資料 (最大不可超過1K位元組)

        ack = 0; // 清除回應值 

        if(myport->waitForReadyRead(3000)){  // 等待回應(最多等3秒)
            myport->read(&ack,1);            // 取得回應值
        }

        curr_size += (vecT[f].cols*2);       // 目前傳送值加上影像寬度*2

        ui->pgbDownload->setValue(((i+1)/(vecT[f].rows*1.0))*100); // 計算單張影像傳送百分比
        ui->pgbDownload->update();                                 // 更新單張影像下載進度條

        ui->pgbTotal->setValue((curr_size/(total_size*1.0))*100);  // 計算所有影像傳送百分比
        ui->pgbTotal->update();                                    // 更新所有影像下載進度條
    }
 }

 t2 = (double)clock(); // 取得目前時間
 str = "Image convert time = " + QString::number((t1-t0)/1000.0,'f',3) + "second.";  // 計算影像轉換時間
 ui->txbRxData->append(str);                                                         // 顯示於狀態框
 str = "Image download time = " + QString::number((t2-t1)/1000.0,'f',3) + "second."; // 計算影像下載時間
 ui->txbRxData->append(str);                                                         // 顯示於狀態框
 ui->txbRxData->append("Transfer done!");                                            // 顯示完成訊息於狀態框
 QMessageBox::information(this,tr("訊息"),tr("圖像下載完成!"));                      // 顯示完成訊息盒
}

// 檢查系統COM數量
void MainWindow::on_btnCheckComPort_clicked()
{
    // 取得所有可用的COM埠名稱表列
    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();

    if(serialPortInfoList.size() == 0)              // 若找不到任何COM埠
        ui->txbRxData->append("No find COM Port!"); // 在狀態框顯示找不到訊息
    else{                                           // 若有找到COM埠
        for (int i = 0; i < serialPortInfoList.size(); i++) {              // 設定迴圈數為COM埠數量
            ui->txbPortName->setText(serialPortInfoList.at(i).portName()); // 將COM埠名稱顯示於文字盒
            ui->txbPortDescription->setText(serialPortInfoList.at(i).description()); // 將COM埠描述顯示於文字盒
        }

        ui->btnPortOpen->setEnabled(true);  // 致能[開啟通訊埠]按鍵
    }
}

// 開啟通訊埠
void MainWindow::on_btnPortOpen_clicked()
{
    myport = new QSerialPort(ui->txbPortName->text());  // 依檢查到的埠名開啟COM埠

    myport->setBaudRate(QSerialPort::Baud57600);        // 設定通訊鮑率 57,600bps
    myport->setDataBits(QSerialPort::Data8);            // 設定傳輸字元為8位元
    myport->setParity(QSerialPort::NoParity);           // 設定為無同位元
    myport->setStopBits(QSerialPort::OneStop);          // 設定停止位元為1
    myport->setFlowControl(QSerialPort::NoFlowControl); // 設定無流量控制

    if (!myport->open(QIODevice::ReadWrite)){               // 若COM埠開啟失敗
        ui->txbRxData->append("serial port open failed");   // 顯示訊息於狀態框
        ui->btnPortOpen->setEnabled(false);                 // 禁能[開啟通訊埠]按鍵
        ui->btnPortClose->setEnabled(false);                // 禁能[關閉通訊埠]按鍵
        ui->btnLED0->setEnabled(false);                     // 禁能[LED Off]按鍵
        ui->btnLED1->setEnabled(false);                     // 禁能[LED On]按鍵
        ui->btnDownload->setEnabled(false);                 // 禁能[下載」按鍵
    }
    else{                                                   // 若COM埠開啟成功
        ui->txbRxData->append("serial port open sucessed"); // 顯示訊息於狀態框 
        ui->btnPortOpen->setEnabled(false);                 // 禁能[開啟通訊埠]按鍵(不可重覆開啟)
        ui->btnPortClose->setEnabled(true);                 // 致能[關閉通訊埠]按鍵
        ui->btnLED0->setEnabled(true);                      // 致能[LED Off]按鍵
        ui->btnLED1->setEnabled(true);                      // 致能[LED On]按鍵
        ui->btnDownload->setEnabled(true);                  // 致能[下載」按鍵
    }
}

// 關閉通訊埠
void MainWindow::on_btnPortClose_clicked()
{
    if(myport->isOpen()){                                     // 若通訊埠已開啟
        myport->close();                                      // 關閉通訊埠
        ui->txbRxData->append("serial port close sucessed");  // 顯示訊息於狀態框 
        ui->btnPortOpen->setEnabled(true);                    // 致能[開啟通訊埠]按鍵
        ui->btnPortClose->setEnabled(false);                  // 禁能[關閉通訊埠]按鍵
        ui->btnLED0->setEnabled(false);                       // 禁能[LED Off]按鍵
        ui->btnLED1->setEnabled(false);                       // 禁能[LED On]按鍵
        ui->btnDownload->setEnabled(false);                   // 禁能[下載」按鍵
    }
}

// 載入圖框影像一
void MainWindow::on_btnLoadF1_clicked()
{
    fileName = QFileDialog::getOpenFileName(this,tr("Open File")); // 開啟檔案對話盒並取得檔名
    imgSrc1 = cv::imread(fileName.toStdString(),IMREAD_COLOR);     // 讀入影像

     if(!imgSrc1.empty()) {                       // 若影像不空
         int fw = ui->txbFrameW->text().toInt();  // 取得圖框寬度
         int fh = ui->txbFrameH->text().toInt();  // 取得圖框高度
         int bw = ui->txbBannerW->text().toInt(); // 取得橫幅寬度
         int bh = ui->txbBannerH->text().toInt(); // 取得橫幅高度

         cv::resize(imgSrc1,imgF1,cvSize(fw,fh));        // 將原始影像縮到指定圖框尺寸
         imgLCDF1 = Mat::zeros(cv::Size(fw,fh),CV_8UC2); // 清除圖框影像緩衝區
         imgLCDB1 = Mat::zeros(cv::Size(bw,bh),CV_8UC2); // 清除橫幅影像緩衝區
         ShowFrame(imgF1, ui->pbxF1);                    // 將圖框影像秀在標籤上
         ui->btnGoB1->setEnabled(true);                  // 致能[更新]按鍵
     }
}

// 載入圖框影像二
void MainWindow::on_btnLoadF2_clicked()
{
    fileName = QFileDialog::getOpenFileName(this,tr("Open File")); // 開啟檔案對話盒並取得檔名
    imgSrc2 = cv::imread(fileName.toStdString(),IMREAD_COLOR);     // 讀入影像

     if(!imgSrc2.empty()) {                       // 若影像不空
         int fw = ui->txbFrameW->text().toInt();  // 取得圖框寬度
         int fh = ui->txbFrameH->text().toInt();  // 取得圖框高度
         int bw = ui->txbBannerW->text().toInt(); // 取得橫幅寬度
         int bh = ui->txbBannerH->text().toInt(); // 取得橫幅高度

         cv::resize(imgSrc2,imgF2,cvSize(fw,fh));        // 將原始影像縮到指定圖框尺寸
         imgLCDF2 = Mat::zeros(cv::Size(fw,fh),CV_8UC2); // 清除圖框影像緩衝區
         imgLCDB2 = Mat::zeros(cv::Size(bw,bh),CV_8UC2); // 清除橫幅影像緩衝區
         ShowFrame(imgF2, ui->pbxF2);                    // 將圖框影像秀在標籤上
         ui->btnGoB2->setEnabled(true);                  // 致能[更新]按鍵
     }
}

// 載入圖框影像三
void MainWindow::on_btnLoadF3_clicked()
{
    fileName = QFileDialog::getOpenFileName(this,tr("Open File")); // 開啟檔案對話盒並取得檔名
    imgSrc3 = cv::imread(fileName.toStdString(),IMREAD_COLOR);     // 讀入影像

     if(!imgSrc3.empty()) {                       // 若影像不空
         int fw = ui->txbFrameW->text().toInt();  // 取得圖框寬度
         int fh = ui->txbFrameH->text().toInt();  // 取得圖框高度
         int bw = ui->txbBannerW->text().toInt(); // 取得橫幅寬度
         int bh = ui->txbBannerH->text().toInt(); // 取得橫幅高度

         cv::resize(imgSrc3,imgF3,cvSize(fw,fh));        // 將原始影像縮到指定圖框尺寸
         imgLCDF3 = Mat::zeros(cv::Size(fw,fh),CV_8UC2); // 清除圖框影像緩衝區
         imgLCDB3 = Mat::zeros(cv::Size(bw,bh),CV_8UC2); // 清除橫幅影像緩衝區
         ShowFrame(imgF3, ui->pbxF3);                    // 將圖框影像秀在標籤上
         ui->btnGoB3->setEnabled(true);                  // 致能[更新]按鍵
     }
}

// 變更橫幅一背景色彩
void MainWindow::on_btnBGColorB1_clicked()
{
    QColorDialog colorDlg;

    colorDlg.exec();                            // 開啟色彩拾取盒
    BGColorB1 = colorDlg.selectedColor();       // 設定橫幅背景色彩
    SetLabelColor(ui->labBGColorB1, BGColorB1); // 設定標籤色彩
    ui->btnGoB1->click();                       // 代理按下[更新]鍵
}

// 變更橫幅一前景(文字)色彩
void MainWindow::on_btnFGColorB1_clicked()
{
    QColorDialog colorDlg;

    colorDlg.exec();                            // 開啟色彩拾取盒
    FGColorB1 = colorDlg.selectedColor();       // 設定橫幅前景(文字)色彩
    SetLabelColor(ui->labFGColorB1, FGColorB1); // 設定標籤色彩
    ui->btnGoB1->click();                       // 代理按下[更新]鍵
}

// 變更橫幅二背景色彩
void MainWindow::on_btnBGColorB2_clicked()
{
    QColorDialog colorDlg;

    colorDlg.exec();                            // 開啟色彩拾取盒
    BGColorB2 = colorDlg.selectedColor();       // 設定橫幅背景色彩
    SetLabelColor(ui->labBGColorB2, BGColorB2); // 設定標籤色彩
    ui->btnGoB2->click();                       // 代理按下[更新]鍵
}

// 變更橫幅二前景(文字)色彩
void MainWindow::on_btnFGColorB2_clicked()
{
    QColorDialog colorDlg;

    colorDlg.exec();                            // 開啟色彩拾取盒
    FGColorB2 = colorDlg.selectedColor();       // 設定橫幅前景(文字)色彩
    SetLabelColor(ui->labFGColorB2, FGColorB2); // 設定標籤色彩
    ui->btnGoB2->click();                       // 代理按下[更新]鍵
}

// 變更橫幅三背景色彩
void MainWindow::on_btnBGColorB3_clicked()
{
    QColorDialog colorDlg;

    colorDlg.exec();                            // 開啟色彩拾取盒
    BGColorB3 = colorDlg.selectedColor();       // 設定橫幅背景色彩
    SetLabelColor(ui->labBGColorB3, BGColorB3); // 設定標籤色彩
    ui->btnGoB3->click();                       // 代理按下[更新]鍵
}

// 變更橫幅三前景(文字)色彩
void MainWindow::on_btnFGColorB3_clicked()
{
    QColorDialog colorDlg;

    colorDlg.exec();                            // 開啟色彩拾取盒
    FGColorB3 = colorDlg.selectedColor();       // 設定橫幅前景(文字)色彩
    SetLabelColor(ui->labFGColorB3, FGColorB3); // 設定標籤色彩
    ui->btnGoB3->click();                       // 代理按下[更新]鍵
}

// 更新橫幅一(含色彩及字體)
void MainWindow::on_btnGoB1_clicked()
{
    int bw = ui->txbBannerW->text().toInt();              // 取得橫幅寬度
    int bh = ui->txbBannerH->text().toInt();              // 取得橫幅高度
    QImage imgB1Q = QImage(bw,bh, QImage::Format_RGB888); // 宣告影像暫存區(RGB888)
    QPainter painter(&imgB1Q);                            // 宣告繪圖區

    painter.setBrush(BGColorB1);     // 設定繪圖區筆刷(背景)色
    painter.setPen(BGColorB1);       // 設定繪圖區彩筆(前景)色，去除邊框
    painter.drawRect(imgB1Q.rect()); // 設定繪圖區尺寸
    painter.setPen(FGColorB1);       // 設定繪圖區彩筆(前景)色
    painter.setFont(fontB1);         // 設定繪圖區字體
    painter.drawText(imgB1Q.rect(), Qt::AlignCenter,ui->txbB1->text()); // 將文字繪至繪圖區

    ui->pbxB1->setPixmap(QPixmap::fromImage(imgB1Q)); // 將繪好的文字區設定至標籤
    ui->pbxB1->setAlignment(Qt::AlignCenter);         // 設定置中對齊

    Mat matROI = imgF1(Rect(0,140,220,36));           // 設定圖框影像預備放橫幅位置
    //設定cv::Mat空白繪圖區，從QImage轉到cv::Mat
    Mat matPainter = cv::Mat(imgB1Q.height(), imgB1Q.width(), CV_8UC3, imgB1Q.bits(), imgB1Q.bytesPerLine());

    cvtColor(matPainter, matPainter, CV_RGB2BGR); // 將色彩從BGR888轉到RGB888
    matPainter.copyTo(imgB1);                     // 將橫幅繪圖區複製到橫幅影像區
    painter.end();                                // 結束繪圖區

    if(ui->txbB1->text() != 0 && !ui->rdbNoneB1->isChecked()){ // 若文字內容不為空且橫幅不是選擇不顯示模式
        imgB1.copyTo(matROI);                                  // 則將橫幅影像貼到圖框影像中指定位置
    }
    else{
        cv::resize(imgSrc1,imgF1,imgF1.size());                // 否則重新以原始影像縮至圖框影像中
    }    

    ShowFrame(imgF1, ui->pbxF1); // 顯示圖框(含橫幅)影像
}

// 變更橫幅一字體
void MainWindow::on_btnFontB1_clicked()
{
    QFontDialog fontDlg;

    fontDlg.exec();                  // 開啟字體選擇對話盒
    fontB1 = fontDlg.selectedFont(); // 取得字體參數
    ui->btnGoB1->click();            // 代理按下[更新]鍵
}

// 更新橫幅二(含色彩及字體)
void MainWindow::on_btnGoB2_clicked()
{
    int bw = ui->txbBannerW->text().toInt();               // 取得橫幅寬度
    int bh = ui->txbBannerH->text().toInt();               // 取得橫幅高度
    QImage imgB2Q = QImage(bw, bh, QImage::Format_RGB888); // 宣告影像暫存區(RGB888)
    QPainter painter(&imgB2Q);                             // 宣告繪圖區

    painter.setBrush(BGColorB2);     // 設定繪圖區筆刷(背景)色
    painter.setPen(BGColorB2);       // 設定繪圖區彩筆(前景)色，去除邊框
    painter.drawRect(imgB2Q.rect()); // 設定繪圖區尺寸
    painter.setPen(FGColorB2);       // 設定繪圖區彩筆(前景)色
    painter.setFont(fontB2);         // 設定繪圖區字體
    painter.drawText(imgB2Q.rect(), Qt::AlignCenter,ui->txbB2->text()); // 將文字繪至繪圖區

    ui->pbxB2->setPixmap(QPixmap::fromImage(imgB2Q)); // 將繪好的文字區設定至標籤
    ui->pbxB2->setAlignment(Qt::AlignCenter);         // 設定置中對齊

    Mat matROI = imgF2(Rect(0,140,220,36));           // 設定圖框影像預備放橫幅位置
    //設定cv::Mat空白繪圖區，從QImage轉到cv::Mat
    Mat matPainter = cv::Mat(imgB2Q.height(), imgB2Q.width(), CV_8UC3, imgB2Q.bits(), imgB2Q.bytesPerLine());

    cvtColor(matPainter, matPainter, CV_RGB2BGR); // 將色彩從BGR888轉到RGB888
    matPainter.copyTo(imgB2);                     // 將橫幅繪圖區複製到橫幅影像區
    painter.end();                                // 結束繪圖區

    if(ui->txbB2->text() != 0 && !ui->rdbNoneB2->isChecked()){ // 若文字內容不為空且橫幅不是選擇不顯示模式
        imgB2.copyTo(matROI);                                  // 則將橫幅影像貼到圖框影像中指定位置
    }
    else{
        cv::resize(imgSrc2,imgF2,imgF2.size());                // 否則重新以原始影像縮至圖框影像中
    }

    ShowFrame(imgF2, ui->pbxF2); // 顯示圖框(含橫幅)影像
}

// 變更橫幅二字體
void MainWindow::on_btnFontB2_clicked()
{
    QFontDialog fontDlg;

    fontDlg.exec();                  // 開啟字體選擇對話盒
    fontB2 = fontDlg.selectedFont(); // 取得字體參數
    ui->btnGoB2->click();            // 代理按下[更新]鍵
}

// 更新橫幅三(含色彩及字體)
void MainWindow::on_btnGoB3_clicked()
{
    int bw = ui->txbBannerW->text().toInt();               // 取得橫幅寬度
    int bh = ui->txbBannerH->text().toInt();               // 取得橫幅高度
    QImage imgB3Q = QImage(bw, bh, QImage::Format_RGB888); // 宣告影像暫存區(RGB888)
    QPainter painter(&imgB3Q);                             // 宣告繪圖區

    painter.setBrush(BGColorB3);     // 設定繪圖區筆刷(背景)色
    painter.setPen(BGColorB3);       // 設定繪圖區彩筆(前景)色，去除邊框
    painter.drawRect(imgB3Q.rect()); // 設定繪圖區尺寸
    painter.setPen(FGColorB3);       // 設定繪圖區彩筆(前景)色
    painter.setFont(fontB3);         // 設定繪圖區字體
    painter.drawText(imgB3Q.rect(), Qt::AlignCenter,ui->txbB3->text()); // 將文字繪至繪圖區

    ui->pbxB3->setPixmap(QPixmap::fromImage(imgB3Q)); // 將繪好的文字區設定至標籤
    ui->pbxB3->setAlignment(Qt::AlignCenter);         // 設定置中對齊

    Mat matROI = imgF3(Rect(0,140,220,36));           // 設定圖框影像預備放橫幅位置
    //設定cv::Mat空白繪圖區，從QImage轉到cv::Mat
    Mat matPainter= cv::Mat(imgB3Q.height(), imgB3Q.width(), CV_8UC3, imgB3Q.bits(), imgB3Q.bytesPerLine());

    cvtColor(matPainter, matPainter, CV_RGB2BGR); // 將色彩從BGR888轉到RGB888
    matPainter.copyTo(imgB3);                     // 將橫幅繪圖區複製到橫幅影像區
    painter.end();                                // 結束繪圖區

    if(ui->txbB3->text() != 0 && !ui->rdbNoneB3->isChecked()){ // 若文字內容不為空且橫幅不是選擇不顯示模式
        imgB3.copyTo(matROI);                                  // 則將橫幅影像貼到圖框影像中指定位置
    }
    else{
        cv::resize(imgSrc3,imgF3,imgF3.size());                // 否則重新以原始影像縮至圖框影像中
    }

    ShowFrame(imgF3, ui->pbxF3); // 顯示圖框(含橫幅)影像
}

// 變更橫幅三字體
void MainWindow::on_btnFontB3_clicked()
{
    QFontDialog fontDlg;

    fontDlg.exec();                  // 開啟字體選擇對話盒
    fontB3 = fontDlg.selectedFont(); // 取得字體參數
    ui->btnGoB3->click();            // 代理按下[更新]鍵
}

// 圖框一顯示停留時間變更
void MainWindow::on_spbF1_valueChanged(double arg1)
{
    disp_timerF1 = disp_timerB1 = (int)(arg1*10); // 取得圖框顯示時間(單位乘10)
}

// 圖框二顯示停留時間變更
void MainWindow::on_spbF2_valueChanged(double arg1)
{
    disp_timerF2 = disp_timerB2 = (int)(arg1*10); // 取得圖框顯示時間(單位乘10)
}

// 圖框三顯示停留時間變更
void MainWindow::on_spbF3_valueChanged(double arg1)
{
    disp_timerF3 = disp_timerB3 = (int)(arg1*10); // 取得圖框顯示時間(單位乘10)
}

// 橫幅一顯示模式切換到固定模式
void MainWindow::on_rdbFixB1_clicked()
{
    disp_modeB1 = 0; // 顯示模式編號設定為0
}

// 橫幅一顯示模式切換到交替閃爍模式
void MainWindow::on_rdbBlinkB1_clicked()
{
    disp_modeB1 = 1; // 顯示模式編號設定為1
}

// 橫幅一顯示模式切換到不顯示模式
void MainWindow::on_rdbNoneB1_clicked()
{
    disp_modeB1 = 2; // 顯示模式編號設定為2
}

// 橫幅二顯示模式切換到固定模式
void MainWindow::on_rdbFixB2_clicked()
{
    disp_modeB2 = 0; // 顯示模式編號設定為0
}

// 橫幅二顯示模式切換到交替閃爍模式
void MainWindow::on_rdbBlinkB2_clicked()
{
    disp_modeB2 = 1; // 顯示模式編號設定為1
}

// 橫幅二顯示模式切換到不顯示模式
void MainWindow::on_rdbNoneB2_clicked()
{
    disp_modeB2 = 2; // 顯示模式編號設定為2
}

// 橫幅三顯示模式切換到固定模式
void MainWindow::on_rdbFixB3_clicked()
{
    disp_modeB3 = 0; // 顯示模式編號設定為0
}

// 橫幅三顯示模式切換到交替閃爍模式
void MainWindow::on_rdbBlinkB3_clicked()
{
    disp_modeB3 = 1; // 顯示模式編號設定為1
}

// 橫幅三顯示模式切換到不顯示模式
void MainWindow::on_rdbNoneB3_clicked()
{
    disp_modeB3 = 2; // 顯示模式編號設定為2
}

