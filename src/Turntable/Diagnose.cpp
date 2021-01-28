/*
 Diagnose functions for the turntable
 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 - Check lost stepps
   - At constant speed
   - When acceleration deceleration is used
*/

#include <Arduino.h>
#include <MobaTools.h>  // https://github.com/MicroBahner/MobaTools

#include "Turntable_Config.h"       // Local configuration file. The file could be empty. In this case the default values are used (See below).
#include "Diagnose.h"

#ifndef ENABLE_DIAGNOSE
#define ENABLE_DIAGNOSE              0
#endif

#ifndef DIAG_CONST_SPEED_START_SPEED
#define DIAG_CONST_SPEED_START_SPEED 25000
#endif

#ifndef DIAG_CONST_SPEED_END_SPEED
#define DIAG_CONST_SPEED_END_SPEED   10000
#endif

#ifndef DIAG_CONST_SPEED_SPEED_STEPP
#define DIAG_CONST_SPEED_SPEED_STEPP -1000
#endif

#ifndef DIAG_CONST_SPEED_N_TESTS
#define DIAG_CONST_SPEED_N_TESTS     5
#endif

//#ifndef DIAG_PAUSE_BETWEEN_TESTS
//#define DIAG_PAUSE_BETWEEN_TESTS     0   // [s]
//#endif

#ifndef DIAG_START_OFFSET
#define DIAG_START_OFFSET            3000
#endif





extern class MoToStepper Step1;

bool Get_Zero_SW();
void WaitforZeroSwitch(bool val);
long WaitforZeroSwitch_readSteps_Debounce(bool val);
void WaitforZeroSwitch_and_SetZero();
void WaitUntilStopped(bool DisableA4988);

//----------------------------------------------
long Check_Steps_for_one_Turn(uint16_t CalSpeed)
//----------------------------------------------
{
  long StepsOneTurn;
  Step1.rotate(1);
  Step1.setSpeedSteps(CalSpeed);

  if (Get_Zero_SW())                          // If the zero switch is achtive => Move until it's released
     WaitforZeroSwitch_readSteps_Debounce(0); // Wait until the zero switch is released

  // *** Set the zero position ***
  //DprintlnT("Searching the zero position...");
  WaitforZeroSwitch_and_SetZero();

  // *** Get the total number of steps ***
  WaitforZeroSwitch_readSteps_Debounce(1);    // Debouncing
  WaitforZeroSwitch_readSteps_Debounce(0);    // Wait until the switch is 0
  //DprintlnT("Rotate one turn to get the total number of steps");

  WaitforZeroSwitch(1);                       // Rotate one turn until the switch is active again
  StepsOneTurn = Step1.readSteps();
  Step1.setZero();
  Serial.print(F("Speed: ")); Serial.print(CalSpeed); Serial.print(F(" StepsOneTurn: ")); Serial.println(StepsOneTurn);
  Step1.doSteps((long)-DIAG_START_OFFSET); // Move back (To start at the same position)
  WaitUntilStopped(false);
  return StepsOneTurn;
}

//-----------------------------------------------------------------
void Check_Steps_for_one_Turn_N_times(uint16_t CalSpeed, uint8_t n)
//-----------------------------------------------------------------
{
 long StepsOneTurn_Sum = 0;
 long MaxVal = 0;
 long MinVal = 0x7FFFFFFF;
 for (uint8_t i = 0; i < n; i++)
    {
    long StepsOneTurn = Check_Steps_for_one_Turn(CalSpeed);
    StepsOneTurn_Sum += StepsOneTurn;
    if (StepsOneTurn > MaxVal) MaxVal = StepsOneTurn;
    if (StepsOneTurn < MinVal) MinVal = StepsOneTurn;
    }
 Serial.print(F(". . . .\t\t\t\t\t\t\tAvg: ")); Serial.print(StepsOneTurn_Sum/n); Serial.print(F(" MaxDif: ")); Serial.println(MaxVal - MinVal);
}

//----------------------------------
void Check_Steps_at_Constant_Speed()
//----------------------------------
// Check the number of steps for one turn at different but constant speeds
{
  uint16_t CalSpeed = DIAG_CONST_SPEED_START_SPEED;
  do {
     Check_Steps_for_one_Turn_N_times(CalSpeed, DIAG_CONST_SPEED_N_TESTS);
     CalSpeed += DIAG_CONST_SPEED_SPEED_STEPP;
     } while((DIAG_CONST_SPEED_SPEED_STEPP > 0 && CalSpeed <= DIAG_CONST_SPEED_END_SPEED) ||
             (DIAG_CONST_SPEED_SPEED_STEPP < 0 && CalSpeed >= DIAG_CONST_SPEED_END_SPEED));
}
/*

Lego "Stepper" ohne Getriebe und ohne Reibung, A4988, MS1-3 +5v, 0.2A, 0.152V, 14V, A4988
Speed: 5000 StepsOneTurn: 32752
Speed: 5000 StepsOneTurn: 32762
Speed: 5000 StepsOneTurn: 32764
Speed: 5000 StepsOneTurn: 32771
Speed: 5000 StepsOneTurn: 32769
. . . .                         Avg: 32763 MaxDif: 19
Speed: 6000 StepsOneTurn: 32767
Speed: 6000 StepsOneTurn: 32763
Speed: 6000 StepsOneTurn: 32768
Speed: 6000 StepsOneTurn: 32769
Speed: 6000 StepsOneTurn: 32768
. . . .                         Avg: 32767 MaxDif: 6
Speed: 7000 StepsOneTurn: 32765
Speed: 7000 StepsOneTurn: 32769
Speed: 7000 StepsOneTurn: 32786
Speed: 7000 StepsOneTurn: 32768
Speed: 7000 StepsOneTurn: 32769
. . . .                         Avg: 32771 MaxDif: 21
Speed: 8000 StepsOneTurn: 32767
Speed: 8000 StepsOneTurn: 32768
Speed: 8000 StepsOneTurn: 32767
Speed: 8000 StepsOneTurn: 32766
Speed: 8000 StepsOneTurn: 32768
. . . .                         Avg: 32767 MaxDif: 2
Speed: 9000 StepsOneTurn: 32765
Speed: 9000 StepsOneTurn: 32768
Speed: 9000 StepsOneTurn: 32768
Stepper war am Ende so Heiß, dass man sich die Finger verbrennt wenn man ihn lönger als 2 Sek. berührt


Großer Stepper mit Lego ohne Getriebe und ohne Reibung, A4988, MS1-3 +5v, 0.2A, 0.152V, 14V, A4988

Speed: 5000 StepsOneTurn: 3200
Speed: 5000 StepsOneTurn: 3200
Speed: 5000 StepsOneTurn: 3200
Speed: 5000 StepsOneTurn: 3200
Speed: 5000 StepsOneTurn: 3200
. . . .                         Avg: 3200 MaxDif: 0
Speed: 6000 StepsOneTurn: 3200
Speed: 6000 StepsOneTurn: 3200
Speed: 6000 StepsOneTurn: 3200
Speed: 6000 StepsOneTurn: 3200
Speed: 6000 StepsOneTurn: 3200
. . . .                         Avg: 3200 MaxDif: 0
Speed: 7000 StepsOneTurn: 3200
Speed: 7000 StepsOneTurn: 3200
Speed: 7000 StepsOneTurn: 3200
Speed: 7000 StepsOneTurn: 3200
Speed: 7000 StepsOneTurn: 3200
. . . .                         Avg: 3200 MaxDif: 0
Speed: 8000 StepsOneTurn: 3200
Speed: 8000 StepsOneTurn: 3200
Speed: 8000 StepsOneTurn: 3200
Speed: 8000 StepsOneTurn: 3200
Speed: 8000 StepsOneTurn: 3200
. . . .                         Avg: 3200 MaxDif: 0
Speed: 9000 StepsOneTurn: 3200
Speed: 9000 StepsOneTurn: 3200
Speed: 9000 StepsOneTurn: 3200
Speed: 9000 StepsOneTurn: 3200

Speed: 10000 StepsOneTurn: 3200
Speed: 10000 StepsOneTurn: 3200
Speed: 10000 StepsOneTurn: 3200
Speed: 10000 StepsOneTurn: 3200
Speed: 10000 StepsOneTurn: 3200
. . . .                         Avg: 3200 MaxDif: 0
Speed: 11000 StepsOneTurn: 3200
Speed: 11000 StepsOneTurn: 3520   <= Von Hand angehalten
Speed: 11000 StepsOneTurn: 3200
Speed: 11000 StepsOneTurn: 3200
Speed: 11000 StepsOneTurn: 3200
. . . .                         Avg: 3264 MaxDif: 320
Speed: 12000 StepsOneTurn: 3200
Speed: 12000 StepsOneTurn: 3201
Speed: 12000 StepsOneTurn: 3201
Speed: 12000 StepsOneTurn: 3200
Speed: 12000 StepsOneTurn: 3200
. . . .                         Avg: 3200 MaxDif: 1
Speed: 13000 StepsOneTurn: 3201
Speed: 13000 StepsOneTurn: 3200
Speed: 13000 StepsOneTurn: 3200
Speed: 13000 StepsOneTurn: 3201
Speed: 13000 StepsOneTurn: 3200
. . . .                         Avg: 3200 MaxDif: 1
Speed: 14000 StepsOneTurn: 3202
Speed: 14000 StepsOneTurn: 3200
Speed: 14000 StepsOneTurn: 3200
Speed: 14000 StepsOneTurn: 3201
Speed: 14000 StepsOneTurn: 3199
. . . .                         Avg: 3200 MaxDif: 3
Speed: 15000 StepsOneTurn: 3199
Speed: 15000 StepsOneTurn: 3200

Bei Speed 500 ruckelt der Stepper
 => mit Ossi anschauen

Mit dem TMC2100 läuft der Spielzeig Stepper auch super gut ohne Last
Eine Umderhung sind 32767 - 32768 Stepps (0x8000)
Selbst bei Speed 25000
Speed: 25000 StepsOneTurn: 32768
Speed: 25000 StepsOneTurn: 32767
Speed: 25000 StepsOneTurn: 32767
Speed: 25000 StepsOneTurn: 32767
Speed: 25000 StepsOneTurn: 32767
. . . .                         Avg: 32767 MaxDif: 1
Speed: 24000 StepsOneTurn: 32767
Speed: 24000 StepsOneTurn: 32767
Speed: 24000 StepsOneTurn: 32767
Speed: 24000 StepsOneTurn: 32767
Speed: 24000 StepsOneTurn: 32767
. . . .                         Avg: 32767 MaxDif: 0
Speed: 23000 StepsOneTurn: 32767
Speed: 23000 StepsOneTurn: 32767
Speed: 23000 StepsOneTurn: 32767
Speed: 23000 StepsOneTurn: 32768
Speed: 23000 StepsOneTurn: 32768
. . . .                         Avg: 32767 MaxDif: 1
Speed: 22000 StepsOneTurn: 32767
Speed: 22000 StepsOneTurn: 32767
Speed: 22000 StepsOneTurn: 32767
Speed: 22000 StepsOneTurn: 32768
Speed: 22000 StepsOneTurn: 32768
. . . .                         Avg: 32767 MaxDif: 1
Speed: 21000 StepsOneTurn: 32767
Speed: 21000 StepsOneTurn: 32768
Speed: 21000 StepsOneTurn: 32768
Speed: 21000 StepsOneTurn: 32768
Speed: 21000 StepsOneTurn: 32767
. . . .                         Avg: 32767 MaxDif: 1
Speed: 20000 StepsOneTurn: 32767
Speed: 20000 StepsOneTurn: 32768
Speed: 20000 StepsOneTurn: 32768
Speed: 20000 StepsOneTurn: 32768
Speed: 20000 StepsOneTurn: 32767
. . . .                         Avg: 32767 MaxDif: 1
Speed: 19000 StepsOneTurn: 32767
Speed: 19000 StepsOneTurn: 32767
Speed: 19000 StepsOneTurn: 32767
Speed: 19000 StepsOneTurn: 32767
Speed: 19000 StepsOneTurn: 32767
. . . .                         Avg: 32767 MaxDif: 0
Speed: 18000 StepsOneTurn: 32768
Speed: 18000 StepsOneTurn: 32767
Speed: 18000 StepsOneTurn: 32768

Alt                             Neu
Port[1]=12423 12173 2000000000  Port[1]=12407 12173 2000000000
Port[2]=13123 12935 2000000000  Port[2]=13123 12935 2000000000
Port[3]=13822 13594 2000000000  Port[3]=13822 13594 2000000000
Port[4]=14588 14301 2000000000  Port[4]=14588 14301 2000000000
Port[5]=15271 14984 2000000000  Port[5]=15271 14984 2000000000
Port[6]=15953 15666 2000000000  Port[6]=15953 15666 2000000000
Port[7]=16636 16349 2000000000  Port[7]=16636 16349 2000000000
Port[8]=17319 17032 2000000000  Port[8]=17319 17032 2000000000
Port[9]=18002 17715 2000000000  Port[9]=18002 17715 2000000000













*/

//----------------
void Do_Diagnose()
//----------------
{
#if ENABLE_DIAGNOSE
  Check_Steps_at_Constant_Speed();
  Serial.println(F("\nEnd"));
  while(1);
#endif
}
