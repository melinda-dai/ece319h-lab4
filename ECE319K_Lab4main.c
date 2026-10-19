/* ECE319K_Lab4main.c
 * Traffic light FSM
 * ECE319H students must use pointers for next state
 * ECE319K students can use indices or pointers for next state
 * Put your names here or look silly
  Melinda Dai
  */

#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
#include "../inc/Clock.h"
#include "../inc/UART.h"
#include "../inc/Timer.h"
#include "../inc/Dump.h"  // student's Lab 3
#include <stdio.h>
#include <string.h>
// put your EID in the next line
const char EID1[] = "MMD3728"; //  ;replace abc123 with your EID
// Hint implement Traffic_Out before creating the struct, make struct match your Traffic_Out
struct State {
  char Name[12];
  uint32_t Out;
  uint32_t Time;
  const struct State *Next[8];};

#define SOn   &FSM[0]
#define SWait   &FSM[1]
#define SOff   &FSM[2]
#define WalkOn   &FSM[3]
#define Warn1   &FSM[4]
#define WalkOff1   &FSM[5]
#define Warn2   &FSM[6]
#define WalkOff2   &FSM[7]
#define WalkOff   &FSM[8]
#define WestOn   &FSM[9]
#define WestWait   &FSM[10]
#define WestOff   &FSM[11]

typedef const struct State State_t;
State_t FSM[12] ={
  {"SOn", 0x04000101, 200, {SOn, SOn, SWait, SWait, SWait, SWait, SWait, SWait}},
  {"SWait", 0x04000102, 50, {SOff, SOff, SOff, SOff, SOff, SOff, SOff, SOff}},
  {"SOff", 0x04000104, 50, {WalkOn, SOn, WestOn, WestOn, WalkOn, WalkOn, WalkOn, WalkOn}},
  {"WalkOn", 0x0C400104, 200, {Warn1, Warn1, Warn1, Warn1, Warn1, Warn1, Warn1, Warn1}},
  {"Warn1", 0x04000104, 50, {WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1, WalkOff1}},
  {"WalkOff1", 0x00000104, 50, {Warn2, Warn2, Warn2, Warn2, Warn2, Warn2, Warn2, Warn2}},
  {"Warn2", 0x04000104, 50, {WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2, WalkOff2}},
  {"WalkOff2", 0x00000104, 50, {WalkOff, WalkOff, WalkOff, WalkOff, WalkOff, WalkOff, WalkOff, WalkOff}},
  {"WalkOff", 0x04000104, 50, {WestOff, WestOff, WestOn, WestOn, WestOn, WestOff, WestOn, WestOn}},
  {"WestOn", 0x04000044, 200, {WestWait, WestWait, WestWait, WestWait, WestWait, WestWait, WestWait, WestWait}},
  {"WestWait", 0x04000084, 50, {WestOff, WestOff, WestOff, WestOff, WestOff, WestOff, WestOff, WestOff}},
  {"WestOff", 0x04000104, 50, {SOn, SOn, WestOn, SOn, WalkOn, SOn, WalkOn, SOn}}};

State_t *Pt; // state pointer

// initialize all 6 LED outputs and 3 switch inputs
// assumes LaunchPad_Init resets and powers A and B
void Traffic_Init(void){ // assumes LaunchPad_Init resets and powers A and B
 // write this 
  IOMUX->SECCFG.PINCM[PB0INDEX] = 0x00000081; // South Green      81 MEANS software enabled
  IOMUX->SECCFG.PINCM[PB1INDEX] = 0x00000081; // South Yellow
  IOMUX->SECCFG.PINCM[PB2INDEX] = 0x00000081; // South Red

  IOMUX->SECCFG.PINCM[PB6INDEX] = 0x00000081; // West Green
  IOMUX->SECCFG.PINCM[PB7INDEX] = 0x00000081; // West Yellow
  IOMUX->SECCFG.PINCM[PB8INDEX] = 0x00000081; // West Red

  IOMUX->SECCFG.PINCM[PB15INDEX] = 0x00040081; // West sensor       software enabled, 
  IOMUX->SECCFG.PINCM[PB16INDEX] = 0x00040081; // South sensor
  IOMUX->SECCFG.PINCM[PB17INDEX] = 0x00040081; // Walk sensor/LED

  GPIOB->DOE31_0 |= 0x0C4001C7;
}
/* Activate LEDs
* Inputs: west is 3-bit value to three east/west LEDs
*         south is 3-bit value to three north/south LEDs
*         walk is 3-bit value to 3-color positive logic LED on PB22,PB26,PB27
* Output: none
* - west =1 sets west green
* - west =2 sets west yellow
* - west =4 sets west red
* - south =1 sets south green
* - south =2 sets south yellow
* - south =4 sets south red
* - walk=0 to turn off LED
* - walk bit 22 sets blue color
* - walk bit 26 sets red color
* - walk bit 27 sets green color
* Feel free to change this. But, if you change the way it works, change the test programs too
* Be friendly*/
void Traffic_Out(uint32_t west, uint32_t south, uint32_t walk){
  
  uint32_t westTemp  = (west & 0x07) << 6;      // west on PB6-8
  uint32_t southTemp = (south & 0x07);          // south on PB0-2

  uint32_t walkTemp  = (walk & 0x01) << 22;  // blue, PB22
  uint32_t walkTemp2 = (walk & 0x02) << 25;  // red, PB26
  uint32_t walkTemp3 = (walk & 0x04) << 25;  // green, PB27

  uint32_t maskSave = westTemp | southTemp | walkTemp | walkTemp2 | walkTemp3;

  GPIOB->DOUT31_0 &= 0xF3BFFE38;   // clear PB0-2, PB6-8, PB22, PB26, PB27 as intended
  GPIOB->DOUT31_0 |= maskSave;      // bits we want LED on for

}
/* Read sensors
 * Input: none
 * Output: sensor values
 * - bit 0 is west car sensor
 * - bit 1 is south car sensor
 * - bit 2 is walk people sensor
* Feel free to change this. But, if you change the way it works, change the test programs too
 */
uint32_t Traffic_In(void){
  uint32_t din = GPIOB->DIN31_0;

  uint32_t west  = (din >> 15) & 1;   // PB15 -> bit0  sensor input
  uint32_t south = (din >> 16) & 1;   // PB16 -> bit1
  uint32_t walk  = (din >> 17) & 1;   // PB17 -> bit2

  return (west) | (south << 1) | (walk << 2);

}
// use main1 to determine Lab4 assignment
void Lab4Grader(int mode);
void Grader_Init(void);
int main1(void){ // main1
  Clock_Init80MHz(0);
  LaunchPad_Init();
  Lab4Grader(0); // print assignment, no grading
  while(1){
  }
}
// use main2 to debug LED outputs
// at this point in ECE319K you need to be writing your own test functions
// modify this program so it tests your Traffic_Out  function
int main2(void){ // main2
  Clock_Init80MHz(0);
  LaunchPad_Init();
  Grader_Init(); // execute this line before your code
  LaunchPad_LED1off();
  Traffic_Init(); // your Lab 4 initialization
  SysTick_Init();
  Debug_Init();   // Lab 3 debugging
  Pt = SOn;       // start state is South On
  uint32_t in;
  if((GPIOB->DOE31_0 & 0x20)==0){
    UART_OutString("access to GPIOB->DOE31_0 should be friendly.\n\r");
  }
  UART_Init();
  UART_OutString("Lab 4, Spring 2026, Step 1. Debug LEDs\n\r");
  UART_OutString("EID1= "); UART_OutString((char*)EID1); UART_OutString("\n\r");
  if((GPIOB->DOUT31_0&0x20) == 0){
      UART_OutString("DOUT not friendly\n\r");
    }
  while(1){
    // write code to test your Traffic_Out
    #define traffic_mask 0x0C4001C7   // mask only pins we want
    // south traffic light (g, y, r)
    Traffic_Out(0,1,0);      // South green -> PB0
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);

    Traffic_Out(0,2,0);      // South yellow -> PB1
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);

    Traffic_Out(0,4,0);      // South red -> PB2
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);


    // west traffic light (g, y, r)
    Traffic_Out(1,0,0);      // West green -> PB6
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);

    Traffic_Out(2,0,0);      // West yellow -> PB7
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);

    Traffic_Out(4,0,0);      // West red -> PB8
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);


    // walk traffic light (walk/don't)
    Traffic_Out(0,0,7);      // Walk white -> PB22+PB26+PB27
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);

    Traffic_Out(0,0,2);      // Don't walk red -> PB26
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);

    // all off
    Traffic_Out(0,0,0);      // all off
    Debug_Dump(GPIOB->DOUT31_0 & traffic_mask);
    SysTick_Wait10ms(100);
  }
}
// use main3 to debug the three input switches
// at this point in ECE319K you need to be writing your own test functions
// modify this program so it tests your Traffic_In  function
int main3(void){ // main3
  uint32_t last=0,now;
  Clock_Init80MHz(0);
  LaunchPad_Init();
  Traffic_Init(); // your Lab 4 initialization
  Debug_Init();   // Lab 3 debugging
  UART_Init();
  __enable_irq(); // UART uses interrupts
  UART_OutString("Lab 4, Spring 2026, Step 2. Debug switches\n\r");
  UART_OutString("EID1= "); UART_OutString((char*)EID1); UART_OutString("\n\r");
  while(1){
    now = Traffic_In(); // Your Lab4 input
    if(now != last){ // change
      UART_OutString("Switch= 0x"); UART_OutUHex(now); UART_OutString("\n\r");
      Debug_Dump(now);
    }
    last = now;
    Clock_Delay(800000); // 10ms, to debounce switch
  }
}
// use main4 to debug using your dump
// proving your machine cycles through all states
int main4(void){// main4
uint32_t input;
  Clock_Init80MHz(0);
  LaunchPad_Init();
  LaunchPad_LED1off();
  Traffic_Init();   // your Lab 4 initialization
  // set initial state
  Debug_Init();   // Lab 3 debugging
  UART_Init();
  __enable_irq();   // UART uses interrupts
  UART_OutString("Lab 4, Spring 2026, Step 3. Debug FSM cycle\n\r");
  UART_OutString("EID1= "); UART_OutString((char*)EID1); UART_OutString("\n\r");
  // initialize your FSM
  SysTick_Init();   // Initialize SysTick for software waits
  Pt = SOn;
  input = 7;

  while(1){
      // 1) output depending on state using Traffic_Out
      uint32_t west  = (Pt->Out >> 6) & 7;
      uint32_t south = Pt->Out & 7;
      uint32_t walk  = ((Pt->Out >> 22) & 1) | (((Pt->Out >> 26) & 1) << 1) |
      (((Pt->Out >> 27) & 1) << 2);

      Traffic_Out(west, south, walk);   
      uint32_t state = Pt - FSM;
      uint32_t packed = (state << 24) | (west << 16) | (south << 8) | walk;

      Debug_Dump(packed);
      
      // call your Debug_Dump logging your state number and output
      // 2) wait depending on state
      SysTick_Wait10ms(Pt -> Time);
      // 3) hard code this so input always shows all switches pressed
      // 4) next depends on state and input
      Pt = Pt->Next[input];
  }
}
// use main5 to grade
int main(void){// main5
  Clock_Init80MHz(0);
  LaunchPad_Init();
  Grader_Init(); // execute this line before your code
  LaunchPad_LED1off();
  Traffic_Init(); // your Lab 4 initialization
  Pt = SOn;
// initialize your FSM
  SysTick_Init();   // Initialize SysTick for software waits
  // initialize your FSM
  Lab4Grader(1); // activate UART, grader and interrupts
  while(1){
      // 1) output depending on state using Traffic_Out
      // call your Debug_Dump logging your state number and output
       Traffic_Out((Pt->Out >> 6) & 0x07,   // west
       Pt->Out & 0x07,           // south
       ((Pt->Out >> 22) & 1) | (((Pt->Out >> 26) & 1) << 1) | (((Pt->Out >> 27) & 1) << 2));     // walk
       Debug_Dump(Pt->Out);
      // 2) wait depending on state
      SysTick_Wait10ms(Pt -> Time);
      // 3) input from switches
      uint32_t input = Traffic_In();
      // 4) next depends on state and input
      Pt = Pt->Next[input];
  }
}

