//===========================================================
// Test program UART6 TX 8/N/1 @ 115200 bit via Port PP1
// LTL 17.5.2020 / mod. V0.2 - V0.4 RMS 1.6.2023
//==========================================================
#include "inc/tm4c1294ncpdt.h" // Header of the controller type
#include <stdint.h> // Header w. types for the register ..
#define IDLETIME 1000 // waiting time between transmissions
int wt = 0; // auxillary variable
void config_port(void){
// initialize Port P
SYSCTL_RCGCGPIO_R |= 0x02000; // switch on clock for Port P
wt++; // delay for stable clock
// initialize Port P
GPIO_PORTP_DEN_R |= 0x2; // enable digital pin function for PP1
GPIO_PORTP_DIR_R |= 0x2; // set PP1 to output
GPIO_PORTP_AFSEL_R |= 0x2; // switch to alternate pin function PP1
GPIO_PORTP_PCTL_R |= 0x10; // select alternate pin function PP1->U6Tx
}
void config_uart(void){
// initialize UART6
SYSCTL_RCGCUART_R |= 0x40; // switch on clock for UART6
wt++; // delay for stable clock
UART6_CTL_R &= ~0x01; // disable UART6 for config
// initialize bitrate of 115200 bit per second
UART6_IBRD_R = 8; // set DIVINT of BRD floor(16 MHz/16*115200 bps)
UART6_FBRD_R = 44; // set DIVFRAC of BRD remaining fraction divider
UART6_LCRH_R = 0x00000060; // serial format 8N1
UART6_CTL_R |= 0x0101; // UART transmit on and UART enable
}
void idle() { // simple wait for idle state
int i;
for (i=IDLETIME;i>0;i--); // count down loop for waiting
}
void main(void){
config_port(); // configuration of Port P
config_uart(); // configuration of UART6
while(1){
while((UART6_FR_R & 0x20) !=0); // till transmit FIFO not full
UART6_DR_R = 'H'; // send the character 'H'
idle(); // idle time
}
}
