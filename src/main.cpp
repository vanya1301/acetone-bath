#include "BluetoothSerial.h"
#include <string>
#include "SSD1306Wire.h"

using namespace std;

#define TEMP_SENS 36
#define HEATER 14
#define FAN 12
#define D5 4
#define D3 5

SSD1306Wire display(0x3c, D3, D5);
BluetoothSerial ESP_BT;
hw_timer_t *timer = NULL;
String command = "";
const int freq = 5000;
const int heaterChannel = 0;
const int resolution = 8;
int temp = 0;
int duration = 0;
int tempSensor = 1;
int seconds = 100;
bool processRunning = false;
bool fanRunning = false;
bool errorShowed = false;
unsigned int heaterPower = 0;

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
    digitalWrite(FAN, LOW);
    Serial.println("Done");

    //display.drawString(20, 0, "FINISHED");
    //display.display();
    seconds = 100;
  }
}

void IRAM_ATTR checkConnection()
{
  display.clear();
  while (!ESP_BT.hasClient())
  {
    if (!errorShowed)
    {
      display.drawString(40, 0, "No");
      display.drawString(0, 30, "Connection");
      display.display();
      errorShowed = true;
    }
    //Serial.print(".");

    //delay(100);
  }
  display.clear();
  display.drawString(20, 0, "Client");
  display.drawString(0, 30, "Connected");
  display.display();

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

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);

  display.drawString(50, 0, "No");
  display.drawString(10, 30, "connection");
  display.display();
  /*display.drawString(0, 0, "ACETONE");
  display.drawString(20, 40, "BATH");
  display.display();*/

  ledcSetup(heaterChannel, freq, resolution);
  ledcAttachPin(HEATER, heaterChannel);

  pinMode(FAN, OUTPUT);
  pinMode(TEMP_SENS, INPUT);
  adcAttachPin(TEMP_SENS);

  checkConnection();
}

void loop()
{
  display.clear();
  tempSensor = (analogRead(TEMP_SENS) / 4096.0) * 500.0;
  //tempSensor = analogRead(TEMP_SENS) /9.31;

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
        /*display.drawString(25, 10, "START");
        display.display();*/

        command.setCharAt(command.lastIndexOf("T"), ' ');
        command.trim();
        temp = command.toInt();
      }
      if (command.startsWith("D"))
      {
        command.setCharAt(command.lastIndexOf("D"), ' ');
        command.trim();
        duration = command.toInt();
        seconds = duration * 60;
        if (!timerStarted(timer))
        {
          timerStart(timer);
        }
      }
      if (command == "s")
      {
        processRunning = false;
        fanRunning = false;
        digitalWrite(FAN, LOW);
        seconds = 100;
        tempSensor = 0;

        display.drawString(40, 20, "STOP");
        display.display();
        Serial.println("Stop");
      }
      if (command == "P")
      {
        processRunning = false;
        fanRunning = false;
        digitalWrite(FAN, LOW);
        timerStop(timer);

        display.drawString(30, 20, "PAUSE");
        display.display();
        Serial.println("Pause");
      }
      if (command == "C")
      {
        processRunning = true;
        fanRunning = false;
        digitalWrite(FAN, HIGH);
        timerStart(timer);
        Serial.println("Continue");
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
    ESP_BT.println((int)tempSensor);
    ESP_BT.print("m");
    ESP_BT.println(seconds);

    display.drawString(0, 0, "Temp:" + String((int)tempSensor) + " °C");

    if (seconds > 60)
      display.drawString(0, 30, "Time:" + String(seconds / 60 + 1) + " min.");
    else
      display.drawString(0, 30, "Time:" + String(seconds) + " sec.");

    display.display();

    if (!fanRunning)
    {
      fanRunning = true;
      digitalWrite(FAN, HIGH);
    }

    /*if (tempSensor < temp)
    {
      digitalWrite(HEATER, HIGH);
      Serial.println("+");
    }
    else
    {
      digitalWrite(HEATER, LOW);
      Serial.println("-");
    }*/
    heaterPower = ((temp - tempSensor) * 10);
    if (temp - tempSensor < 1)
    {
      heaterPower = 10;
    }
    else if ((temp - tempSensor) * 10 > 255)
    {
      heaterPower = 255;
    }
    Serial.println();
    Serial.print("Heater power: ");
    Serial.println(heaterPower);
    ledcWrite(heaterChannel, heaterPower);
  }
  else
  {
    //digitalWrite(HEATER, LOW);
    ledcWrite(heaterChannel, 0);
    //digitalWrite(FAN, LOW);
  }

  if (!ESP_BT.hasClient() && !processRunning)
  {
    errorShowed=false;
    checkConnection();
  }

  delay(500);
}