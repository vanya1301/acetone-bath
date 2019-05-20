#include "BluetoothSerial.h"
#include <string>

using namespace std;

#define TEMP_SENS 34
BluetoothSerial ESP_BT;
hw_timer_t *timer = NULL;
int LED_BUILTIN = 14;
String command = "";
int temp = 0;
int duration = 0;
int tempSensor = 1;
int seconds = 0;
bool processRunning = false;

void IRAM_ATTR onTimer()
{
  if (--seconds < 0)
  {
    processRunning = false;
  }
}

void setup()
{
  Serial.begin(115200);
  ESP_BT.begin("ESP32_LED_Control");
  Serial.println("Bluetooth Device is Ready to Pair");

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TEMP_SENS, INPUT);
  while (!ESP_BT.hasClient())
  {
    Serial.print(".");
    delay(1000);
  }
  //sensors.begin();
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
        timerAlarmEnable(timer);
        //Serial.println(duration);
        command = "";
      }
      if (command == "s")
      {
        processRunning = false;
        Serial.println("Stop");
        command = "";
      }

      command = "";
    }
  }

  if (processRunning == true)
  {
    Serial.print(seconds);
    Serial.println(" sec");
    Serial.println(tempSensor);
    ESP_BT.print("t");
    ESP_BT.println(tempSensor);
    ESP_BT.print("m");
    ESP_BT.println(seconds / 60 + 1);
    if (tempSensor < temp)
    {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else
    {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
    ESP_BT.println("s");
  }

  delay(1000);
}