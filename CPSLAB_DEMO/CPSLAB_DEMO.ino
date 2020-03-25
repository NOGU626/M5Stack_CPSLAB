#include <WiFi.h>
#include "NineAxesMotion.h"
#include <Wire.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

const int thureshould = 15;

const char* ssid = "CPSLAB_WLX" ;
const char* password = "6bepa8ideapbu";

NineAxesMotion mySensor;
unsigned long lastStreamTime = 0;
unsigned long lastStreamTime1 = 0;

const int streamPeriod = 60000;
bool updateSensorData = true;
unsigned long previousMillis_timeout = 0;

String output = "";
float sensor_euler_pre[3];
float measure [3];



void setup() {
  Serial.begin(115200);
  I2C.begin();
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println("Connect....");
  Serial.println(WiFi.localIP());

  mySensor.initSensor();
  mySensor.setOperationMode(OPERATION_MODE_NDOF);
  mySensor.setUpdateMode(MANUAL);
  mySensor.updateAccelConfig();
  updateSensorData = true;
}


void webSend() {
  HTTPClient http;
  http.begin("https://logpot.planckunits.io/log");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-authlogpot", "<key※自分で調べて>");
  int httpCode = http.POST(output);
  Serial.printf("Response: %d", httpCode);
}

void _measure(int state) {
  if (updateSensorData)  //Keep the updating of data as a separate task
  {
    mySensor.updateAccel();        //Update the Accelerometer data
    mySensor.updateLinearAccel();  //Update the Linear Acceleration data
    mySensor.updateGravAccel();    //Update the Gravity Acceleration data
    mySensor.updateEuler();        //Update the Euler data into the structure of the object
    mySensor.updateCalibStatus();  //Update the Calibration Status
    updateSensorData = false;
  }

  output = "";
  const int capacity = JSON_OBJECT_SIZE(24);
  StaticJsonDocument<capacity> doc;
  DynamicJsonDocument logs(512);
  doc["topic_id"] = "<key※自分で調べて>";
  doc["device_id"]   = "<key※自分で調べて>";
  JsonArray data = doc.createNestedArray("logs");
  logs["lat"] = "";
  logs["lng"] = "";
  logs["accX"] =  mySensor.readAccelerometer(X_AXIS);//Accelerometer X-Axis data
  logs["accY"] =  mySensor.readAccelerometer(Y_AXIS);//Accelerometer Y-Axis data
  logs["accZ"] =  mySensor.readAccelerometer(Z_AXIS);//Accelerometer Z-Axis data
  logs["magX"] =  mySensor.readGravAcceleration(X_AXIS);//Gravity Acceleration X-Axis data
  logs["magY"] =  mySensor.readGravAcceleration(Y_AXIS);//Gravity Acceleration Y-Axis data
  logs["magZ"] =  mySensor.readGravAcceleration(Z_AXIS);//Gravity Acceleration Z-Axis data
  logs["gyroX"] =  mySensor.readLinearAcceleration(X_AXIS);//Linear Acceleration X-Axis data
  logs["gyroY"] =  mySensor.readLinearAcceleration(Y_AXIS);//Linear Acceleration Y-Axis data
  logs["gyroZ"] =  mySensor.readLinearAcceleration(Z_AXIS);//Linear Acceleration Z-Axis data
  logs["temp"] =  "";
  logs["humid"] =  "";
  logs["airPressure"] = "";
  sensor_euler_pre[0] = mySensor.readEulerPitch();
  sensor_euler_pre[1] = mySensor.readEulerRoll();
  sensor_euler_pre[2] = mySensor.readEulerHeading();
  logs["pitch"] = sensor_euler_pre[0];
  logs["roll"] = sensor_euler_pre[1];
  logs["heading"] = sensor_euler_pre[2];
  logs["volt"] = "";
  logs["ampere"] = "";
  logs["interrupt"] = String(state);
  data.add(logs);
  serializeJson(doc, output);
  updateSensorData = true;
}


void loop() {
  if ((millis() - lastStreamTime) >= streamPeriod) {
    Serial.print("Loop1");
    lastStreamTime = millis();
    _measure(0);
    webSend();
  }

  if ((millis() - lastStreamTime1) >= 1000) {
    Serial.print("Loop2");
    lastStreamTime1 = millis();
    measure[0] = sensor_euler_pre[0];
    measure[1] = sensor_euler_pre[1];
    measure[2] = sensor_euler_pre[2];
    _measure(1);
    int x = (sensor_euler_pre[0] - measure[0]);
    int y = (sensor_euler_pre[1] - measure[1]);
    int z = (sensor_euler_pre[2] - measure[2]);
    if (abs(x) >= thureshould || abs(y) >= thureshould || abs(z) >= thureshould) {
      Serial.print("<<move>>");
      webSend();
    }
  }
}
