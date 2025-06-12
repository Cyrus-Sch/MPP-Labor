#include "inc/tm4c1294ncpdt.h"

#include <stdio.h>

#include <string.h>

#include <stdint.h>

#include <math.h>

#define MATCHVALUE 800 // timer match value
#define STARTVALUE 1600 // timer start value = PWM Period 1600
#define WAITLOOPLENGTH 100 //Waiting time for changes of PWM dutycyclcle
#define ADC_VREF 3300.0 //voltage on V_REF+pin in mV
#define V_LSB (ADC_VREF / 4096) //V_LSB voltage in mV
#define V_COEFF ((4.70+9.137)/9.137*V_LSB) //Adjust voltage for HAW-Board
/**

* main.c

*/

 

//wait for some ms...

void wait(int ms){

    TIMER0_CTL_R |= 0x00000000;

    TIMER0_TAILR_R = ms*16000; // empirical defined from 1

    TIMER0_ICR_R |= 1;

    TIMER0_CTL_R |= 0x00000001;

    while ((TIMER0_RIS_R & 0x1) ==0x0);

}

 

void clockWait(int clock){

    TIMER0_CTL_R |= 0x00000000;

    TIMER0_TAILR_R = clock;

    TIMER0_ICR_R |= 1;

    TIMER0_CTL_R |= 0x00000001;

    while ((TIMER0_RIS_R & 0x1) ==0x0);

}

int waegeVerfahren(int start, int end){

    int mid = (end - start) / 2;

 

    // Base case: if the difference is too small, return start (or end, as they are the same at this point)

    if (mid <= 0) {

        return start;

    }

 

    // Output the current value of mid to GPIO_PORTK for debugging or other purposes

    GPIO_PORTK_DATA_R = start + mid;

    wait(1);

    int hit = GPIO_PORTD_AHB_DATA_R;

    if (hit == 0) {

        return waegeVerfahren(start, start + mid);

    }

    else if (hit == 1) {

        return waegeVerfahren((start + mid), end);

    }

    return start;

}

int main(){
	// general variables 
	int i = 0;
	int inc_dec = 1;
	float ulsb = 5.0/256.0;
	int w = 0;
	//
	// port enable
	//
	SYSCTL_RCGCGPIO_R |= (1<<3) | (1<<4) | (1<<11); 
	SYSCTL_RCGCADC_R |= 0x1; //Clock ADC0 enable
	GPIO_PORTM_DEN_R= 0x03;//Digital enable
	GPIO_PORTM_DIR_R= 0x001;// set direction Output and Input
	GPIO_PORTD_AHB_DEN_R|=0x20; // enable digital function I/O pin PD5
	GPIO_PORTD_AHB_DIR_R|=0x20; // define output direction I/O for pinPD5
	GPIO_PORTD_AHB_AFSEL_R|= 0x20; // PINPD5 is set to alternative function
	GPIO_PORTD_AHB_PCTL_R |=0x00300000; // use the port pin by the control

	// PWM
	SYSCTL_RCGCTIMER_R= 0x00000008; w++; // clock enable timer3
	
	 TIMER3_CTL_R &= ~0x0000100; // stop timer3B
	 TIMER3_CFG_R = 0x00000004; // set timer3 in 16 bit mode
	 TIMER3_TBMR_R = 0x0000000A; // timer3B in PWM MODE Flags are TBAMS=1, TBCMR=0, TBMR=2 (2 Bitfield)
	 TIMER3_CTL_R &= ~0x004000; // PWM Output not inverted
	 TIMER3_TBILR_R = STARTVALUE; // set start value = PWM period 1/10000s at 16Mhz => 1600 clocks
	 TIMER3_TBMATCHR_R = MATCHVALUE; // set duty cycle of 50% at beginning
	 TIMER3_CTL_R |= 0x000100; // start timer3B
	 //ADC
	// 
	//
	 // Magic code for start the ADC clocking at 16MHz
	 // => see datasheet, 15.3.2.7 ADC module clocking, p1061-1062
	 SYSCTL_PLLFREQ0_R |= SYSCTL_PLLFREQ0_PLLPWR; // power on the PLL
	 while(!(SYSCTL_PLLSTAT_R & SYSCTL_PLLSTAT_LOCK)); // wait till PLL has locked
	 ADC0_CC_R = 0x01 ; wt++; // select PIOSC (internal RCOsc) as ADC analog clock
	 SYSCTL_PLLFREQ0_R &= ~SYSCTL_PLLFREQ0_PLLPWR; // power off the PLL (s. above)
	 // end of magic code ...
	 // Prepare Port Pin PE0 as AIN3
	 GPIO_PORTE_AHB_AFSEL_R |=0x01; // PE0 Alternative Pin Function enable
	 GPIO_PORTE_AHB_AMSEL_R |=0x01; // PE0 Analog Pin Function enable
	 GPIO_PORTE_AHB_DEN_R &=~0x01; // PE0 Digital Pin Function DISABLE
	 GPIO_PORTE_AHB_DIR_R &=~0x01; // Allow Input PE0
	 ADC0_ACTSS_R &=~0x0F; // Disable all 4 sequencers of ADC 0
	 ADC0_SSMUX0_R |= 0x03; // Sequencer SS0 channel AIN3 without multiplexing
	 ADC0_SSCTL0_R |= 0x02; // Set "END=0" sequence length 1 (one sample sequence)
	 ADC0_CTL_R &=~0x01; // Use Vdda 3.3V as V_REF .. if Bit0 is clear
	 ADC0_ACTSS_R |= 0x01; // Enable sequencer SS0 with ADC 0
	//
	//
	
	while(1){

	wait(1); // wait 1ms so we dont drown in terminal statements
	ADC0_PSSI_R|=0x01; // Start ADC0
	while(ADC0_SSFSTAT0_R & 0x000000100); // wait for FIFO (inverted) Flag "EMPTY = False"
	ADCoutput=(unsigned long) ADC0_SSFIFO0_R; // Take avalue from FIFO output
	// Three variants output: hexadecimal, decimal, and mV
	// calculated output in mV with respect to voltage divider 5:3 in the Lab, see macro
	printf("0x%3x=%4d (dec) ==> %04d [mV] \n", ADCoutput, ADCoutput, (int) ( ADCoutput * V_COEFF + 0.5));

	clockWait(100); // wait for change of duty cycle
	inc_dec = (ADCoutput - 2000)/1000; // should range -2 to 2
	if (TIMER3_TBMATCHR_R ==STARTVALUE) inc_dec= 0; // maximal duty cycle reached so we count down
	if (TIMER3_TBMATCHR_R ==0) inc_dec=0; // min duty cycle reached so we stop. 
	TIMER3_TBMATCHR_R =TIMER3_TBMATCHR_R + dv; // adjusting the duty cycle
	
	if (GPIO_PORTM_DATA_R == 0x01 && !(TIMER3_TBMATCHR_R == STARTVALUE)){
		TIMER3_TBMATCHR_R = STARTVALUE;
		}
	else if(GPIO_PORTM_DATA_R == 0x01 && (TIMER3_TBMATCHR_R == STARTVALUE){
		TIMER3_TBMATCHR_R = 0;
		}

	// Ablauf
	
	// 1. Die Lampe leuchten lassen wie eingestellt
	// 2. nach input mit ADC und Port M schauen
		// 3. Anpassen des Match
	}


}
