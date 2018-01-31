// 迷你開源電子看板 OpenQSignage
// 作者：歐尼克斯實境互動工作室 OmniXRI Jack Hsu 
//       Jan. 2018

#include <SPI.h> // 引入驅動SPI函式庫，應用於SPI PSRAM

// LCD 顯示模式
#define LCD_DISP_FIXED  0 // 固定顯示
#define LCD_DISP_BLINK  1 // 交替閃爍
#define LCD_DISP_NONE   2 // 不顯示

#define FRAME_TOTAL_AMOUNT  6  // 圖像(含圖框及橫幅)
#define FRAME_HEADER_SIZE   10 // 排程資料位元數 

// LCD常用色定義 (16bit, RGB565, 65536色)
#define VGA_BLACK    0x0000  // 黑色 
#define VGA_WHITE    0xFFFF  // 白色
#define VGA_RED      0xF800  // 紅色
#define VGA_GREEN    0x0400  // 綠色
#define VGA_BLUE     0x001F  // 藍色
#define VGA_SILVER   0xC618  // 銀(亮灰)色
#define VGA_GRAY     0x8410  // 灰色
#define VGA_MAROON   0x8000  // 栗(暗紅)色
#define VGA_YELLOW   0xFFE0  // 黃色
#define VGA_OLIVE    0x8400  // 橄欖(暗綠)色
#define VGA_LIME     0x07E0  // 青檸(亮綠)色
#define VGA_AQUA     0x07FF  // 水藍(亮藍)色
#define VGA_TEAL     0x0410  // 水鳥(暗監綠)色
#define VGA_NAVY     0x0010  // 海軍(暗藍)色
#define VGA_FUCHSIA  0xF81F  // 紫紅色
#define VGA_PURPLE   0x8010  // 紫色

// 主要I/O設定
int PIN_SW1 = 2;       // 按鍵(SW)1
int PIN_LED1 = 3;      // 指示燈(LED)1

// 2.2吋 LCD 模塊I/O設定(176*220像素，ILI9225驅動晶片，8 Bit匯流排)
int PIN_LCD_CS = 10;   // LCD晶片選擇線(和SPI PSRAM晶片選擇線PIN_PSRAM_CE共用)
int PIN_LCD_RS = 9;    // LCD命令暫存器選擇線
int PIN_LCD_WR = 8;    // LCD寫入線
int PIN_LCD_DB0 = 14;  // LCD資料匯流排位元0, Arduino PC0/A0
int PIN_LCD_DB1 = 15;  // LCD資料匯流排位元1, Arduino PC1/A1
int PIN_LCD_DB2 = 16;  // LCD資料匯流排位元2, Arduino PC2/A2
int PIN_LCD_DB3 = 17;  // LCD資料匯流排位元3, Arduino PC3/A3
int PIN_LCD_DB4 = 4;   // LCD資料匯流排位元4, Arduino PD4
int PIN_LCD_DB5 = 5;   // LCD資料匯流排位元5, Arduino PD5
int PIN_LCD_DB6 = 6;   // LCD資料匯流排位元6, Arduino PD6
int PIN_LCD_DB7 = 7;   // LCD資料匯流排位元7, Arduino PD7

// 來揚科技(Lyontek) SPI PSRAM LY68L6400 I/O設定
int PIN_PSRAM_CE = 10;  // 晶片選擇線， Arduino PB2(和LCD晶片選擇線PIN_LCD_CS共用)
int PIN_PSRAM_SI = 11;  // 串列資料輸入線， Arduino PB3
int PIN_PSRAM_SO = 12;  // 串列資料輸出線， Arduino PB4
int PIN_PSRAM_SCK = 13; // 時脈線， Arduino PB5

int SW1State = 0;       // 按鍵1狀態
int color_table[16] = {VGA_BLACK, VGA_WHITE, VGA_RED, VGA_GREEN,
                       VGA_BLUE,  VGA_SILVER, VGA_GRAY, VGA_MAROON,
                       VGA_YELLOW, VGA_OLIVE, VGA_LIME, VGA_AQUA,
                       VGA_TEAL, VGA_NAVY, VGA_FUCHSIA, VGA_PURPLE}; // 色彩索引表
int auto_loop = 0;       // 0為手動操作，非0為自動依排程播放
int frame_index = 0;     // 目前播放圖框(含橫幅)編號 
int frame_updated = 0;   // 0為LCD尚未更新顯示內容，1為已更新
int download_finish = 0; // 下載顯示及排程資料完成
unsigned long startTime; // 計時器起始時間
unsigned long duration;  // 計時器經過時間
 
// 顯示排程資料格式
// [0] 圖框寬度高位元組(d15 ~ d8)
// [1] 圖框寬度低位元組(d0  ~ d7)
// [2] 圖框高度高位元組(d15 ~ d8)
// [3] 圖框高度低位元組(d0  ~ d7)
// [4] LCD顯示起始位置X高位元組(d15 ~ d8)
// [5] LCD顯示起始位置X低位元組(d0  ~ d7)
// [6] LCD顯示起始位置Y高位元組(d15 ~ d8)
// [7] LCD顯示起始位置Y低位元組(d0  ~ d7)
// [8] LCD顯示模式，0:固定不變，1:交替閃爍，2:不顯示
// [9] LCD顯示計時器， 0 ~ 255 表示 0.0 ~ 25.5 秒
char frame_header[10];  // 顯   

// Arduino初始化程式
void setup() {  
  // 初始化主要I/O
  pinMode(PIN_LED1, OUTPUT);  // 設定LED腳位為輸出
  pinMode(PIN_SW1, INPUT);    // 設定SW腳位為輸入
  
  // 初始化UART通訊埠(COM Port)
  Serial.begin(57600);        // 設定通訊鮑率為57,600bps
  
  // 初始化SPI串列埠為控制來揚科技(Lyontek) SPI PSRAM LY68L6400
  //  pinMode(PIN_PSRAM_CE, OUTPUT); // 和LCD晶片選擇線共用，所以不用重覆設定
  SPI.begin();                          // 啟動SPI
  SPI.setBitOrder(MSBFIRST);            // 設定SPI為高位元先送模式
  SPI.setDataMode(SPI_MODE0);           // 設定SPI為模式0(CPOL,CPHA皆為零，即資料SI/SO備妥後CLK再以正緣觸發)
  SPI.setClockDivider(SPI_CLOCK_DIV2);  // 為外頻再除以2，3.3V Arduino外頻為8MHz，所以SPI時脈為4MHz
  
  // 初始化2.2吋LCD模塊相關I/O
  pinMode(PIN_LCD_CS, OUTPUT);   // 設定晶片選擇線為輸出，和SPI PSRAM晶片選擇線共用
  pinMode(PIN_LCD_RS, OUTPUT);   // 設定暫存器選擇線為輸出
  pinMode(PIN_LCD_WR, OUTPUT);   // 設定寫入線為輸出
  pinMode(PIN_LCD_DB0, OUTPUT);  // 設定資料匯流排DB0為輸出
  pinMode(PIN_LCD_DB1, OUTPUT);  // 設定資料匯流排DB1為輸出  
  pinMode(PIN_LCD_DB2, OUTPUT);  // 設定資料匯流排DB2為輸出  
  pinMode(PIN_LCD_DB3, OUTPUT);  // 設定資料匯流排DB3為輸出  
  pinMode(PIN_LCD_DB4, OUTPUT);  // 設定資料匯流排DB4為輸出  
  pinMode(PIN_LCD_DB5, OUTPUT);  // 設定資料匯流排DB5為輸出  
  pinMode(PIN_LCD_DB6, OUTPUT);  // 設定資料匯流排DB6為輸出  
  pinMode(PIN_LCD_DB7, OUTPUT);  // 設定資料匯流排DB7為輸出  
  ILI9225_init();                // LCD工作暫存器初始化
  ILI9225_LCD_Test();            // LCD顯示測試
}

// Arduino無限循環程式段
void loop() {
  if (digitalRead(PIN_SW1) == HIGH) {    // 當SW1被按下時 
    startTime = millis();                // 開始啟動計時器(ms)
    
    while(digitalRead(PIN_SW1) == HIGH); // 等待按鍵放開

    duration = millis() - startTime;     // 計算按鍵按下時間長度
    
    if(duration > 3000){                 // 若按超過3秒
      if(auto_loop == 1)                 // 若目前是自動播放模式
        auto_loop = 0;                   // 則切換到手動切換模式
      else
        auto_loop = 1;                   // 若否則切換到自動模式
    }  
    else{                                // 若按鍵按下不到3秒
      auto_loop = 0;                     // 則切換到手動切換模式
      frame_updated = 0;                 // 畫面設為未更新
    }    
  }    

  // 若顯示資料已下載完成且為手動切換模式且畫面尚未更新
  if (download_finish != 0 && auto_loop == 0 && frame_updated == 0) {
     ShowFrame(frame_index, 0);          // 正常顯示圖框內容到LCD(偶數為圖框)
     frame_index++;                      // 目前圖框編號加一
     ShowFrame(frame_index, 0);          // 正常顯示橫幅內容到LCD(奇時為橫幅)
     frame_updated = 1;                  // 設定LCD顯示已更新
     frame_index++;                      // 圖框(橫幅)編號加一
    
     if(frame_index >= 6)                // 圖框(橫幅)編號大於等於6
       frame_index = 0;                  // 圖框(橫幅)編號歸零
  }
  else if (download_finish != 0 && auto_loop == 1) { // 若顯示資料已下載完成且為自動播放模式
     ShowFrame(frame_index, 0);          // 正常顯示圖框內容到LCD(偶數為圖框)
     frame_index++;                      // 圖框編號加一
     GetFrameHeader(frame_index);        // 取得圖框(橫幅)顯示排程資料
     frame_updated = 0;                  // 設定LCD顯示未更新
     startTime = millis();               // 啟動計時器
     
     while((millis() - startTime) < (frame_header[9]*100)){ // 若計時器未達排程設定時間
       switch(frame_header[8])               // 依顯示模式執行顯示內容
       {
        case LCD_DISP_FIXED:                 // 固定顯示模式 
               if(frame_updated == 0)        // 若LCD尚未更新
                 ShowFrame(frame_index, 0);  // 則正常顯示目前圖框編號內容
                  
               frame_updated = 1;            // 設定為LCD已更新內容
               break;
        case LCD_DISP_BLINK:                 // 交替顯示模式
               ShowFrame(frame_index, 0);    // 正常顯示目前圖框(橫幅)編號內容
               delay(500);                   // 延時500ms
               ShowFrame(frame_index, 1);    // 反相顯示目前圖框(橫幅)編號內容
               delay(500);                   // 延時500ms
               break;
        case LCD_DISP_NONE:                  // 不顯式模式(直接跳過)
        default:
             break;
       }
     }
     
     frame_index++;                      // 圖框(橫幅)編號加一
    
     if(frame_index >= 6)                // 圖框(橫幅)編號大於等於6
       frame_index = 0;                  // 圖框(橫幅)編號歸零
  }
  
  int rx_data;                      // 儲存接收到的位元組
  
  if (Serial.available() > 0) {     // 假若UART未接收到任何資料則略過
    download_finish = 0;            // 清除已下載完成旗標
    
    rx_data = Serial.read();        // 取得動作命令
    
    if(rx_data == 0xA5){            // 若命令為0xA5
      digitalWrite(PIN_LED1, HIGH); // 則點亮LED1
      delay(100);                   // 延時100ms
    }  
    else if(rx_data == 0xB4){       // 若命令為0xB4
      ILI9225_Clr_Screen();         // 則清除LCD顯示內容(屏幕全黑)
      digitalWrite(PIN_LED1, LOW);  // 並熄滅LED1
    }  
    else if(rx_data == 0xC3){       // 若命令為0xC3則開始接收PC透過UART送過來的資料        
      for(int f_idx=0; f_idx<FRAME_TOTAL_AMOUNT; f_idx++){ // 預計接收六組資料(圖框加橫幅各三組)        
        digitalWrite(PIN_LCD_CS, LOW);  // SPI PSRAM 晶片選擇線設為低電位
        // 設定 SPI PSRAM寫入起始位置
        SPI.transfer(0x02);             // 寫入PSRAM寫入命令0x02
        SPI.transfer((f_idx+1)*2);      // 寫入PSRAM位址 bit 23 ~ 16，每個圖框(橫幅)儲存間隔20000h
        SPI.transfer(0);                // 寫入PSRAM位址 bit 15 ~ 8
        SPI.transfer(0);                // 寫入PSRAM位址 bit 7 ~ 0           
     
        // 取得圖框排程資料
        for(int i=0; i<FRAME_HEADER_SIZE; i++){  // 預計接收10位元組資料
          while(!Serial.available());            // 等待UART已備妥接收到資料信號
          
          rx_data = Serial.read();               // 接收一個位元組資料
          frame_header[i] = (char)rx_data;       // 將資料讀入frame_header中指定位置
          SPI.transfer(frame_header[i]);         // 將讀入資料寫入PSRAM(位址自動加一)
        }
        
        Serial.write(0x5A);                      // 透過UART回傳0x5A回應已接收到資料
        
        // 依接收到的資料，設定顯示圖框(橫幅)的寬(fw)、高(fh)及起始位置(sx,sy)        
        int fw = (frame_header[0] * 256) + (unsigned char)frame_header[1];
        int fh = (frame_header[2] * 256) + (unsigned char)frame_header[3]; 
        //int sx = (frame_header[4] * 256) + frame_header[5]);
        //int sy = (frame_header[6] * 256) + frame_header[7];     

        // PC指定的LCD顯示位置(SXdisp,SYdisp)與LCD GRAM和起始位置(SXgram,SYgram)的換算公式
        // SXgram = 176 - SYdisp - 1; LCD width = 176
        // SYgram = SXdisp;        
        // SXlcd,  SYlcd  (Left,Top)-(Right,Bottom), Frame (0  ,0)-(219,175)
        // SXgram, SYgram (Left,Top)-(Right,Bottom), GRAM  (175,0)-(0,  219)
        // x1 <= sx <= x2, y1 <= sy <= y2, if x1>x2 or y1>y2 must be swap
        // ILI9225_SetSY(x1,y1,x2,y2,sx,sy);  // 設定LCD GRAM讀寫區域大小

        if(f_idx % 2 == 0)                       // 若圖框編號為偶數
          ILI9225_SetXY(0, 0, 175, 219, 175, 0); // 則設定顯示區域為(0, 0, 175, 219, 175, 0)
        else                                     // 若為奇數則為橫幅  
          ILI9225_SetXY(0, 0, 35, 219, 35, 0);   // 則設定顯示區域為(0, 0, 35, 219, 35, 0)
          
        digitalWrite(PIN_LCD_CS, LOW);  // SPI PSRAM 晶片選擇線設為低電位
        // 設定 SPI PSRAM寫入起始位置
        SPI.transfer(0x02);             // 寫入PSRAM寫入命令0x02
        SPI.transfer((f_idx+1)*2);      // 寫入PSRAM位址 bit 23 ~ 16，每個圖框(橫幅)儲存間隔20000h
        SPI.transfer(0);                // 寫入PSRAM位址 bit 15 ~ 8
        SPI.transfer(10);               // 寫入PSRAM位址 bit 7 ~ 0，圖框資料從位置10開始
    
        for(int i=0; i<fh; i++){        // 設定迴圈數為圖框高度
          for(int j=0; j<fw*2; j++){    // 設定迴圈數為圖框寬度乘2，每個點為2個位元組(RGB565)
            while(!Serial.available()); // 等待UART已備妥接收到資料信號
          
            rx_data = Serial.read();           // 接收一個位元組資料
            ILI9225_WR_Data8((char) rx_data);  // 寫一個位元組資料到LCD繪圖記憶區(GRAM)先高再低位元組
            SPI.transfer((char) rx_data);      // 寫一個位元組資料到SPI PSRAM
          }

          Serial.write(0x5B);                  // 透過UART回傳0x5B回應已接收到資料
        }
      
        digitalWrite(PIN_LCD_CS, HIGH);        // 將晶片選擇線設為高電位結束接收圖框資料 
        digitalWrite(PIN_LED1, HIGH);          // 點亮LED1表目前圖框已下載完成
      }
      
      frame_index = 0;                         // 圖框編號歸零
      download_finish = 1;                     // 設定已完成接收旗標為1
      auto_loop = 1;                           // 預設下載完成後為自動依排程播放
    } 
  } 
}

// 讀取圖框(橫幅)排程資訊
// [in] idx 圖框(橫幅)編號
void GetFrameHeader(int idx)
{             
      digitalWrite(PIN_LCD_CS, LOW);  // SPI PSRAM 晶片選擇線設為低電位 
      // 設定 SPI PSRAM讀取起始位置
      SPI.transfer(0x03);             // 寫入PSRAM低速讀取命令0x03
      SPI.transfer((idx+1)*2);        // 寫入PSRAM位址 bit 23 ~ 16，每個圖框(橫幅)儲存間隔20000h
      SPI.transfer(0);                // 寫入PSRAM位址 bit 15 ~ 8
      SPI.transfer(0);                // 寫入PSRAM位址 bit 7 ~ 0
          
      for(int i=0; i<FRAME_HEADER_SIZE; i++){
        frame_header[i] = SPI.transfer(0);  // 從SPI PSRAM讀取資料寫入frame_header對應位置
      } 
      
      digitalWrite(PIN_LCD_CS, HIGH);  // SPI PSRAM 晶片選擇線設為高電位 
}

// 顯示圖框(橫幅)於LCD屏幕上
// [in] idx 圖框(橫幅)編號
// [in] inv 0為正常顯示，1為色彩反相顯示
void ShowFrame(int idx, int inv)
{
      digitalWrite(PIN_LED1, LOW);     // 晶片選擇線設為低電位 
      GetFrameHeader(idx);             // 讀取圖框(橫幅)排程資訊 
      
      int fw = (frame_header[0] * 256) + (unsigned char)frame_header[1];  // 計算圖框寬度
      int fh = (frame_header[2] * 256) + (unsigned char)frame_header[3];  // 計算圖框高度
      
      if(frame_index%2 == 0)                    // 若圖框編號為偶數     
        ILI9225_SetXY(0, 0, 175, 219, 175, 0);  // 則設定顯示區域為(0, 0, 175, 219, 175, 0)
      else                                      // 若為奇數則為橫幅
        ILI9225_SetXY(0, 0, 35, 219, 35, 0);    // 則設定顯示區域為(0, 0, 35, 219, 35, 0)
       
      digitalWrite(PIN_LCD_CS, LOW);    // ILI9225_SetXY()會使晶片選擇線設為高電位，所以要重新設低電位
      // 設定 SPI PSRAM讀取起始位置
      SPI.transfer(0x03);               // 寫入PSRAM低速讀取命令0x03
      SPI.transfer((idx+1)*2);          // 寫入PSRAM位址 bit 23 ~ 16，每個圖框(橫幅)儲存間隔20000h
      SPI.transfer(0);                  // 寫入PSRAM位址 bit 15 ~ 8
      SPI.transfer(10);                 // 寫入PSRAM位址 bit 7 ~ 0，圖框資料從位置10開始
         
      for(int i=0;i<fh;i++){            // 設定迴圈數為圖框高度 
        for(int j=0;j<fw*2;j++){        // 設定迴圈數為圖框寬度乘2，每個點為2個位元組(RGB565)
          char dataT = SPI.transfer(0); // 從SPI PSRAM讀取一個位元組資料         
          if(inv==1)                    // 若設定為色彩反相
            dataT = dataT ^ 0xFF;       // 則令數值反相(作XOR運算)
            
          ILI9225_WR_Data8(dataT);      // 將資料寫入LCD GRAM中                 
        }
      }
      
      digitalWrite(PIN_LCD_CS, HIGH);   // 晶片選擇線設為高電位 
      digitalWrite(PIN_LED1, HIGH);     // 點亮LED1   
}

//*************************************************************************************
// LCD 模塊驅動程式
// 176*220像素，65,536(2^16)色(RGB565)，8 bit資料匯流排, ILI9225驅動晶片
// 完整資訊可參考http://www.displayfuture.com/Display/datasheet/controller/ILI9225.pdf
//*************************************************************************************

// LCD初始化
// 【注意】不熟悉參數設定的人請勿任意調整下列參數，以免造成LCD損壞或無法正常顯示
void ILI9225_init()
{
 digitalWrite(PIN_LCD_CS, HIGH); 
 digitalWrite(PIN_LCD_RS, HIGH); 
 digitalWrite(PIN_LCD_WR, HIGH);  
 delay(50);
 ILI9225_WR_Cmd_Data(0x10, 0x0000); // Power Control 1
 ILI9225_WR_Cmd_Data(0x11, 0x0000); // Power Control 2
 ILI9225_WR_Cmd_Data(0x12, 0x0000); // Power Control 3
 ILI9225_WR_Cmd_Data(0x13, 0x0000); // Power Control 4
 ILI9225_WR_Cmd_Data(0x14, 0x0000); // Power Control 5
 delay(40);
 ILI9225_WR_Cmd_Data(0x11, 0x0018); // Power Control 2 
 ILI9225_WR_Cmd_Data(0x12, 0x6121); // Power Control 3 
 ILI9225_WR_Cmd_Data(0x13, 0x006F); // Power Control 4 
 ILI9225_WR_Cmd_Data(0x14, 0x495F); // Power Control 5 
 ILI9225_WR_Cmd_Data(0x10, 0x0800); // Power Control 1
 delay(10);
 ILI9225_WR_Cmd_Data(0x11, 0x103B); // Power Control 2 
 delay(50);
 ILI9225_WR_Cmd_Data(0x01, 0x011C); // Driver Output Control
 ILI9225_WR_Cmd_Data(0x02, 0x0100); // LCD AC Driving Control
 ILI9225_WR_Cmd_Data(0x03, 0x1028); // Entry Mode 螢幕橫式顯示，由左而右掃描
 ILI9225_WR_Cmd_Data(0x07, 0x0000); // Display Control 1
 ILI9225_WR_Cmd_Data(0x08, 0x0808); // Blank Period Control 1
 ILI9225_WR_Cmd_Data(0x0b, 0x1100); // Frame Cycle Control
 ILI9225_WR_Cmd_Data(0x0c, 0x0000); // Interface Control
 ILI9225_WR_Cmd_Data(0x0f, 0x0D01); // Oscillation Control
 ILI9225_WR_Cmd_Data(0x15, 0x0020); // ??
 ILI9225_WR_Cmd_Data(0x20, 0x0000); // RAM Address Set 1
 ILI9225_WR_Cmd_Data(0x21, 0x0000); // RAM Address Set 2
 ILI9225_WR_Cmd_Data(0x30, 0x0000); // Gate Scan Control
 ILI9225_WR_Cmd_Data(0x31, 0x00DB); // Vertical Scroll Control 1
 ILI9225_WR_Cmd_Data(0x32, 0x0000); // Vertical Scroll Control 2
 ILI9225_WR_Cmd_Data(0x33, 0x0000); // Vertical Scroll Control 3
 ILI9225_WR_Cmd_Data(0x34, 0x00DB); // Partial Driving Position -1
 ILI9225_WR_Cmd_Data(0x35, 0x0000); // Partial Driving Position -2
 ILI9225_WR_Cmd_Data(0x36, 0x00AF); // Horizontal Window Address -1
 ILI9225_WR_Cmd_Data(0x37, 0x0000); // Horizontal Window Address -2
 ILI9225_WR_Cmd_Data(0x38, 0x00DB); // Vertical Window Address -1
 ILI9225_WR_Cmd_Data(0x39, 0x0000); // Vertical Window Address -2
 ILI9225_WR_Cmd_Data(0x50, 0x0000); // Gamma Control 1
 ILI9225_WR_Cmd_Data(0x51, 0x0808); // Gamma Control 2
 ILI9225_WR_Cmd_Data(0x52, 0x080A); // Gamma Control 3
 ILI9225_WR_Cmd_Data(0x53, 0x000A); // Gamma Control 4		
 ILI9225_WR_Cmd_Data(0x54, 0x0A08); // Gamma Control 5
 ILI9225_WR_Cmd_Data(0x55, 0x0808); // Gamma Control 6
 ILI9225_WR_Cmd_Data(0x56, 0x0000); // Gamma Control 7
 ILI9225_WR_Cmd_Data(0x57, 0x0A00); // Gamma Control 8
 ILI9225_WR_Cmd_Data(0x58, 0x0710); // Gamma Control 9
 ILI9225_WR_Cmd_Data(0x59, 0x0710); // Gamma Control 10
 ILI9225_WR_Cmd_Data(0x07, 0x0012); // Display Control 1
 delay(50);
 ILI9225_WR_Cmd_Data(0x07, 0x1017); // Display Control 1
 digitalWrite(PIN_LCD_CS, LOW); 
 ILI9225_WR_Cmd(0x22);              // Write Data to GRAM
 digitalWrite(PIN_LCD_CS, HIGH);  
}

// 設定LCD讀寫視窗範圍
// [in] x1,y1 為左上座標
// [in] x2,y2 為右下座標
// [in] sx,sy 為起始座標
// 必須符合 x1 < sx < x2, y1 < sy < y2, 
//          x1 >= 0 && x2 < 176, y1 >= 0 && y2 < 220
// 座標是以直式顯示時表示，當橫式顯示時座標須逆時針轉90度
void ILI9225_SetXY(int x1, int y1, int x2, int y2, int sx, int sy)
{
 ILI9225_WR_Cmd_Data(0x36,x2); // HEX
 ILI9225_WR_Cmd_Data(0x37,x1); // HSX
 ILI9225_WR_Cmd_Data(0x38,y2); // VEY
 ILI9225_WR_Cmd_Data(0x39,y1); // VSY
 ILI9225_WR_Cmd_Data(0x20,sx); // GRAM start address low  byte
 ILI9225_WR_Cmd_Data(0x21,sy); // GRAM start address high byte  

 digitalWrite(PIN_LCD_CS, LOW); 
 ILI9225_WR_Cmd(0x22);         // Write Data to GRAM
 digitalWrite(PIN_LCD_CS, HIGH); 
}

// LCD寫入控制命令
// [in] VL 只送低位元組，高位元組一律填0x00
void ILI9225_WR_Cmd(char VL)
{ 
 digitalWrite(PIN_LCD_RS, LOW);   // 晶片選擇線設為低電位 
 ILI9225_WR_Data16(0,VL);         // LCD寫入2個位元組(16 bit)資料
 digitalWrite(PIN_LCD_RS, HIGH);  // 晶片選擇線設為高電位  
}

// LCD寫入2個位元組(16 bit)資料
// [in] VH 為資料高位元組
// [in] VL 為資料低位元組
void ILI9225_WR_Data16(char VH, char VL)
{
 ILI9225_WR_Data8(VH);            // LCD寫入1個(高)位元組資料
 ILI9225_WR_Data8(VL);            // LCD寫入1個(低)位元組資料
}

// LCD寫入1個位元組(8 bit)資料
// [in] VL 為資料
// 寫入時由於I/O腳位分散於Arduino Port C & D，為加快速度採埠直接位元運算方式變更I/O
void ILI9225_WR_Data8(char VL)
{
 PORTC = (PORTC & 0xF0) | (VL & 0x0F);  // 取得資料低4位元送到 PC0 ~ 3
 PORTD = (PORTD & 0x0F) | (VL & 0xF0);  // 取得資料高4位元送到 PD4 ~ 7
 digitalWrite(PIN_LCD_WR, LOW);         // 晶片選擇線設為低電位 
 digitalWrite(PIN_LCD_WR, HIGH);        // 晶片選擇線設為高電位   
}

// LCD寫入一組完整命令(暫存器編號加16bit資料)
// [in] cmd  命令暫存器編號
// [in] data 資料
void ILI9225_WR_Cmd_Data(char cmd, int data)
{
  digitalWrite(PIN_LCD_CS, LOW);        // 晶片選擇線設為低電位  
  ILI9225_WR_Cmd(cmd);                  // 寫入命令  
  ILI9225_WR_Data16(data>>8, data);     // 寫入資料(高位元組，低位元組)
  digitalWrite(PIN_LCD_CS, HIGH);       // 晶片選擇線設為高電位 
}

// LCD清除畫面(全部黑屏)
// 即將GRAM全部填入0x00
void ILI9225_Clr_Screen()
{
  ILI9225_SetXY(0, 0, 175, 219, 175, 0); // 設定視窗範圍為全螢幕
  
  PORTC = (PORTC & 0xF0);                // 清除 PC0 ~ 3
  PORTD = (PORTD & 0x0F);                // 清除 PD4 ~ 7
  digitalWrite(PIN_LCD_CS, LOW);         // 晶片選擇線設為低電位 
  
  for(int i=0; i<220; i++){              // 設定迴圈數為圖框高度
    for(int j=0; j<176*2; j++){          // 設定迴圈數為圖框寬度乘2，每個點為2個位元組(RGB565)
      digitalWrite(PIN_LCD_WR, LOW);     // 設定LCD寫入線為低電位
      digitalWrite(PIN_LCD_WR, HIGH);    // 設定LCD寫入線為高電位
    }
  }
  
  digitalWrite(PIN_LCD_CS, HIGH);        // 晶片選擇線設為高電位 
}

// LCD測試
// 顯示十六色橫條紋於畫面上
void ILI9225_LCD_Test()
{
    ILI9225_SetXY(0, 0, 175, 219, 175, 0);  // 設定視窗範圍為全螢幕
    digitalWrite(PIN_LCD_CS, LOW);          // 晶片選擇線設為低電位
    
    for(int y=0; y<176; y++){               // 設定迴圈數為圖框高度
      int index = (y/11) % 16;              // 取得顏色表索引值
      int color = color_table[index];       // 取得欲寫入顏色內容
      
      for(int x=0; x<220; x++){             // 設定迴圈數為圖框寬度，每個點為2個位元組(RGB565)
        ILI9225_WR_Data16(color>>8, color); // 寫入LCD GRAM中
      }
   }
    
    digitalWrite(PIN_LCD_CS, HIGH);         // 晶片選擇線設為高電位
    delay(1000);                            // 延時1秒
    ILI9225_Clr_Screen();                   // 清除LCD畫面
    digitalWrite(PIN_LED1, HIGH);           // 點亮LED1
}
