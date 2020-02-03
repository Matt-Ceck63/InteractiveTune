/* ------------------------------------------
       Simple Demo of LCD
             
   This test program uses two buttons    
     Button 1: cycles through a sequence of tests
     Button 2: shows some behaviour of interest  
     
   The test title is displayed on the top line.
   The behaviour is on the bottm line     
   A screen clear is done between each test.

     Test                     Behaviour on button press
     ---------------------------------------------
     1. Scroll text           Display text and scroll right then left
     2. Scroll cursor         Display text and scroll cursor
     3. Increment entry mode  Enter characters in increment mode
     4. Shift entry mode      Enter characters in shift entry mode
     5. Screen control        Cycle through screen off / on states
     
     Hardware
     --------
     Arduino LCD shield
     Button1 : PTD pin 6
     Button2 : PTD pin 7
     GND connection to pin on LCD shield
     
  -------------------------------------------- */

#include <MKL25Z4.h>
#include "../include/gpio_defs.h"
#include "../include/SysTick.h"
#include "../include/LCD.h"
#include "../include/adc_defs.h"
#include "../include/pit.h"
#include <stdbool.h>

/* ----------------------------------------
     Configure GPIO output for Audio
       1. Enable clock to GPIO port
       2. Enable GPIO port
       3. Set GPIO direction to output
       4. Ensure output low
 * ---------------------------------------- */

void configureGPIOoutput(void) {
    // Enable clock to port A
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    // Make pin GPIO
    PORTA->PCR[AUDIO_POS] &= ~PORT_PCR_MUX_MASK;          
    PORTA->PCR[AUDIO_POS] |= PORT_PCR_MUX(1);          
    
    // Set ports to outputs
    PTA->PDDR |= MASK(AUDIO_POS);

    // Turn off output
    PTA->PCOR = MASK(AUDIO_POS);
} ;

/* ----------------------------------------
     Toggle the Audio Output 
 * ---------------------------------------- */

void audioToggle(void) {
    PTA->PTOR = MASK(AUDIO_POS) ;
}

/*----------------------------------------------------------------------------
  GPIO Input Configuration

  Initialse a Port D pin as an input, with no interrupt
  Bit number given by BUTTON_POS
 *----------------------------------------------------------------------------*/ 
void configureGPIOinput(void) {
    SIM->SCGC5 |=  SIM_SCGC5_PORTD_MASK; /* enable clock for port D */

    /* Select GPIO and enable pull-up resistors and no interrupts */
    PORTD->PCR[BUTTON1_POS] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0);
    PORTD->PCR[BUTTON2_POS] |= PORT_PCR_MUX(1) | PORT_PCR_PS_MASK | PORT_PCR_PE_MASK | PORT_PCR_IRQC(0x0);
    
    /* Set port D switch bit to inputs */
    PTD->PDDR &= ~(MASK(BUTTON1_POS) | MASK(BUTTON2_POS));
}

float MeasureKeypad() {
    int i ;
    uint32_t res = 0 ;
		// take 16 voltage readings
		for (i = 0; i < 16; i++) { 
			// measure the voltage
				MeasureVoltage() ;
				res = res + sres ;
		}
		res = res >> 4 ; // take average
		res = (1000 * res * VREF) / ADCRANGE ;
		return res;
}
/*----------------------------------------------------------------------------
  task 1: Poll the input

*----------------------------------------------------------------------------*/

#define NO_PRESS (0)
#define LEFT_PRESS (1)
#define UP_PRESS (2)
#define RIGHT_PRESS (3)
#define DOWN_PRESS (4)

int state1 = NO_PRESS;
int signal = NO_PRESS;
int count1 = 5;
float Vin = 0;
int delay = 0;
int wait = 0;

void task1PollButtons(){
	
		if(delay > 0) {delay--;}
	
		if (delay == 0){
			
			Vin = MeasureKeypad();
			delay = 5;
			
			switch(state1){
				
				case NO_PRESS:
					if (2100 < Vin && Vin < 3300){
						state1 = LEFT_PRESS;
						signal = LEFT_PRESS;
					}
					if (800 < Vin && Vin < 2100){
						state1 = DOWN_PRESS;
						signal = DOWN_PRESS;
					}
					if (0 <= Vin && Vin < 235){
						state1 = RIGHT_PRESS;
						signal = RIGHT_PRESS;
					}
					if (235 < Vin && Vin < 800){
						state1 = UP_PRESS;
						signal = UP_PRESS;
					}
					break;
					
				case LEFT_PRESS:
					if (3300 < Vin  || Vin > 2000){
						state1 = NO_PRESS;
						signal = NO_PRESS;
					}
					break;
					
				case DOWN_PRESS:
					
					if (700 < Vin  || Vin > 2200){
						state1 = NO_PRESS;
						signal = NO_PRESS;
					}
					break;
					
				case RIGHT_PRESS:
					if (335 < Vin  || Vin > 0){
						state1 = NO_PRESS;
						signal = NO_PRESS;
					}
					break;
					
				case UP_PRESS:
					if (135 < Vin  || Vin > 900){
						state1 = NO_PRESS;
						signal = NO_PRESS;
					}	
					break;
				}
			}
}

/*----------------------------------------------------------------------------
  task 2: run tests 

*----------------------------------------------------------------------------*/

#define SELECT_NOTE (0)
#define SELECT_VALUE (1)
#define PLAY (2)

char note_prompt[] = "Note: ";
char length_prompt[] = "Length: ";

const char notes_names[12] = {'C', 'c', 'D', 'd', 'E', 'F', 'f', 'G', 'g', 'A', 'a', 'B'};
const uint32_t notes[12] = {20040, 18915, 17853, 16851, 15905, 15013, 14170, 13375, 12624, 11916, 11247, 10616}; 
const char length_names[] = "1248"; //in standard timing
const int lengths[4] = {25, 50, 100, 200}; //in fractions of 10ms

// Max of 12 notes can be chosen
// stores the index number corresponding to the note in notes array
int chosen_notes[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
// stores index number of lengths arrays
int chosen_lengths[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int state2 = SELECT_NOTE;

int note_selector = 0;
int length_selector = 0;
int curr_note = 0; // index used in arrays chosen_lengths and chosen_notes
int curr_playing = 0;
int time_count = 0;
         
void task2ControlLCD(void) {
	
	switch(state2){
	
		case SELECT_NOTE:
			
			if(signal == UP_PRESS){ // Display prev note
				signal = NO_PRESS;
				
				note_selector++;
				
				if (note_selector > 11) note_selector = 0;
				setLCDAddress(1,6); 
				writeLCDChar(notes_names[note_selector]);
				
				state2 = SELECT_NOTE;
			}
			
			if(signal == DOWN_PRESS){ // Display nextNote
				signal = NO_PRESS;
				
				note_selector--;
				if (note_selector < 0) note_selector = 11;
				setLCDAddress(1,6); 
				writeLCDChar(notes_names[note_selector]);
				
				state2 = SELECT_NOTE;
				
			}
			
			if(signal == RIGHT_PRESS){ // Prompt Length Value
				signal = NO_PRESS;
				
				setLCDAddress(1,0); //Select LCD bottom line pos 0
				writeLCDString(length_prompt);
				state2 = SELECT_VALUE;
				
			}
			
			if(signal == LEFT_PRESS){ // Play notes, if no notes are selected will not play
				signal = NO_PRESS;

				setTimer(0, chosen_notes[0]); // Start from first note
				time_count = chosen_lengths[0];
				startTimer(0);
				state2 = PLAY;

			}
			break;
		
		case SELECT_VALUE:
			
			if(signal == UP_PRESS){
				signal = NO_PRESS;
				
				length_selector++;
				if (length_selector > 3) length_selector = 0;
				
				setLCDAddress(1,7); 
				writeLCDChar(length_names[length_selector]);
				
				state2 = SELECT_VALUE;
				
			}
			
			if(signal == DOWN_PRESS){
				signal = NO_PRESS;
				
				length_selector--;
				
				if(length_selector < 0) {length_selector = 3;}
				
				setLCDAddress(1,7); 
				writeLCDChar(length_names[length_selector]);
				
				state2 = SELECT_VALUE;
				
			}
			
			if(signal == RIGHT_PRESS){ // Add note to display
				signal = NO_PRESS;
				
				chosen_notes[curr_note] = notes[note_selector];
				chosen_lengths[curr_note] = lengths[length_selector];
				
				setLCDAddress(0,curr_note);
				writeLCDChar(notes_names[note_selector]); // add note name to top row
				
				note_selector = 0;
				length_selector = 0;
				curr_note++;
				if(curr_note > 11){curr_note = 0;};
				
				setLCDAddress(1,0); //Select LCD bottom line pos 0
				writeLCDString(note_prompt);
				
				state2 = SELECT_NOTE;
			}
			
			break;
		
		case PLAY:
			
			if((time_count--) == 0){ //If the chosen time has passed
				curr_playing++; //Play the next note
				time_count = chosen_lengths[curr_playing]; //Choose the next time
				setTimer(0, chosen_notes[curr_playing]); //Choose the next note frequency
			}
			
			if(signal == LEFT_PRESS){
				signal = NO_PRESS;
				stopTimer(0);
				int chosen_notes[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
				int chosen_lengths[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};	
				state2 = SELECT_NOTE;	
			}
			break;
	}
}

/*----------------------------------------------------------------------------
  MAIN function

  Configure and then run tasks every 10ms

 *----------------------------------------------------------------------------*/

int main (void) {
		uint8_t calibrationFailed ; // zero expected
    configureGPIOinput() ;
		configureGPIOoutput() ;      // Initialise output    
    Init_SysTick(1000) ; 
    initLCD() ;
		Init_ADC() ;
    calibrationFailed = ADC_Cal(ADC0) ; // calibrate the ADC 
    while (calibrationFailed) ; // block progress if calibration failed
    Init_ADC() ;
    lcdClear(true) ;
    configurePIT(0) ;            // Configure PIT channel 0
    waitSysTickCounter(10) ; // initialise counter
	
		setLCDAddress(1,0); //Select LCD bottom line pos 0
		writeLCDString(note_prompt);
    
    while (1) {        
			  task1PollButtons();
				task2ControlLCD();
        waitSysTickCounter(10) ; // cycle every 10 ms
    }
}

