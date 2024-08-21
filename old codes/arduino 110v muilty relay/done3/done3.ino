#include <SoftwareSerial.h> // TX RX software library for bluetooth
#include <Servo.h> // servo library 
#include <DS1302.h> //rtc ds1302 library
 
const int kCePin   = 5;  //RST
const int kIoPin   = 6;  //DAT
const int kSclkPin = 7;  //CLK
 
DS1302 rtc(kCePin, kIoPin, kSclkPin);
 
Servo myservo; // servo name

int bluetoothTx = 10; // bluetooth tx to 10 pin
int bluetoothRx = 11; // bluetooth rx to 11 pin

SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

int pos,val,angle,iihour,iiminute;

void printTime() {//日期時間的函式
 Time t = rtc.time();
   char buf[50];
   snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
   int ihour=int(t.hr);
   int iminute=int(t.min); 
   int isecond=int(t.sec);
   Serial.println(buf);
   if(iihour==ihour&&iiminute==iminute&&isecond==0){//自動啟動函式
     Serial.println("自動啟動中...");
     feed();  
     }
   }

void setup()
{
  myservo.attach(9);//attach servo signal wire to pin 9
  myservo.write(0);
  Serial.begin(9600);//Setup usb serial connection to computer
  bluetooth.begin(9600);//Setup Bluetooth serial connection to android
  rtc.writeProtect(true);// 是否防止寫入 (日期時間設定成功後即可改成true)
  rtc.halt(false);// 是否停止計時
  //Time t(2017, 11, 28, 22, 16, 00, Time::kTuesday); //年 月 日 時 分 秒 星期幾 (日期時間設定成功後即可註解掉)
  //rtc.time(t);//設定日期時間 (日期時間設定成功後即可註解掉)
}
void openservo(){//閘門打開函式
  Serial.println("閘門開");
for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 90 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    //Serial.println(pos);
    delay(5);                       // waits 5ms for the servo to reach the position
  }
  }
void closeservo(){//閘門關閉函式
    Serial.println("閘門關");
  for (pos = 90; pos >= 0; pos -= 1) { // goes from 90 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    //Serial.println(pos);
    delay(5);                       // waits 5ms for the servo to reach the position
  }
  }
  
void feed(){//Feed函式
  Serial.println("Feeding....");
  openservo();
  delay(angle);
  closeservo();
  Serial.print("閘門打開時間為:");
  Serial.print(angle);
  Serial.println("毫秒");
}
void loop()
{
  if(bluetooth.available()> 0 ){//連接藍芽時啟動函式
    //Serial.println("connected !");
    val = bluetooth.read(); //設定val等於藍芽接收數值 
   // Serial.println(val);
     if(val>=1,val<=10){//飼料量接收函式
        val=map(val, 1, 10, 1000, 3500);//飼料量轉換函式
        angle=val;
     }
        if(val==200){//Feed函式
          feed();
          }
        if(val==201){//開啟自動啟動函式
          iihour=iihour-25;
          iiminute=iiminute-61;
          Serial.print("已恢復自動啟動時間至: ");
          Serial.print(iihour);
          Serial.print("點");
          Serial.print(iiminute);
          Serial.println("分");
          }
        if(val==199){//關閉自動啟動函式
          iihour=iihour+25;
          iiminute=iiminute+61;
          Serial.println("已取消自動啟動");
          //Serial.println(iihour);
          //Serial.println(iiminute);
          }
        if(val>=20&&val<=43){//設定自動啟動"小時"函式
          iihour=val-20;
          Serial.print("設定自動啟動時間為: ");
          Serial.print(iihour);
          Serial.print("點");
          }
        if(val>=50&&val<=110){//設定自動啟動"分鐘"函式
          iiminute=val-50;
          Serial.print(iiminute);
          Serial.println("分");
          }
          
  }
          printTime();//呼叫自動啟動函式
          myservo.write(0);//伺服馬達歸零
}




