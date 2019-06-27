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
float tempSensor = 1;
int seconds = 100;
bool processRunning = false;
bool fanRunning = false;
bool errorShowed = false;
bool paused = false;
unsigned int heaterPower = 0;

void IRAM_ATTR onTimer();

void IRAM_ATTR checkConnection();

void setup()
{
  Serial.begin(115200);
  ESP_BT.begin("Acetone bath");
  Serial.println("Bluetooth Device is Ready to Pair");

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
  timerStop(timer);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);

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
  //tempSensor = ((analogRead(TEMP_SENS)/819.2) - 0.5) / 0.01;

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
      }
      if (command.startsWith("D"))
      {
        command.setCharAt(command.lastIndexOf("D"), ' ');
        command.trim();
        duration = command.toInt();
        seconds = duration * 60;
      }
      if (command == "s")
      {
        processRunning = false;
        fanRunning = false;
        paused = false;

        timerStop(timer);
        digitalWrite(FAN, LOW);

        seconds = 100;
        tempSensor = 0;

        display.drawString(40, 20, "STOP");
        display.display();
        Serial.println("Stop");
      }
      if (command == "P")
      {
        paused = true;
        processRunning = false;
        fanRunning = false;
        digitalWrite(FAN, LOW);
        timerStop(timer);
        if (timerStarted(timer))
        {
          timerStop(timer);
          digitalWrite(FAN, LOW);
        }

        display.drawString(30, 20, "PAUSE");
        display.display();
        Serial.println("Pause");
      }
      if (command == "C")
      {
        paused = false;
        processRunning = true;
        fanRunning = false;
        digitalWrite(FAN, HIGH);
        if (!timerStarted(timer))
          timerStart(timer);
        Serial.println("Continue");
      }

      command = "";
    }
  }

  if (processRunning == true && seconds > 0)
  {
    if (tempSensor >= temp - 1 && !timerStarted(timer))
    {
      timerStart(timer);
      digitalWrite(FAN, HIGH);
    }
    Serial.print(seconds);
    Serial.println(" sec");
    Serial.println(tempSensor);

    ESP_BT.print("t");
    ESP_BT.println((int)tempSensor);
    ESP_BT.print("m");
    ESP_BT.println(seconds);

    display.drawString(0, 0, "Temp:" + String((int)tempSensor) + " °C");

    if (seconds > 60)
      display.drawString(0, 30, "Time:" + String(seconds / 60) + " min.");
    else
      display.drawString(0, 30, "Time:" + String(seconds) + " sec.");

    display.display();

    if (!fanRunning)
    {
      fanRunning = true;
      digitalWrite(FAN, HIGH);
    }

    heaterPower = ((temp - tempSensor) * temp/2);
    if ((temp-1) - tempSensor < 1)
    {
      heaterPower = 0;
    }
    else if (heaterPower > 255)
    {
      heaterPower = 255;
    }
    Serial.println();
    Serial.print("Heater power: ");
    Serial.println(heaterPower);
    ledcWrite(heaterChannel, heaterPower);
  }
  else if (seconds <= 1 && seconds != -2)
  {
    ESP_BT.println("f");
    processRunning = false;
    fanRunning = false;
    digitalWrite(FAN, LOW);
    Serial.println("Done");
    timerStop(timer);
    display.drawString(20, 20, "Finished");
    display.display();
    seconds = -2;
  }
  else
  {
    ledcWrite(heaterChannel, 0);
    digitalWrite(FAN, LOW);
  }

  if (!ESP_BT.hasClient() && !processRunning)
  {
    errorShowed = false;
    checkConnection();
  }
  delay(1000);
}

void IRAM_ATTR onTimer()
{
  if (seconds > 0)
  {
    seconds--;
  }
}

void IRAM_ATTR checkConnection()
{

  while (!ESP_BT.hasClient())
  {
    if (!errorShowed)
    {
      display.clear();
      display.drawString(50, 0, "No");
      display.drawString(10, 30, "Connection");
      display.display();
      errorShowed = true;
    }
  }

  if (paused)
  {
    
    ESP_BT.println("RP");
    Serial.println("Device connected.");
    Serial.println("SEND PAUSED");
    display.clear();
    display.drawString(30, 20, "PAUSE");
    display.display();
  }
  else
  {
    
    ESP_BT.println("R");
    Serial.println("Device connected.");
    display.clear();
    display.drawString(20, 0, "Client");
    display.drawString(10, 30, "Connected");
    display.display();
  }
}