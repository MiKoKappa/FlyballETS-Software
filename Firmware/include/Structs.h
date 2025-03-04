#pragma once
#include <rom/rtc.h>

struct stDogTimeData
{
   String Time;
   String CrossingTime;
};

struct stDogData
{
   uint8_t DogNumber;
   //String DogName;
   stDogTimeData Timing[4];
   bool Running;
   bool Fault;
};

struct stRaceData
{
   int Id;
   unsigned long StartTime;
   unsigned long EndTime;
   String ElapsedTime;
   uint8_t RaceState;
   stDogData DogData[4];
   String NetTime;
   uint8_t RacingDogs;
   bool RerunsOff;
};

struct stSystemData
{
   unsigned long Uptime;
   uint32_t FreeHeap;
   uint16_t PwrOnTag;
   int RaceID;
   uint NumClients;
   char* LocalSystemTime;
   uint BatteryPercentage = 0;
};

struct stLightsState
{
   uint8_t State[5];
};