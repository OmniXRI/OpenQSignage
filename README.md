我有沒有聽錯！3.3V / 8MHz 8bit MCU的Arduino Mini Pro竟然可以拿來做「動畫胸牌」，不是LED的那種(圖一左圖)，而是64K彩色LCD的那種(圖一右圖)。它不只可以秀文字還可秀影像，並可選擇多張自己喜歡的影像及輸入自己想要的文字後再下載，更誇張的是竟然沒有用到SD卡，總成本不到二個小朋友(<NT$500)。各位Maker，這是真的！而最主要的功臣就是來揚科技(Lyontek Inc.)的那顆PSRAM(Pseudo Static Random Access Memory)(圖一右圖綠色板子上正中間那顆八隻腳的IC)，到底什麼是PSRAM，又要怎麼才能完成這項作品，就讓我們繼續看下去。

![](https://github.com/OmniXRI/OpenQSignage/blob/master/Figures/OpenQSignage_Fig01.jpg)

圖一、左圖為傳統LED胸牌，右圖為本開源專案迷你電子看板【OpenQSignage】

近似同義名詞補充：電子看板、數位看板(數字標牌)(Digital Signage)、數位相框(Digital Frame)、胸牌(名牌)(Name Badge)、名片型字幕機、廣告機、跑馬燈(Scrolling Advertising)。

前一陣子剛好買了一些LCD模組準備來開源另一個項目，正好MAKERPRO歐兄邀我協助想一些點子來推廣來揚科技(Lyontek Inc.)的PSRAM又可幫助Maker們創作出一些有趣的應用。在沒收業配情況下，經過一番集思廣義後，我決定把原先另一個項目延遲，並把LCD模組先挪過來使用，先推出迷你電子看板【OpenQSignage】並免費開源給各位Maker。

這個開源專案主要是以數位看板(大陸稱為數字標牌)(Digital Signage)俗稱廣告機的架構來實現。一般我們常會在超商或賣場看到此類廣告機，其系統最主要包括一台顯示器、一台本地端主機負責依排程播放內容及一台遠端主機進行內容編輯及多子機管理，或者為了節省成本省去本地端主機，而直接用串流影像方式播放。通常數位看板可以用來顯示靜態影像、動態影片(視頻)及即時文字跑馬，而大型系統還需要一個遠端系統來管理播放排程、更新播放內容及顯示即時資訊，更進階一點的還要提供客戶自行編排播放內容視窗的排列方式。

話說回來，我一個人不可能用幾百塊錢、幾週時間就可完成這麼大的系統，所以這個專案我把它迷你化(Q版)，並將其改成動畫胸(名)牌的應用，方便大家能快速了解整個開發流程及軟硬體架構，所以很多地方寫的不好，有很多改善空間，就留給各位Maker自由發揮，如有任何想法，歡迎來信或留言討論。

目前這個迷你電子看板不只是體積變小，該有的LCD顯示屏、本地端排程播放及內容(影像、文字)編輯及下載功能一樣不缺，可說是麻雀雖小五臟俱全。本地端部份(如圖一右圖)主要由Arduino Pro Mini (3.3V / 8MHz)作為主機，負責接收及播放排程內容，而排程及顯示用影像都儲存在來揚科技提供的那顆PSRAM上，另外還有64K色LCD顯示屏、鋰電池及充放電模組板。而PC端則提供一套排程編輯及下載專屬程式(如圖二)，包括影像選取、轉換成LCD顯示格式、顯示停留時間設定、文字輸入、字體選擇、文字(前、背景)色彩、橫幅文字顯示模式及下載排程到本地端主機等功能。

![](https://github.com/OmniXRI/OpenQSignage/blob/master/Figures/OpenQSignage_Fig02_Converter_UI.JPG)

圖二、PC端排程編輯及影像轉換系統操作介面

本專案目前仍有許多不足的地方有待各位自由發揮，後續如有任何想法或者在實作上遇到任何問題歡迎在下方留言，我會儘我所能回答大家。當然如果有人有興趣把本專案開發成真正的商品，不管是自行發展或找我合作都很歡迎。

本專案目前仍有許多不足的地方有待各位自由發揮，後續如有任何想法或者在實作上遇到任何問題歡迎在下方留言，我會儘我所能回答大家。當然如果有人有興趣把本專案開發成真正的商品，不管是自行發展或找我合作都很歡迎。

作者：歐尼克斯實境互動工作室 https://omnixri.blogspot.tw/ (Jan. 2018)

參考文獻：
1. 來揚科技官網 http://www.lyontek.com.tw/
2. USB轉UART晶片CH340G資料手冊 http://www.datasheet5.com/pdf-local-2195953
3. LCD驅動晶片ILI9225資料手冊 http://www.displayfuture.com/Display/datasheet/controller/ILI9225.pdf
4. Arduino Pro Mini官方資料 https://store.arduino.cc/usa/arduino-pro-mini
