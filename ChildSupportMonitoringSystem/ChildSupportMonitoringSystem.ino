#include <TinyGPS++.h>
#include <TinyGPSPlus.h>

#include "ESP8266.h"
#include <SoftwareSerial.h>

// Wi-Fi SSID
#define SSID        "yukitaka"//ルーターのSSID
// Wi-Fi PASSWORD
#define PASSWORD    "20001005"//ルーターのパスワード
// サーバーのホスト名
#define HOST_NAME   "kamake.co.jp"//KAMAKEサーバーのURL
// ポート番号
#define HOST_PORT   80

#define FILE_NAME "logSent.php"

int n = 0;
 
ESP8266 wifi(Serial1);

TinyGPSPlus gps;
SoftwareSerial mySerial(10, 11); // RX, TX

#define DIR_NAME      "http://kamake.co.jp/iot-seminar/20221021/0121/"  //サーバーのパス（配布したパスはここに入力してください）
#define SENT_MESSAGE  "hoge"                        //受信内容
#define MAIL_ADDRESS  "smyktk12@gmail.com"                 //メールアドレス
#define MAIL_SUBJECT  "通報"                  //メール件名
#define MAIL_MESSAGE  "大変です"                //メール本文

#define C 1200
#define buttonON LOW
#define pinponTime 100

  // サーバーへ渡す情報
const char ledGet[] PROGMEM = "GET "DIR_NAME"/ledGet.php HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
const char logGet[] PROGMEM = "GET "DIR_NAME"/logGet.php HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
const char logSent[] PROGMEM = "GET "DIR_NAME"/logSent.php?message="SENT_MESSAGE" HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
const char mailSent[] PROGMEM = "GET "DIR_NAME"/mailSent.php?address="MAIL_ADDRESS"&subject="MAIL_SUBJECT"&message="MAIL_MESSAGE" HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";

const char *const send_table[] PROGMEM = {
  ledGet , logGet , mailSent
};

String str1;
String str2;
String str3;
String str_send;
int state;
char buttonState;
unsigned long time_data1;
unsigned long time_data2;
bool mail_able;

/**
 * 初期設定
 */

void sw(void);
 
void setup(void)
{
  // デジタル13番ピンを出力として設定
  pinMode(13, OUTPUT);

  pinMode(12,OUTPUT);
  pinMode(2,INPUT_PULLUP);

  state = 0;
  buttonState = "HIGH";
  mail_able = false;

  Serial.begin(115200);

  while (!Serial) {
   ; // wait for serial port to connect. Needed for native USB port only
   }
 
   Serial.println("Goodnight moon!");
   
   // set the data rate for the SoftwareSerial port
   mySerial.begin(9600);
   mySerial.println("Hello, world?");

  while (1) {
    Serial.print("restaring esp8266...");
    if (wifi.restart()) {
      Serial.print("ok\r\n");
      break;
    }
    else {
      Serial.print("not ok...\r\n");
      Serial.print("Trying to kick...");
      while (1) {
        if (wifi.kick()) {
          Serial.print("ok\r\n");
          break;
        }
        else {
          Serial.print("not ok... Wait 5 sec and retry...\r\n");
          delay(5000);
        }
      }
    }
  }
  
  Serial.print("setup begin\r\n");

  Serial.print("FW Version:");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStationSoftAP()) {
    Serial.print("to station + softap ok\r\n");
  } else {
    Serial.print("to station + softap err\r\n");
  }

  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print("Join AP success\r\n");
    Serial.print("IP:");
    Serial.println( wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  if (wifi.disableMUX()) {
    Serial.print("single ok\r\n");
  } else {
    Serial.print("single err\r\n");
  }
 
  Serial.print("setup end\r\n");

  attachInterrupt(0,sw, CHANGE);
}

//
// ループ処理
//
void loop(void)
{
  uint8_t buffer[340] = {0};
  char sendStr[280];
  char sendStr1[280];
  static int j = 0;

  str1 = "GET "DIR_NAME"/logSent.php?message=";
  str2 = " HTTP/1.0\r\nHost: kamake.co.jp\r\nUser-Agent: arduino\r\n\r\n";
  str_send = str1 + "http://maps.google.com/maps/?q=" + gps.location.lat() + "," + gps.location.lng() + str2;
  str_send.toCharArray(sendStr1,280);
  Serial.println(sendStr1);
        
  if(j==2 && mail_able==1){
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
      Serial.print("create tcp ok\r\n");
    } else {
      Serial.print("create tcp err\r\n");
    }
    
    strcpy_P(sendStr, (char *)pgm_read_word(&(send_table[j])));
    Serial.println(sendStr);
  
    wifi.send((const uint8_t*)sendStr, strlen(sendStr));

    mail_able = false;
    
  }

  else if(j==0 || j==1){
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
      Serial.print("create tcp ok\r\n");
    } else {
      Serial.print("create tcp err\r\n");
    }
    
    strcpy_P(sendStr, (char *)pgm_read_word(&(send_table[j])));
    Serial.println(sendStr);
  
    wifi.send((const uint8_t*)sendStr, strlen(sendStr));
    
  }


  //サーバからの文字列を入れるための変数
  String resultCode = "";

  // 取得した文字列の長さ
  uint32_t len = wifi.recv(buffer, sizeof(buffer), 10000);

  // 取得した文字数が0でなければ
  if (len > 0) {
    for(uint32_t i = 0; i < len; i++) {
      resultCode += (char)buffer[i];
    }

    // lastIndexOfでresultCodeの最後から改行を探す
    int lastLF = resultCode.lastIndexOf('\n');

    // resultCodeの長さを求める
    int resultCodeLength = resultCode.length();
  
    // substringで改行コードの次の文字から最後までを求める
    String resultString = resultCode.substring(lastLF+1, resultCodeLength);

    // 前後のスペースを取り除く
    resultString.trim();

    Serial.print("resultString = ");
    Serial.println(resultString);

    if(j == 0){
      // 取得した文字列がONならば
      if(resultString == "ON") {
        digitalWrite(13, HIGH);
      } else {
        digitalWrite(13, LOW);
      }
    }
  }

  if(j < 2){
    j = j+1;
  }
  else{
    j = 0;
  }
  
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
    Serial.print("create tcp ok1\r\n");
  } else {
    Serial.print("create tcp err1\r\n");
  }

    char sendStr2[280];
    //sprintf(sendStr, "GET /%s?data=%d HTTP/1.0\r\nHost: %s\r\nUser-Agent: arduino\r\n\r\n", FILE_NAME, n, HOST_NAME);
    //sprintf(sendStr2, sendStr1);
    Serial.println(sendStr1);
    wifi.send((const uint8_t*)sendStr1, strlen(sendStr1));

    gps1();
}

void sw(void){
  if(state!=2){
    if(digitalRead(2)==buttonON){
      delay(10);
  
      time_data1 = millis();
      state=1; //押された状態にする
  
      Serial.println("A");
  
      buttonState = buttonON;
    }
    else{
      delay(10);
      time_data2 = millis();
      Serial.println(time_data2-time_data1);
      
      if((time_data2-time_data1)>2500){
        Serial.println("C");
        mail_able = true;
        tone(12,C);
        state=2;
      }
      else state=0;
      time_data1 = time_data2;
    }
  }

  if(digitalRead(2)==buttonON && state==2){
    delay(10);
    noTone(12);
    time_data1 = time_data2;
    
    state=0;
  }
}

void gps1(){
  Serial.println(gps.location.isUpdated());
  while (mySerial.available() > 0){
   char c = mySerial.read();
   Serial.print(c);
   gps.encode(c);
   if (gps.location.isUpdated()){
   Serial.print("LAT="); Serial.println(gps.location.lat(), 6);
   Serial.print("LONG="); Serial.println(gps.location.lng(), 6);
   Serial.print("ALT="); Serial.println(gps.altitude.meters());
   }
  }
}
