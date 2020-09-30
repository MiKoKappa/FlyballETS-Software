// file:	FlyballETS-Software.ino summary: FlyballETS-Software v1, by Alex Goris
//
// Flyball ETS (Electronic Training System) is an open source project which is designed to help
// teams who practice flyball (a dog sport). Read all about the project, including extensive
// information on how to build your own copy of Flyball ETS, on the following link: https://
// sparkydevices.wordpress.com/tag/flyball-ets/
//
// This part of the project (FlyballETS-Software) contains the Arduino source code for the Arduino
// Pro Mini which controls all components in the Flyball ETS These sources are originally
// distributed from: https://github.com/vyruz1986/FlyballETS-Software.
//
// Copyright (C) 2019 Alex Goris
// This file is part of FlyballETS-Software
// FlyballETS-Software is free software : you can redistribute it and / or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.If not,
// see <http://www.gnu.org/licenses/>
#include "main.h"
#include "WifiManager.h"

/*List of pins and the ones used (Lolin32 board):
   - 34: S1 (handler side) photoelectric sensor
   - 33: S2 (box side) photoelectric sensor

   - 27: LCD Data7
   - 14: LCD Data6
   - 26: LCD Data5
   - 12: LCD Data4
   -  2: LCD1 (line 1&2) enable pin
   - 15: LCD2 (line 3&4) enable pin
   - 25: LCD RS Pin

   -  0: WS2811B lights data pin / Lights 74HC595 clock pin
   - xx: <free> / Lights 74HC595 data pin
   - xx: <free> / Lights 74HC595 latch pin

   - 19: remote D0
   - 23: remote D1
   - 18: remote D2
   - 17: remote D3
   - 16: remote D4
   -  4: remote D5

   - 35: battery sensor pin

   - 22: Side switch button

   - 32: Laser trigger button
   - 12: Laser output

   -  1: free/TX
   -  3: free/RX
   -  5: free/LED/SS
   - 21: free/SCA
   - 36/VP: GPS rx (ESP tx)
   - 39/VN: GPS tx (ESP rx)
*/
uint8_t iS1Pin = 34;
uint8_t iS2Pin = 33;
uint8_t iCurrentDog;
uint8_t iCurrentRaceState;

char cDogTime[8];
char cDogCrossingTime[8];
char cElapsedRaceTime[8];
char cTotalCrossingTime[8];

//Battery variables
int iBatterySensorPin = 35;
uint16_t iBatteryVoltage = 0;

//Initialise Lights stuff
#ifdef WS281x
uint8_t iLightsDataPin = 0;
NeoPixelBus<NeoRgbFeature, WS_METHOD> LightsStrip(5, iLightsDataPin);

#else
uint8_t iLightsClockPin = 8;
uint8_t iLightsDataPin = 9;
uint8_t iLightsLatchPin = 21;
#endif // WS281x

//Other IO's
uint8_t iLaserTriggerPin = 32;
uint8_t iLaserOutputPin = 12;
boolean bLaserState = false;

stInputSignal SideSwitch = {5, 0, 500};

//Set last serial output variable
long lLastSerialOutput = 0;

//remote control pins
int iRC0Pin = 19;
int iRC1Pin = 23;
int iRC2Pin = 18;
int iRC3Pin = 17;
int iRC4Pin = 16;
int iRC5Pin = 4;
//Array to hold last time button presses
unsigned long lLastRCPress[6] = {0, 0, 0, 0, 0, 0};

uint8_t iLCDData4Pin = 13;
uint8_t iLCDData5Pin = 26;
uint8_t iLCDData6Pin = 14;
uint8_t iLCDData7Pin = 27;
uint8_t iLCDE1Pin = 2;
uint8_t iLCDE2Pin = 15;
uint8_t iLCDRSPin = 25;

LiquidCrystal lcd(iLCDRSPin, iLCDE1Pin, iLCDData4Pin, iLCDData5Pin, iLCDData6Pin, iLCDData7Pin);  //declare two LCD's, this will be line 1&2
LiquidCrystal lcd2(iLCDRSPin, iLCDE2Pin, iLCDData4Pin, iLCDData5Pin, iLCDData6Pin, iLCDData7Pin); //declare two LCD's, this will be line 1&2

//String for serial comms storage
String strSerialData;
byte bySerialIndex = 0;
boolean bSerialStringComplete = false;

//Define serial pins for GPS module
HardwareSerial GPSSerial(1);

//Keep last reported OTA progress so we can send message for every % increment
unsigned int uiLastProgress = 0;

//ESP32 multi core magic :)
#ifdef ESP32
TaskHandle_t Task0;
#endif

WifiManager wifiManager;

void setup()
{
   //Configure serial interface
   Serial.begin(115200);
   strSerialData[0] = 0;

   //init SettingsManager
   SettingsManager.init();
   SettingsManager.setSetting("OperationMode", "0");
   SettingsManager.setSetting("APName", "FlyballETS");

   pinMode(iS1Pin, INPUT_PULLDOWN);
   pinMode(iS2Pin, INPUT_PULLDOWN);

   //Set light data pin as output
   pinMode(iLightsDataPin, OUTPUT);

   //initialize pins for remote control
   pinMode(iRC0Pin, INPUT_PULLDOWN);
   pinMode(iRC1Pin, INPUT_PULLDOWN);
   pinMode(iRC2Pin, INPUT_PULLDOWN);
   pinMode(iRC3Pin, INPUT_PULLDOWN);
   pinMode(iRC4Pin, INPUT_PULLDOWN);
   pinMode(iRC5Pin, INPUT_PULLDOWN);

   //LCD pins as output
   pinMode(iLCDData4Pin, OUTPUT);
   pinMode(iLCDData5Pin, OUTPUT);
   pinMode(iLCDData6Pin, OUTPUT);
   pinMode(iLCDData7Pin, OUTPUT);
   pinMode(iLCDE1Pin, OUTPUT);
   pinMode(iLCDE2Pin, OUTPUT);
   pinMode(iLCDRSPin, OUTPUT);

   //Initialize other I/O's
   pinMode(iLaserTriggerPin, INPUT_PULLUP);
   pinMode(iLaserOutputPin, OUTPUT);
   pinMode(SideSwitch.Pin, INPUT_PULLUP);

   //Set ISR's with wrapper functions
#if !Simulate
   attachInterrupt(digitalPinToInterrupt(iS2Pin), Sensor2Wrapper, CHANGE);
   attachInterrupt(digitalPinToInterrupt(iS1Pin), Sensor1Wrapper, CHANGE);
#endif

   //Initialize BatterySensor class with correct pin
   BatterySensor.init(iBatterySensorPin);

   //Initialize LightsController class with shift register pins
#ifdef WS281x
   LightsController.init(&LightsStrip);
#else
   LightsController.init(iLightsLatchPin, iLightsClockPin, iLightsDataPin);
#endif

   //Initialize LCDController class with lcd1 and lcd2 objects
   LCDController.init(&lcd, &lcd2);

   //Init Wifi setup
   wifiManager.SetupWiFi();

   //configure webserver
   WebHandler.init(80);

   //Initialize RaceHandler class with S1 and S2 pins
   RaceHandler.init(iS1Pin, iS2Pin);

   SystemManager.init();

   //Initialize simulatorclass pins if applicable
#if Simulate
   Simulator.init(iS1Pin, iS2Pin);
#endif

   //Ota setup
   ArduinoOTA.setPassword("FlyballETS.1234");
   ArduinoOTA.setPort(3232);
   ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
         type = "sketch";
      else // U_SPIFFS
         type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      ESP_LOGI(__FILE__, "Start updating %s", type);
   });

   ArduinoOTA.onEnd([]() {
      ESP_LOGI(__FILE__, "[OTA]: End");
   });
   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      unsigned int progressPercentage = (progress / (total / 100));
      if (uiLastProgress != progressPercentage)
      {
         ESP_LOGI(__FILE__, "[OTA]: Progress: %u%%", progressPercentage);
         uiLastProgress = progressPercentage;
      }
   });
   ArduinoOTA.onError([](ota_error_t error) {
      ESP_LOGE(__FILE__, "[OTA]: Error[%u]: ", error);
   });
   ArduinoOTA.begin();

   //Initialize GPS Serial port and class
   GPSSerial.begin(9600, SERIAL_8N1, 39, 36);
   GPSHandler.init(&GPSSerial);

#ifdef ESP32
   mdnsServerSetup();

   xTaskCreatePinnedToCore(
       Core0Loop,
       "Task0",
       8192,
       NULL,
       1,
       &Task0,
       0);
#endif //  ESP32

   ESP_LOGI(__FILE__, "Ready, version %s", APP_VER);
}

void loop()
{
   //Handle OTA update if incoming
   ArduinoOTA.handle();

   //Handle lights main processing
   LightsController.Main();

   //Check for serial events
   serialEvent();

   //Handle Race main processing
   RaceHandler.Main();

   //Handle battery sensor main processing
   BatterySensor.CheckBatteryVoltage();

   //Handle LCD processing
   LCDController.Main();

   //Handle WebSocket server
   WebHandler.loop();

   //Handle settings manager loop
   SettingsManager.loop();

   //Handle GPS
   GPSHandler.loop();

   wifiManager.WiFiLoop();

   SystemManager.loop();

#if Simulate
   //Run simulator
   Simulator.Main();
#endif

   HandleSerialMessages();

   HandleRemoteControl();

   HandleLCDUpdates();

   if (iCurrentRaceState != RaceHandler.RaceState)
   {
      if (RaceHandler.RaceState == RaceHandler.STOPPED)
      {
         //Race is finished, put final data on screen
         dtostrf(RaceHandler.GetDogTime(RaceHandler.iCurrentDog, -2), 7, 3, cDogTime);
         ESP_LOGI(__FILE__, "D%i: %s|CR: %s", RaceHandler.iCurrentDog, cDogTime, RaceHandler.GetCrossingTime(RaceHandler.iCurrentDog, -2).c_str());
         ESP_LOGI(__FILE__, "RT:%s", cElapsedRaceTime);
      }
      ESP_LOGI(__FILE__, "RS: %i", RaceHandler.RaceState);
   }

   if (RaceHandler.iCurrentDog != iCurrentDog)
   {
      dtostrf(RaceHandler.GetDogTime(RaceHandler.iPreviousDog, -2), 7, 3, cDogTime);
      ESP_LOGI(__FILE__, "D%i: %s|CR: %s", RaceHandler.iPreviousDog, cDogTime, RaceHandler.GetCrossingTime(RaceHandler.iPreviousDog, -2).c_str());
      ESP_LOGI(__FILE__, "D: %i", RaceHandler.iCurrentDog);
      ESP_LOGI(__FILE__, "RT:%s", cElapsedRaceTime);
   }

   //Enable (uncomment) the following if you want periodic status updates on the serial port
   if ((millis() - lLastSerialOutput) > 5000)
   {
      lLastSerialOutput = millis();
      //ESP_LOGI(__FILE__, "%lu: ping! analog: %i ,voltage is: %i, this is %i%%", millis(), BatterySensor.GetLastAnalogRead(), iBatteryVoltage, iBatteryPercentage);
      //ESP_LOGI(__FILE__, "%lu: Elapsed time: %s", millis(), cElapsedRaceTime);
      //ESP_LOGI(__FILE__, "Free heap: %d", system_get_free_heap_size());
      /*
      if (RaceHandler.RaceState == RaceHandler.RUNNING)
      {
         dtostrf(RaceHandler.GetDogTime(RaceHandler.iCurrentDog), 7, 3, cDogTime);
         ESP_LOGI(__FILE__, "Dog %i: %ss", RaceHandler.iCurrentDog, cDogTime);
      }
      */
      ESP_LOGI(__FILE__, "GPS Time: %s\r\n", GPSHandler.GetUTCTimestamp());
   }
   //Cleanup variables used for checking if something changed
   iCurrentDog = RaceHandler.iCurrentDog;
   iCurrentRaceState = RaceHandler.RaceState;

   //Handle laser output
   digitalWrite(iLaserOutputPin, !digitalRead(iLaserTriggerPin));

   //Handle side switch button
   if (digitalRead(SideSwitch.Pin) == LOW && millis() - SideSwitch.LastTriggerTime > SideSwitch.CoolDownTime)
   {
      SideSwitch.LastTriggerTime = millis();
      ESP_LOGI(__FILE__, "Switching sides!");
      RaceHandler.ToggleRunDirection();
   }
}

void serialEvent()
{
   //Listen on serial port

   while (Serial.available() > 0)
   {
      char cInChar = Serial.read(); // Read a character
                                    //Check if buffer contains complete serial message, terminated by newline (\n)
      if (cInChar == '\r')
      {
         if (Serial.available() > 0)
         {
            //More data, so probably \r\n
            cInChar = Serial.read();
         }
         else
         {
            //We only got \r, treat it as newline
            cInChar = '\n';
         }
      }
      if (cInChar == '\n')
      {
         //Serial message in buffer is complete, null terminate it and store it for further handling
         bSerialStringComplete = true;
         strSerialData += '\0'; // Null terminate the string
         break;
      }
      strSerialData += cInChar; // Store it
   }
}

/// <summary>
///   These are wrapper functions which are necessary because it's not allowed to use a class member function directly as an ISR
/// </summary>
void Sensor2Wrapper()
{
   RaceHandler.TriggerSensor2();
}

/// <summary>
///   These are wrapper functions which are necessary because it's not allowed to use a class member function directly as an ISR
/// </summary>
void Sensor1Wrapper()
{
   RaceHandler.TriggerSensor1();
}

/// <summary>
///   Starts (if stopped) or stops (if started) a race. Start is only allowed if race is stopped and reset.
/// </summary>
void StartStopRace()
{
   ESP_LOGI(__FILE__, "StartStopRace called\r\n");
   lLastRCPress[0] = millis();
   if (RaceHandler.RaceState == RaceHandler.STOPPED //If race is stopped
       && RaceHandler.GetRaceTime() == 0)           //and timers are zero
   {
      //Then start the race
      ESP_LOGD(__FILE__, "%lu: START!", millis());
      LightsController.InitiateStartSequence();
      RaceHandler.StartRace();
   }
   else //If race state is running or starting, we should stop it
   {
      RaceHandler.StopRace();
      LightsController.DeleteSchedules();
   }
}

/// <summary>
///   Reset race so new one can be started, reset is only allowed when race is stopped
/// </summary>
void ResetRace()
{
   if (RaceHandler.RaceState != RaceHandler.STOPPED) //Only allow reset when race is stopped first
   {
      return;
   }
   lLastRCPress[1] = millis();
   LightsController.ResetLights();
   RaceHandler.ResetRace();
}

#ifdef ESP32
void mdnsServerSetup()
{
   MDNS.addService("http", "tcp", 80);
   MDNS.addServiceTxt("arduino", "tcp", "app_version", APP_VER);
   MDNS.begin("FlyballETS");
}
#endif

void Core0Loop(void *parameter)
{
   SlaveHandler.init();
   for (;;)
   {
      SlaveHandler.loop();
      //yield();
      vTaskDelay(5);
   }
}

void HandleSerialMessages()
{
   if (!bSerialStringComplete)
   {
      return;
   }
   if (strSerialData.equals("START") || strSerialData.equals("STOP"))
   {
      StartStopRace();
   }

   if (strSerialData == "RESET")
   {
      ResetRace();
   }

   if (strSerialData == "D0F")
   {
      RaceHandler.SetDogFault(0);
   }
   ESP_LOGI(__FILE__, "AP Stopped");

   if (strSerialData == "D1F")
   {
      RaceHandler.SetDogFault(1);
   }

   if (strSerialData == "D2F")
   {
      RaceHandler.SetDogFault(2);
   }

   if (strSerialData == "D3F")
   {
      RaceHandler.SetDogFault(3);
   }

   if (strSerialData.indexOf("SET_LOGLEVEL=") > -1)
   {
      String strLogLevel = strSerialData.substring(13);
      ESP_LOGI(__FILE__, "Setting loglevel to %s", strLogLevel.c_str());
      if (strLogLevel.indexOf("ERROR") > -1)
         esp_log_level_set("*", ESP_LOG_ERROR);
      if (strLogLevel.indexOf("WARN") > -1)
         esp_log_level_set("*", ESP_LOG_WARN);
      if (strLogLevel.indexOf("INFO") > -1)
         esp_log_level_set("*", ESP_LOG_INFO);
      if (strLogLevel.indexOf("DEBUG") > -1)
         esp_log_level_set("*", ESP_LOG_DEBUG);
      if (strLogLevel.indexOf("VERB") > -1)
         esp_log_level_set("*", ESP_LOG_VERBOSE);
   }

   //Make sure this stays last in the function!
   if (strSerialData.length() > 0 && bSerialStringComplete)
   {
      ESP_LOGI(__FILE__, "cSer: '%s'", strSerialData.c_str());

      strSerialData = "";
      bSerialStringComplete = false;
   }
}

void HandleRemoteControl()
{
   //Race start/stop button (remote D0 output)
   if (digitalRead(iRC0Pin) == HIGH && (millis() - lLastRCPress[0] > 2000))
   {
      lLastRCPress[0] = millis();
      StartStopRace();
   }

   //Race reset button (remote D1 output)
   if (digitalRead(iRC1Pin) == HIGH && (millis() - lLastRCPress[1] > 2000))
   {
      lLastRCPress[1] = millis();
      ResetRace();
   }

   //Dog0 fault RC button
   if (digitalRead(iRC2Pin) == HIGH && (millis() - lLastRCPress[2] > 2000))
   {
      lLastRCPress[2] = millis();
      //Toggle fault for dog
      RaceHandler.SetDogFault(0);
   }

   //Dog1 fault RC button
   if (digitalRead(iRC3Pin) == HIGH && (millis() - lLastRCPress[3] > 2000))
   {
      lLastRCPress[3] = millis();
      //Toggle fault for dog
      RaceHandler.SetDogFault(1);
   }
   //Dog2 fault RC button
   if (digitalRead(iRC4Pin) == HIGH && (millis() - lLastRCPress[4] > 2000))
   {
      lLastRCPress[4] = millis();
      //Toggle fault for dog
      RaceHandler.SetDogFault(2);
   }

   //Dog3 fault RC button
   if (digitalRead(iRC5Pin) == HIGH && (millis() - lLastRCPress[5] > 2000))
   {
      lLastRCPress[5] = millis();
      //Toggle fault for dog
      RaceHandler.SetDogFault(3);
   }
}

void HandleLCDUpdates()
{
   //Update LCD Display fields
   //Update team time to display
   dtostrf(RaceHandler.GetRaceTime(), 7, 3, cElapsedRaceTime);
   LCDController.UpdateField(LCDController.TeamTime, cElapsedRaceTime);

   //Update battery percentage to display
   iBatteryVoltage = BatterySensor.GetBatteryVoltage();
   uint16_t iBatteryPercentage = BatterySensor.GetBatteryPercentage();
   LCDController.UpdateField(LCDController.BattLevel, String(iBatteryPercentage));

   //Update total crossing time
   dtostrf(RaceHandler.GetTotalCrossingTime(), 7, 3, cTotalCrossingTime);
   LCDController.UpdateField(LCDController.TotalCrossTime, cTotalCrossingTime);

   //Update race status to display
   LCDController.UpdateField(LCDController.RaceState, RaceHandler.GetRaceStateString());

   //Handle individual dog info
   dtostrf(RaceHandler.GetDogTime(0), 7, 3, cDogTime);
   LCDController.UpdateField(LCDController.D1Time, cDogTime);
   LCDController.UpdateField(LCDController.D1CrossTime, RaceHandler.GetCrossingTime(0));
   LCDController.UpdateField(LCDController.D1RerunInfo, RaceHandler.GetRerunInfo(0));

   dtostrf(RaceHandler.GetDogTime(1), 7, 3, cDogTime);
   LCDController.UpdateField(LCDController.D2Time, cDogTime);
   LCDController.UpdateField(LCDController.D2CrossTime, RaceHandler.GetCrossingTime(1));
   LCDController.UpdateField(LCDController.D2RerunInfo, RaceHandler.GetRerunInfo(1));

   dtostrf(RaceHandler.GetDogTime(2), 7, 3, cDogTime);
   LCDController.UpdateField(LCDController.D3Time, cDogTime);
   LCDController.UpdateField(LCDController.D3CrossTime, RaceHandler.GetCrossingTime(2));
   LCDController.UpdateField(LCDController.D3RerunInfo, RaceHandler.GetRerunInfo(2));

   dtostrf(RaceHandler.GetDogTime(3), 7, 3, cDogTime);
   LCDController.UpdateField(LCDController.D4Time, cDogTime);
   LCDController.UpdateField(LCDController.D4CrossTime, RaceHandler.GetCrossingTime(3));
   LCDController.UpdateField(LCDController.D4RerunInfo, RaceHandler.GetRerunInfo(3));

   bool bMasterSlaveConnectionStatus = SystemManager.CheckMasterSlaveConnection();
   LCDController.UpdateField(LCDController.MasterSlaveConnection, bMasterSlaveConnectionStatus ? "1" : "0");
}