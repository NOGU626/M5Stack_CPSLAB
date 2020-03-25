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

bool status_flag = false;
bool status_flag1 = false;
bool status_screen = false;

String output = "";

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

void playMP3(char *filename, double volume) {
  file = new AudioFileSourceSD(filename);
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  out->SetGain(volume);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
  while (mp3->isRunning()) {
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

int BatteryCheck() {
  Wire.beginTransmission(0x75);
  Wire.write(0x78);
  int battlevel = 0;
  byte retval;
  if (Wire.endTransmission(false) == 0 && Wire.requestFrom(0x75, 1)) {
    retval = Wire.read() & 0xF0;
    if (retval == 0xE0) battlevel = 25;
    else if (retval == 0xC0) battlevel = 50;
    else if (retval == 0x80) battlevel = 75;
    else if (retval == 0x00) battlevel = 100;
  }
  return battlevel;
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
  if (cmd_s == "fire" && !status_flag) {
    M5.Lcd.drawJpgFile(SD, "/fire.jpg");
    playMP3("/fireAnno_girl01.mp3", 0.5);
  } else if (cmd_s == "fire_s" && !status_flag) {
    M5.Lcd.drawJpgFile(SD, "/fire.jpg");
    playMP3("/fireAnno_girl02.mp3", 0.5);
  } else if (cmd_s == "todayCook" && !status_flag1) {
    playMP3("/todayCook.mp3", 0.5);
  } else if (cmd_s == "poseGoHome" && !status_flag1) {
    playMP3("/poseGoHome.mp3", 0.5);
  } else if (cmd_s == "matuco" && !status_flag1) {
    playMP3("/matuco.mp3", 0.5);
  } else if (cmd_s == "call_interview" || cmd_s == "call_visit" || cmd_s == "call_other") {
    M5.Lcd.drawJpgFile(SD, "/visiter.jpg");
    playMP3("/visiter_bgm.mp3", 0.3);
  } else if (cmd_s == "setting_a") {
    Serial.println("セッティングモード");
    if (doc["device"] == String(deviceID)) {
      status_flag = doc["value"];
    }
  } else if (cmd_s == "setting_b") {
    Serial.println("セッティングモード");
    if (doc["device"] == String(deviceID)) {
      status_flag1 = doc["value"];
      Serial.println(status_flag1);
    }
  } else if (cmd_s == "turnON") {
    M5.Lcd.wakeup();
    M5.Lcd.setBrightness(200);
    status_screen = false;
  } else if (cmd_s == "turnOFF") {
    M5.Lcd.sleep();
    M5.Lcd.setBrightness(0);
    status_screen = true;
  }
}

void mqttLoop() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
}


void loop() {
  output = "";
  const int capacity = JSON_OBJECT_SIZE(24);
  StaticJsonDocument<capacity> doc;
  // 常にチェックして切断されたら復帰できるように
  mqttLoop();

  M5.update();
  M5.Lcd.drawJpgFile(SD, "/home.jpg");
  unsigned long curr = millis();    // 現在時刻を取得

  if ((curr - prev) >= interval) {  // 前回実行時刻から実行周期以上経過していたら
    prev = curr;                    // 前回実行時刻を現在時刻で更新

    output = "";
    doc.clear();

    doc["DeviceName"] = deviceID;
    doc["data_type"] = "config";
    doc["config1"] = status_flag;
    doc["config2"]   = status_flag1;
    doc["config3"] = status_screen;
    serializeJson(doc, output);
    output.toCharArray(pubMessage, output.length()+1);
    mqttClient.publish(pubTopic, pubMessage);
    Serial.println(output);

    output = "";
    doc.clear();
    IPAddress ip;
    ip = WiFi.localIP();

    doc["DeviceName"] = deviceID;
    doc["data_type"] = "status";
    doc["rssi"] = WiFi.RSSI();
    doc["IP"]   = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    doc["Battery"] = BatteryCheck();

    serializeJson(doc, output);
    output.toCharArray(pubMessage, output.length()+1);
    mqttClient.publish(pubTopic, pubMessage);
    Serial.println(output);
  }


  //      if (M5.BtnA.wasPressed()){
  //        M5.Lcd.drawJpgFile(SD, "/home.jpg");
  //        mqttClient.publish(pubTopic, "{\"cmd\": \"call_interview\"}");
  //        playMP3("/ender.mp3",0.3);
  //        }else if(M5.BtnB.wasPressed()){
  //          M5.Lcd.drawJpgFile(SD, "/home.jpg");
  //          mqttClient.publish(pubTopic, "{\"cmd\": \"call_visit\"}");
  //          playMP3("/BGM02.mp3",0.3);
  //          }else if(M5.BtnC.wasPressed()){
  //            M5.Lcd.drawJpgFile(SD, "/home.jpg");
  //            mqttClient.publish(pubTopic,"{\"cmd\": \"call_other\"}");
  //            playMP3("/BGM03.mp3",0.3);
  //            }
  delay(10);

}
