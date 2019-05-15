#include "BluetoothSerial.h"
#include <string>
#include "time.h"

using namespace std;

#define ONE_WIRE_BUS 12
#define TEMP_SENS 34
BluetoothSerial ESP_BT;
int incoming;
int LED_BUILTIN = 14;
String command = "";
String value = "";
int temp = 0;
int duration;
int tempSensor = 1;
int i = 0;
bool processRunning = false;

void setup()
{
  Serial.begin(115200);
  ESP_BT.begin("ESP32_LED_Control");
  Serial.println("Bluetooth Device is Ready to Pair");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TEMP_SENS, INPUT);
  //sensors.begin();
}

void loop()
{
  tempSensor = (analogRead(TEMP_SENS) / 4096.0) * 500.0;

  if (ESP_BT.hasClient())
  {
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
      Serial.println(command);
      Serial.println(tempSensor);
      ESP_BT.println(tempSensor);
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
    }
  }

  delay(1000);
}