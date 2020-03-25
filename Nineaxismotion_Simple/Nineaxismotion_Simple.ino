#include <ArduinoJson.h>
#include "NineAxesMotion.h"
#include <Wire.h>

NineAxesMotion mySensor;
unsigned long lastStreamTime = 0;
const int streamPeriod = 40;
bool updateSensorData = true;
void setup()
{
  //Peripheral Initialization
  Serial.begin(115200);           //Initialize the Serial Port to view information on the Serial Monitor
  I2C.begin();                    //Initialize I2C communication to the let the library communicate with the sensor.
  //Sensor Initialization
  mySensor.initSensor();          //The I2C Address can be changed here inside this function in the library
  mySensor.setOperationMode(OPERATION_MODE_NDOF);   //Can be configured to other operation modes as desired
  mySensor.setUpdateMode(MANUAL);  //The default is AUTO. Changing to manual requires calling the relevant update functions prior to calling the read functions
  //Setting to MANUAL requires lesser reads to the sensor
  mySensor.updateAccelConfig();
  updateSensorData = true;
  Serial.println();
  Serial.println("Default accelerometer configuration settings...");
  Serial.print("Range: ");
  Serial.println(mySensor.readAccelRange());
  Serial.print("Bandwidth: ");
  Serial.println(mySensor.readAccelBandwidth());
  Serial.print("Power Mode: ");
  Serial.println(mySensor.readAccelPowerMode());
  Serial.println("Streaming in ..."); //Countdown
  Serial.print("3...");
  delay(1000);  //Wait for a second
  Serial.print("2...");
  delay(1000);  //Wait for a second
  Serial.println("1...");
  delay(1000);  //Wait for a second
}

void loop() //This code is looped forever
{
  if (updateSensorData)  //Keep the updating of data as a separate task
  {
    mySensor.updateAccel();        //Update the Accelerometer data
    mySensor.updateLinearAccel();  //Update the Linear Acceleration data
    mySensor.updateGravAccel();    //Update the Gravity Acceleration data
    mySensor.updateCalibStatus();  //Update the Calibration Status
    updateSensorData = false;
  }
  if ((millis() - lastStreamTime) >= streamPeriod)
  {
    lastStreamTime = millis();
    String output;
    DynamicJsonDocument doc(1024);
    DynamicJsonDocument logs(512);

    doc["topic_id"] = "demo";
    doc["device_id"]   = "from-insomunia";
    JsonArray data = doc.createNestedArray("logs");
    logs["lat"] = "";
    logs["lng"] = "";
    logs["acc_x"] =  mySensor.readAccelerometer(X_AXIS);//Accelerometer X-Axis data
    logs["acc_y"] =  mySensor.readAccelerometer(Y_AXIS);//Accelerometer Y-Axis data
    logs["acc_z"] =  mySensor.readAccelerometer(Z_AXIS);//Accelerometer Z-Axis data
    logs["mag_x"] =  mySensor.readGravAcceleration(X_AXIS);//Gravity Acceleration X-Axis data
    logs["mag_y"] =  mySensor.readGravAcceleration(Y_AXIS);//Gravity Acceleration Y-Axis data
    logs["mag_z"] =  mySensor.readGravAcceleration(Z_AXIS);//Gravity Acceleration Z-Axis data
    logs["gyr_x"] =  mySensor.readLinearAcceleration(X_AXIS);//Linear Acceleration X-Axis data
    logs["gyr_y"] =  mySensor.readLinearAcceleration(Y_AXIS);//Linear Acceleration Y-Axis data
    logs["gyr_z"] =  mySensor.readLinearAcceleration(Z_AXIS);//Linear Acceleration Z-Axis data
    logs["temp"] =  22.2;
    logs["humid"] =  33.3;
    logs["air_pressure"] = 1111.1;
    logs["pitch"] = 1.11;
    logs["roll"] = 2.22;
    logs["heading"] = 3.33;
    logs["volt"] = 1.11;
    logs["ampere"] = 1.11;

    data.add(logs);

    serializeJson(doc, output);
    Serial.println(output);

    updateSensorData = true;
  }
}
