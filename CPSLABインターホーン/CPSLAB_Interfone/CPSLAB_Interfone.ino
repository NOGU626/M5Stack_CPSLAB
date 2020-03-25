#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_MPL3115A2.h>
#include <M5Stack.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// MQTTの接続先のIP
const char *endpoint = "192.168.12.29";
// MQTTのポート
const int port = 1883;
// デバイスID
char *deviceID = "M5Stack_Room2";  // デバイスIDは機器ごとにユニークにします
// メッセージを知らせるトピック
char *pubTopic = "/pub/M5Stack";
// メッセージを待つトピック
char *subTopic = "/sub/M5Stack";

WiFiClient httpsClient;
PubSubClient mqttClient(httpsClient);

// Wi-FiのSSID
char *ssid = "CPSLAB_WLX";
// Wi-Fiのパスワード
char *password = "6bepa8ideapbu";

int count = 0;
char pubMessage[128];

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

unsigned long prev, next, interval;
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
#define SEC1 35

void playMP3(char *filename,double volume){
  file = new AudioFileSourceSD(filename);
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  out->SetGain(volume);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
  while(mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  }
}

void connectMQTT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect(deviceID)) {
            mqttClient.setCallback(callback);
            Serial.println("Connected.");
            int qos = 0;
            mqttClient.subscribe(subTopic, qos);
            Serial.println("Subscribed.");
        } else {
            Serial.print("Failed. Error state=");
            Serial.print(mqttClient.state());
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
 
void setup() {
  Serial.begin(115200);
  prev = 0;         // 前回実行時刻を初期化
  interval = 5000;   // 実行周期を設定
  
  // Start WiFi
  Serial.println("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
    
  pinMode(SEC1, INPUT);
  M5.begin();
  M5.update();
  M5.Lcd.drawJpgFile(SD, "/invite.jpg");
  mqttClient.setServer(endpoint, port);
  connectMQTT();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String Json = String((char*) payload);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, Json);
  // Test if parsing succeeds.
if (error) {
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.c_str());
  return;
  }
  const char* cmd = doc["cmd"];
  String cmd_s = String((cmd));
  Serial.println(cmd_s);
  if(cmd_s == "fire"){
    Serial.println("火災警報器が作動しました。");
    playMP3("/fireAnno_girl01.mp3",0.5);
  }else if(cmd_s == "fire_s"){
    Serial.println("火災警報器の異常はありませんでした。");
    playMP3("/fireAnno_girl02.mp3",0.5);
  }
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}

 
void loop() {
  // 常にチェックして切断されたら復帰できるように
  mqttLoop();
  
  M5.update();
  M5.Lcd.drawJpgFile(SD, "/invite.jpg");
  unsigned long curr = millis();    // 現在時刻を取得
  
      
      if (M5.BtnA.wasPressed()){
        M5.Lcd.drawJpgFile(SD, "/home.jpg");
        mqttClient.publish("/+/M5Stack", "{\"cmd\": \"call_interview\"}");
        playMP3("/ender.mp3",0.3);
        }else if(M5.BtnB.wasPressed()){
          M5.Lcd.drawJpgFile(SD, "/home.jpg");
          mqttClient.publish("/+/M5Stack", "{\"cmd\": \"call_visit\"}");
          playMP3("/BGM02.mp3",0.3);
          }else if(M5.BtnC.wasPressed()){
            M5.Lcd.drawJpgFile(SD, "/home.jpg");
            mqttClient.publish("/+/M5Stack","{\"cmd\": \"call_other\"}");
            playMP3("/BGM03.mp3",0.3);
            }
      delay(10);
  
}
