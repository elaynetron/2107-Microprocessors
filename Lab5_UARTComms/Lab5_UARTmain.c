// RSLK Self Test via UART

/* This example accompanies the books
   "Embedded Systems: Introduction to the MSP432 Microcontroller",
       ISBN: 978-1512185676, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Interfacing to the MSP432 Microcontroller",
       ISBN: 978-1514676585, Jonathan Valvano, copyright (c) 2017
   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
       ISBN: 978-1466468863, , Jonathan Valvano, copyright (c) 2017
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/

Simplified BSD License (FreeBSD License)
Copyright (c) 2017, Jonathan Valvano, All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are
those of the authors and should not be interpreted as representing official
policies, either expressed or implied, of the FreeBSD Project.
*/

#include "msp.h"
#include <stdint.h>
#include <string.h>
#include "..\inc\UART0.h"
#include "..\inc\EUSCIA0.h"
#include "..\inc\FIFO0.h"
#include "..\inc\Clock.h"
//#include "..\inc\SysTick.h"
#include "..\inc\SysTickInts.h"
#include "..\inc\CortexM.h"
#include "..\inc\TimerA1.h"
//#include "..\inc\Bump.h"
#include "..\inc\BumpInt.h"
#include "..\inc\LaunchPad.h"
#include "..\inc\Motor.h"
#include "../inc/IRDistance.h"
#include "../inc/ADC14.h"
#include "../inc/LPF.h"
#include "..\inc\Reflectance.h"
#include "../inc/TA3InputCapture.h"
#include "../inc/Tachometer.h"

#define P2_4 (*((volatile uint8_t *)(0x42098070)))
#define P2_3 (*((volatile uint8_t *)(0x4209806C)))
#define P2_2 (*((volatile uint8_t *)(0x42098068)))
#define P2_1 (*((volatile uint8_t *)(0x42098064)))
#define P2_0 (*((volatile uint8_t *)(0x42098060)))

// At 115200, the bandwidth = 11,520 characters/sec
// 86.8 us/character
// normally one would expect it to take 31*86.8us = 2.6ms to output 31 characters
// Random number generator
// from Numerical Recipes
// by Press et al.
// number from 0 to 31
uint32_t Random(void){
static uint32_t M=1;
  M = 1664525*M+1013904223;
  return(M>>27);
}
char WriteData,ReadData;
uint32_t NumSuccess,NumErrors;
void TestFifo(void){char data;
  while(TxFifo0_Get(&data)==FIFOSUCCESS){
    if(ReadData==data){
      ReadData = (ReadData+1)&0x7F; // 0 to 127 in sequence
      NumSuccess++;
    }else{
      ReadData = data; // restart
      NumErrors++;
    }
  }
}
uint32_t Size;
int Program5_1(void){
//int main(void){
    // test of TxFifo0, NumErrors should be zero
  uint32_t i;
  Clock_Init48MHz();
  WriteData = ReadData = 0;
  NumSuccess = NumErrors = 0;
  TxFifo0_Init();
  TimerA1_Init(&TestFifo,43);  // 83us, = 12kHz
  EnableInterrupts();
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      TxFifo0_Put(WriteData);
      WriteData = (WriteData+1)&0x7F; // 0 to 127 in sequence
    }
    Clock_Delay1ms(10);
  }
}

char String[64];
uint32_t MaxTime,First,Elapsed;
int Program5_2(void){
//int main(void){
    // measurement of busy-wait version of OutString
  uint32_t i;
  DisableInterrupts();
  Clock_Init48MHz();
  UART0_Init();
  WriteData = 'a';
  SysTick_Init(0x1000000,2); //OHL - using systick INT api
  MaxTime = 0;
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      String[i] = WriteData;
      WriteData++;
      if(WriteData == 'z') WriteData = 'a';
    }
    String[i] = 0; // null termination
    First = SysTick->VAL;
    UART0_OutString(String);
    Elapsed = ((First - SysTick->VAL)&0xFFFFFF)/48; // usec

    if(Elapsed > MaxTime){
        MaxTime = Elapsed;
    }
    UART0_OutChar(CR);UART0_OutChar(LF);
    Clock_Delay1ms(100);
  }
}

int Program5_3(void){
//int main(void){
    // measurement of interrupt-driven version of OutString
  uint32_t i;
  DisableInterrupts();
  Clock_Init48MHz();
  EUSCIA0_Init();
  WriteData = 'a';
  SysTick_Init(0x1000000,2); //OHL - using systick INT api
  MaxTime = 0;
  EnableInterrupts();
  while(1){
    Size = Random(); // 0 to 31
    for(i=0;i<Size;i++){
      String[i] = WriteData;
      WriteData++;
      if(WriteData == 'z') WriteData = 'a';
    }
    String[i] = 0; // null termination
    First = SysTick->VAL;
    EUSCIA0_OutString(String);
    Elapsed = ((First - SysTick->VAL)&0xFFFFFF)/48; // usec
    if(Elapsed > MaxTime){
        MaxTime = Elapsed;
    }
    EUSCIA0_OutChar(CR);EUSCIA0_OutChar(LF);
    Clock_Delay1ms(100);
  }
}
int Program5_4(void){
//int main(void){
    // demonstrates features of the EUSCIA0 driver
  char ch;
  char string[20];
  uint32_t n;
  DisableInterrupts();
  Clock_Init48MHz();  // makes SMCLK=12 MHz
  EUSCIA0_Init();     // initialize UART
  EnableInterrupts();
  EUSCIA0_OutString("\nLab 5 Test program for EUSCIA0 driver\n\rEUSCIA0_OutChar examples\n");
  for(ch='A'; ch<='Z'; ch=ch+1){// print the uppercase alphabet
     EUSCIA0_OutChar(ch);
  }
  EUSCIA0_OutChar(LF);
  for(ch='a'; ch<='z'; ch=ch+1){// print the lowercase alphabet
    EUSCIA0_OutChar(ch);
  }
  while(1){
    EUSCIA0_OutString("\n\rInString: ");
    EUSCIA0_InString(string,19); // user enters a string
    EUSCIA0_OutString(" OutString="); EUSCIA0_OutString(string); EUSCIA0_OutChar(LF);

    EUSCIA0_OutString("InUDec: ");   n=EUSCIA0_InUDec();
    EUSCIA0_OutString(" OutUDec=");  EUSCIA0_OutUDec(n); EUSCIA0_OutChar(LF);
    EUSCIA0_OutString(" OutUFix1="); EUSCIA0_OutUFix1(n); EUSCIA0_OutChar(LF);
    EUSCIA0_OutString(" OutUFix2="); EUSCIA0_OutUFix2(n); EUSCIA0_OutChar(LF);

    EUSCIA0_OutString("InUHex: ");   n=EUSCIA0_InUHex();
    EUSCIA0_OutString(" OutUHex=");  EUSCIA0_OutUHex(n); EUSCIA0_OutChar(LF);
  }
}

//Bumper - from lab3
volatile uint8_t CollisionData, CollisionFlag;
void BumpCollision(uint8_t bumpSensor){
    CollisionData = bumpSensor;
    CollisionFlag = 1;
    P4->IFG &= ~0xED;                  // clear interrupt flags
}

//IR Sensor - from lab4
volatile uint32_t ADCflag = 0;// used as semaphore
volatile uint32_t nr,nc,nl;
void SensorRead_ISR(void){  // runs at 2000 Hz
    uint32_t raw17,raw12,raw16;
    P1OUT ^= 0x01;         // profile
    P1OUT ^= 0x01;         // profile
    ADC_In17_12_16(&raw17,&raw12,&raw16);  //this the connection
    nr = LPF_Calc(raw17);  // right is channel 17 P9.0
    nc = LPF_Calc2(raw12); // center is channel 12, P4.1
    nl = LPF_Calc3(raw16); // left is channel 16, P9.1
    ADCflag = 1;           // semaphore
    P1OUT ^= 0x01;         // profile
}

void IRSensor_Init(void){
    uint32_t raw17,raw12,raw16;
    uint32_t s;
    s = 256; // replace with your choice
    ADC0_InitSWTriggerCh17_12_16();   // initialize channels 17,12,16
    ADC_In17_12_16(&raw17,&raw12,&raw16);  // sample
    LPF_Init(raw17,s);     // P9.0/channel 17 right
    LPF_Init2(raw12,s);    // P4.1/channel 12 center
    LPF_Init3(raw16,s);    // P9.1/channel 16 left
    TimerA1_Init(&SensorRead_ISR,250);    // 2000 Hz sampling
    ADCflag = 0;
}

//Tachometer
uint16_t Period0, First0; //TimerA3 first edge P10.4
uint16_t Period2, First2; //TimerA3 first edge P8.2
int Done0, Done2;
uint32_t main_count=0;
#define PERIOD 1000

void PeriodMeasure0(uint16_t time){
  P2_0 = P2_0^0x01;           // thread profile, P2.0
  Period0 = (time - First0)&0xFFFF; // 16 bits, 83.3 ns resolution
  First0 = time;                   // setup for next
  Done0 = 1;
}

void PeriodMeasure2(uint16_t time){
  P2_2 = P2_2^0x01;           // thread profile, P2.4
  Period2 = (time - First2)&0xFFFF; // 16 bits, 83.3 ns resolution
  First2 = time;                   // setup for next
  Done2 = 1;
}

void Tachometer_Init(void){
    P2->SEL0 &= ~0x11;
    P2->SEL1 &= ~0x11;  // configure P2.0 and P2.4 as GPIO
    P2->DIR |= 0x11;    // P2.0 and P2.4 outputs
    First0 = First2 = 0; // first will be wrong
    Done0 = Done2 = 0;   // set on subsequent
}

void toggle_GPIO(void){
    P2_4 ^= 0x01;     // create output
}

void TimedPause(uint32_t time){
  Clock_Delay1ms(time);          // run for a while and stop
  Motor_Stop();
  while(LaunchPad_Input()==0);  // wait for touch
  while(LaunchPad_Input());     // wait for release
}

//RSLK Reset - Init and reset all modules used
void RSLK_Reset(void){
    DisableInterrupts();
    Clock_Init48MHz();  // makes SMCLK=12 MHz
    Motor_Init();
    Motor_Stop();
    LaunchPad_Init(); //LED
    Reflectance_Init();
    //Bump_Init();
    //Bumper_Init();
    BumpInt_Init(&BumpCollision);
    IRSensor_Init();
    Tachometer_Init();
    EUSCIA0_Init();     // initialize UART
    EnableInterrupts();
    EUSCIA0_OutString("RSLK Reset Done"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
}

// RSLK Self-Test
int main(void) {
    uint8_t RefData;
    uint32_t cmd=0xDEAD, menu=0;
    CollisionFlag = 0;
    CollisionData = 0x3F;

    DisableInterrupts();
    Clock_Init48MHz();  // makes SMCLK=12 MHz
    //SysTick_Init(48000,2);  // set up SysTick for 1000 Hz interrupts
    Motor_Init();
    Motor_Stop();
    LaunchPad_Init();
    Reflectance_Init();
    //Bump_Init();
    //Bumper_Init();
    BumpInt_Init(&BumpCollision);
    IRSensor_Init();
    Tachometer_Init();
    EUSCIA0_Init();     // initialize UART
    EnableInterrupts();

    while(1){                     // Loop forever
        // write this as part of Lab 5
        EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("RSLK Testing"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[0] RSLK Reset"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[1] Motor Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[2] Reflectance Sensors Test - press any bumper to end"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[3] Bumper Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[4] IR Sensors Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[5] Tachometer Test - press bumper to reduce speed, press launchpad to end"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[6] Extra: Move towards obstacle (IR Sensor) - press launchpad to end"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[7] Extra: Avoid obstacle (IR Sensor) - press launchpad to end"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
        EUSCIA0_OutString("[8] Extra: Avoid obstacle (Bumpers) - press launchpad to end"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);

        EUSCIA0_OutString("CMD: ");
        cmd=EUSCIA0_InUDec(); //Input (ASCII in UDec to 32-bit Unsigned number)
        EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);

        switch(cmd){
            case 0:
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[0] RSLK Reset"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                RSLK_Reset();
                menu=1;
                cmd=0xDEAD;
                break;

            case 1:
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[1] Motor Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Please select motor direction (RPM fixed at 3000)/cancel:"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[0] Motor Forward "); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[1] Motor Backward"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[2] Motor Left"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[3] Motor Right"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[4] Cancel (Back to Main Menu)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Your choice: ");
                uint32_t choice = EUSCIA0_InUDec();
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                switch (choice){
                    case 0:
                        //move forward
                        EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        EUSCIA0_OutString("Moving forward for 3s..."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        Motor_Forward(3000, 3000);
                        Clock_Delay1ms(3000); //delay 3ms
                        Motor_Stop();
                        break;

                    case 1:
                        //move backward
                        EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        EUSCIA0_OutString("Moving backwards for 3s..."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        Motor_Backward(3000, 3000);
                        Clock_Delay1ms(3000); //delay 3ms
                        Motor_Stop();
                        break;

                    case 2:
                        //move left
                        EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        EUSCIA0_OutString("Moving left for 3s..."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        Motor_Left(0, 3000);
                        Clock_Delay1ms(3000); //delay 3ms
                        Motor_Stop();
                        break;

                    case 3:
                        //move right
                        EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        EUSCIA0_OutString("Moving right for 3s..."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                        Motor_Right(3000, 0);
                        Clock_Delay1ms(3000); //delay 3ms
                        Motor_Stop();
                        break;

                    default:
                        //cancel
                        break;
                }
                menu = 1;
                cmd=0xDEAD;
                break;

            case 2:
                CollisionFlag = 0;
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[2] Reflectance Sensors Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Testing 50 samples at intervals of 0.5s, press any bumper to end"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                while(1){
                    //Reflectance Sensors - from lab2
                    RefData = Reflectance_Read(500); //500 -> 0.5s
                    EUSCIA0_OutString("Reflectance Sensor Data Right (1) - Left (8): ");
                    for (int r = 0; r < 8; r++){
                        EUSCIA0_OutUDec(RefData%2);
                        EUSCIA0_OutString(" ");
                        RefData /= 2;
                    }

                    //stop by pressing bumper
                    if(CollisionFlag == 1){
                        break;
                    }
                    EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                }
                menu = 1;
                cmd=0xDEAD;
                break;

            case 3:
                CollisionData=0x3F;
                CollisionFlag=0;
                uint8_t TempCD = 0x3F;
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[3] Bumper Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Please press bumpers."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                while(1){
                    WaitForInterrupt();
                    if (CollisionFlag==1){
                        TempCD = CollisionData;
                        break;
                    }

                }

                //EUSCIA0_OutString("Bumper Readings: "); EUSCIA0_OutUDec(TempCD); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                for(uint8_t b = 1; b < 7; b++ ){
                    if(TempCD %2 == 0){
                        EUSCIA0_OutString("Bumper ");
                        EUSCIA0_OutUDec(b);
                        EUSCIA0_OutString(" was pressed. "); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                    }
                    TempCD = TempCD >> 1;
                }

                TempCD = 0x3F;
                CollisionData = 0x3F;
                CollisionFlag = 0;
                menu=1;
                cmd=0xDEAD;
                break;

            case 4:
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[4] IR Sensors Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Taking 15 IR Sensor Readings"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                TimerA1_Init(&SensorRead_ISR, 250);

                int sample_num = 15; //number of samples to take
                for (int s = 0; s < sample_num; s++){
                    for(int n=0; n<2000; n++){
                        while(ADCflag == 0){};
                        ADCflag = 0; //semaphore
                    }

                    UART0_OutString("Left IR sensor: ");UART0_OutUDec5(LeftConvert(nl));UART0_OutString(" cm,");
                    UART0_OutString("Center IR sensor: ");UART0_OutUDec5(CenterConvert(nc));UART0_OutString(" cm,");
                    UART0_OutString("Right IR sensor: ");UART0_OutUDec5(RightConvert(nr));UART0_OutString(" cm\r\n");
                }
                menu = 1;
                cmd=0xDEAD;
                break;

            case 5:
                CollisionFlag = 0;
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[5] Tachometer Test"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Reduce speed from 3000 to 1000 with any bumper. Press launchpad to end."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                TimerA1_Init(&toggle_GPIO,10);    // 50Khz sampling, period: 10
                TimerA3Capture_Init(&PeriodMeasure0,&PeriodMeasure2);
                Clock_Delay1ms(500);
                Motor_Forward(3000,3000);
                EnableInterrupts();
                while(1){
                    //WaitForInterrupt(); //if using rst
                    main_count++;
                    if(main_count%1000){
                        UART0_OutString("Period0 = ");UART0_OutUDec5(Period0);UART0_OutString(" Period2 = ");UART0_OutUDec5(Period2);UART0_OutString(" \r\n");
                    }
                    if (CollisionFlag) {
                        //reduce speed
                       Motor_Forward(1000,1000);
                    }
                    if(LaunchPad_Input() != 0){
                        //press launchpad to end
                        Motor_Stop();
                        break;
                    }

                }
                Motor_Stop();
                CollisionFlag = 0;
                menu=1;
                cmd=0xDEAD;
                break;

            case 6:
                IRSensor_Init();
                UART0_Init();
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[6] Extra: Move towards obstacle (IR Sensor)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Robot will move towards obstacle within a maximum distance specified, press launchpad to end."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Please specify maximum distance:"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                uint32_t maxdist = EUSCIA0_InUDec(); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);

                while(1){
                    //move in corresponding direction if detected dist < maxdist
                    if(LeftConvert(nl) < maxdist){
                        Motor_Left(3000,3000);
                        Clock_Delay1us(1000);
                        Motor_Stop();
                    }
                    if(RightConvert(nr) < maxdist){
                        Motor_Right(3000,3000);
                        Clock_Delay1us(1000);
                        Motor_Stop();
                    }
                    if(CenterConvert(nc) < maxdist){
                        Motor_Forward(3000,3000);
                        Clock_Delay1us(1000);
                        Motor_Stop();
                    }
                    if(LaunchPad_Input() != 0){
                        //press launchpad to end
                        Motor_Stop();
                        break;
                    }

                }
                menu = 1;
                cmd=0xDEAD;
                break;

            case 7:
                IRSensor_Init();
                UART0_Init();
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[7] Extra: Avoid obstacle (IR Sensor)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Robot will avoid obstacle (using IR Sensors) within 10cm, press launchpad to end."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                // uint32_t userRPM = EUSCIA0_InUDec(); // to get UDec Input for RPM
                uint32_t case7RPM = 1000;
                while (1){
                    Motor_Forward(case7RPM, case7RPM);

                    if(LeftConvert(nl)<=10){
                        Motor_Stop();
                        Motor_Right(case7RPM, case7RPM);
                        Clock_Delay1ms(1500);
                    }
                    if(CenterConvert(nc)<=10){
                        Motor_Stop();
                        Motor_Backward(case7RPM, case7RPM);
                        Clock_Delay1ms(500);
                        Motor_Right(case7RPM, case7RPM);
                        Clock_Delay1ms(1000);
                    }
                    if(RightConvert(nr)<=10){
                        Motor_Stop();
                        Motor_Left(case7RPM, case7RPM);
                        Clock_Delay1ms(1000);
                    }

                    UART0_OutString("Left IR sensor: ");UART0_OutUDec5(LeftConvert(nl));UART0_OutString(" cm,");
                    UART0_OutString("Center IR sensor: ");UART0_OutUDec5(CenterConvert(nc));UART0_OutString(" cm,");
                    UART0_OutString("Right IR sensor: ");UART0_OutUDec5(RightConvert(nr));UART0_OutString(" cm\r\n");

                    if (LaunchPad_Input() != 0) break;
                }

                Motor_Stop();
                menu=1;
                cmd=0xDEAD;
                break;

            case 8:
                CollisionFlag = 0;
                CollisionData = 0x3F;
                EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("[8] Extra: Avoid obstacle (Bumpers)"); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                EUSCIA0_OutString("Robot will avoid obstacle (using Bumpers), press launchpad to end."); EUSCIA0_OutChar(CR); EUSCIA0_OutChar(LF);
                uint32_t case8RPM = 1500;
                while(1){
                    Motor_Forward(case8RPM, case8RPM);
                    if(CollisionFlag==1){
                        Motor_Backward(case8RPM, case8RPM);
                        Clock_Delay1ms(500);
                        Motor_Right(case8RPM, case8RPM);
                        Clock_Delay1ms(2000);
                        CollisionFlag=0;
                    }
                    if (LaunchPad_Input() != 0) break;
                }
                Motor_Stop();
                CollisionFlag = 0;
                CollisionData = 0x3F;
                menu = 1;
                cmd=0xDEAD;
                break;

            default:
                menu = 1;
                cmd=0xDEAD;
                break;
        }

        if (!menu) Clock_Delay1ms(3000);
        else menu=0;
    }
}
