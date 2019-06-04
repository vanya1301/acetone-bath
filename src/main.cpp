#include "BluetoothSerial.h"
#include <string>

using namespace std;

#define TEMP_SENS 34
#define HEATER 14
#define FAN 12
BluetoothSerial ESP_BT;
hw_timer_t *timer = NULL;
String command = "";
int temp = 0;
int duration = 0;
int tempSensor = 1;
int seconds = 0;
bool processRunning = false;
bool fanRunning = false;

void IRAM_ATTR onTimer()
{
  if (seconds > 0)
  {
    seconds--;
  }
  else if (processRunning)
  {
    processRunning = false;
    fanRunning = false;
    Serial.println("Done");
    seconds = 0;
  }
}

void checkConnection()
{
  while (!ESP_BT.hasClient())
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Device connected.");
  ESP_BT.println("R");
}

void setup()
{
  Serial.begin(115200);
  ESP_BT.begin("ESP32_LED_Control");
  Serial.println("Bluetooth Device is Ready to Pair");

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);

  pinMode(HEATER, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(TEMP_SENS, INPUT);
  checkConnection();
}

void loop()
{
  tempSensor = (analogRead(TEMP_SENS) / 4096.0) * 500.0;

  if (ESP_BT.available())
  {
    command += (char)ESP_BT.read();
    if (command.endsWith("|"))
    {
      processRunning = true;

      command.setCharAt(command.lastIndexOf("|"), ' ');
      command.trim();

      if (command.startsWith("T"))
      {
        
        
        command.setCharAt(command.lastIndexOf("T"), ' ');
        command.trim();
        temp = command.toInt();

        // Serial.println(temp);
        command = "";
      }
      if (command.startsWith("D"))
      {
        command.setCharAt(command.lastIndexOf("D"), ' ');
        command.trim();
        duration = command.toInt();
        seconds = duration * 60;

        //Serial.println(duration);
        command = "";
      }
      if (command == "s")
      {
        processRunning = false;
        fanRunning = false;
        digitalWrite(FAN, LOW);
        Serial.println("Stop");
        command = "";
      }

      command = "";
    }
  }

  if (processRunning == true && seconds > 0)
  {

    Serial.print(seconds);
    Serial.println(" sec");
    Serial.print(tempSensor);
   
    ESP_BT.print("t");
    ESP_BT.println(tempSensor);
    ESP_BT.print("m");
    ESP_BT.println(seconds);
    if(!fanRunning)
    {
      fanRunning = true;
      digitalWrite(FAN,HIGH);
    }

    if (tempSensor < temp)
    {
      digitalWrite(HEATER, HIGH);
      Serial.println("+");
    }
    else
    {
      digitalWrite(HEATER, LOW);
      Serial.println("-");
    }
  }
  else
  {
    digitalWrite(HEATER, LOW);
    digitalWrite(FAN, LOW);
  }

  if (!ESP_BT.hasClient() && !processRunning)
  {
    checkConnection();
  }

  delay(1000);
}