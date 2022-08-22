// WebHandler.h
#ifndef _WEBHANDLER_h
#define _WEBHANDLER_h

#include "config.h"
#include "global.h"
#include "Arduino.h"
#include "SettingsManager.h"
#include <Hash.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include "SDcardController.h"
#include <ArduinoJson.h>
#include "RaceHandler.h"
#include "Structs.h"
#include "LightsController.h"
#include "BatterySensor.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
// #include <FS.h>
// #include <SPIFFS.h>
#include <ArduinoJson.h>
#include "SettingsManager.h"
#include "GPSHandler.h"
#include "BatterySensor.h"
#include <rom/rtc.h>
#ifndef WebUIonSDcard
#include "index.html.gz.h"
#endif
#include "SlaveHandler.h"
#include "SystemManager.h"


class WebHandlerClass
{
   friend class RaceHandlerClass;

protected:
   AsyncWebServer *_server;
   AsyncWebSocket *_ws;
   AsyncWebSocket *_wsa;
   void _WsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
   bool _DoAction(JsonObject ActionObj, String *ReturnError, AsyncWebSocketClient *Client);
   void _SendRaceData(int iRaceId, int8_t iClientId);
   //bool _GetRaceDataJsonString(int iRaceId, String &strJsonString);
   void _SendLightsData();

   boolean _ProcessConfig(JsonArray newConfig, String *ReturnError);

   boolean _GetData(String dataType, JsonObject ReturnError);

   void _SendSystemData(int8_t iClientId = -1);
   void _onAuth(AsyncWebServerRequest *request);
   bool _authenticate(AsyncWebServerRequest *request);
   bool _wsAuth(AsyncWebSocketClient *client);
   void _onHome(AsyncWebServerRequest *request);
   void _onFavicon(AsyncWebServerRequest *request);
   void _CheckMasterStatus();
   void _DisconnectMaster();

   unsigned long _lLastRaceDataBroadcast;
   unsigned long _lRaceDataBroadcastInterval;
   unsigned long _lLastSystemDataBroadcast;
   unsigned long _lSystemDataBroadcastInterval;
   unsigned long _lLastPingBroadcast;
   unsigned long _lPingBroadcastInterval;
   unsigned long _lWebSocketReceivedTime;
   unsigned long _lLastBroadcast;
   stSystemData _SystemData;
   char _last_modified[50];
   bool _bSlavePresent;

   typedef struct
   {
      bool Configured;
      IPAddress ip;
      AsyncWebSocketClient *client;
      uint8_t ClientID;
      unsigned long LastCheck;
      unsigned long LastReply;
   } stMasterStatus;
   stMasterStatus _MasterStatus;
   typedef struct
   {
      IPAddress ip;
      unsigned long timestamp = 0;
   } ws_ticket_t;
   ws_ticket_t _ticket[WS_TICKET_BUFFER_SIZE];

   bool _bIsConsumerArray[255];
   uint8_t _iNumOfConsumers;

public:
   void init(int webPort);
   void loop();
   bool _bUpdateLights = false;
   bool _bSendRaceData = false;
   bool MasterConnected();
};

extern WebHandlerClass WebHandler;

#endif
