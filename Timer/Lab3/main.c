/*
 * FinalProject.c
 *
 * Created: 11/20/2019 2:23:22 PM
 * Author : Yaser Ibrahim, Andrew Schoen
 */

#define F_CPU 1000000 // MCU operating at 1 Mhz

/*lcd character locations for time components */
#define COLUMN_HOUR 0
#define COLUMN_MINUTE 3
#define COLUMN_SECOND 6
#define COLUMN_MILLISEC 9

#include <avr/io.h>
#include "lcd.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

typedef struct {
	uint8_t milliseconds; // Range of 0-99
	uint8_t seconds; // 0-59
	uint8_t minutes; // 0-59
	uint8_t hours; // 0-23
} time_struct;

typedef enum Program_States {
	PAUSED,
	PROGRAM,
	RUNNING
} Program_State;

typedef enum Count_States {
	UP,
	DOWN
} Count_State;

/* time instance (volatile so it can be changed by ISR)*/
volatile time_struct time;

/* initialize rx input array  */
unsigned char input;

/* initialize character array for displaying on LCD */
char string_buffer [20];

/* initialize states*/
Program_State program_state;
Count_State count_state;

/* configures UART RX Input */
void uart_RX_init(uint16_t baud)
{
	UCSR0B |= (1<<RXEN0);												// Turn on RX pin
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);						// UCSRC = 0000 00110 Asynch, No parity, 1 stop bit, 8 bit data, falling clock edge
	//int UBRR_calc = (F_CPU / (16*baud)) - 1 ; // Set max baud rate to 65K (DISABLED)
	UBRR_calc = 250;														// Set max baud rate to 250
	UBRR0L=UBRR_calc;														// Push lower half of value (half of 16 bit) to bottom register (8 bit)
	UBRR0H=UBRR_calc>>8;												// Push upper half of value (half of 16 bit) to top register (8 bit)
	DDRD &= ~(1<<PORTD0);												//set RX as input
}

/* reads from RX pin */
unsigned char USART_Receive( void ){

	/* wait to receive data unless switch button is pressed */
	while(!(UCSR0A & (1<<RXC0))){
		PORTD |= (1<<PORTD4);						// led on; ready to receive data
		if(!(PINC & (1<<PORTC1))){
			PORTD &= ~(1<<PORTD4);				// led off; not listening for data
			return '~';
		}
	}

	unsigned char received = UDR0;				// save the received data
	PORTD &= ~(1<<PORTD4);								// led off; not listening for data
	return received;
}

/* resets time displayed on LCD screen*/
void timer_reset(){
	time.milliseconds = 0;
	time.seconds = 0;
	time.minutes = 0;
	time.hours = 0;

	sprintf(string_buffer, "00:00:00:00"); 	// create string for output
	lcd_gotoxy(0,0);												// move to beginning of LCD (1st row & column)
	lcd_puts(string_buffer);							  // display on string on LCD

}

/* prints time component on the lcd in its proper column */
void lcd_print_time(int column, uint8_t time_unit){
	lcd_gotoxy(column, 0);

	if(time_unit < 10){
		sprintf(string_buffer, "0%u", time_unit); // i.e 1 will be displayed as 01
	}
	else{
		sprintf(string_buffer, "%u", time_unit); // i.e 11 will be displayed as 11
	}

	lcd_puts(string_buffer); 										// print to lcd screen
}

/* increment time routine: called every 100us and increments/decrements time */
ISR(TIMER1_COMPA_vect, ISR_BLOCK) {
		switch (current_count) {
			case UP:
			if(time.milliseconds == 99){
				time.milliseconds = 0;
				lcd_print_time(COLUMN_MILLISEC, time.milliseconds);
				if(time.seconds == 59){
					time.seconds = 0;
					lcd_print_time(COLUMN_SECOND, time.seconds);
					if(time.minutes == 59){
						time.minutes = 0;
						lcd_print_time(COLUMN_MINUTE, time.minutes);
						if(time.hours == 23){
							time.hours = 0;
							lcd_print_time(COLUMN_HOUR,time.hours);
							} else {
							time.hours++;
							lcd_print_time(COLUMN_HOUR,time.hours);
						}
						} else {
						time.minutes++;
						lcd_print_time(COLUMN_MINUTE, time.minutes);
					}
					} else{
					time.seconds++;
					lcd_print_time(COLUMN_SECOND, time.seconds);
				}
				} else {
				time.milliseconds++;
				lcd_print_time(COLUMN_MILLISEC, time.milliseconds);
			}
			break;

			case DOWN:
			if(time.milliseconds == 0){

				if(time.seconds == 0){

					if(time.minutes == 0){

						if (time.hours == 0) {
							lcd_print_time(COLUMN_HOUR,time.hours);
							/* don't do anything and display 00:00:00:00 */
						} else {
							time.minutes == 59;
							lcd_print_time(COLUMN_MINUTE, time.minutes);
							time.hours--;
							lcd_print_time(COLUMN_HOUR,time.hours);
						}
					} else {
						time.seconds = 59;
						lcd_print_time(COLUMN_SECOND, time.seconds);
						time.minutes--;
						lcd_print_time(COLUMN_MINUTE, time.minutes);
					}
				} else {
					time.milliseconds = 99;
					lcd_print_time(COLUMN_MILLISEC, time.milliseconds);
					time.seconds--;
					lcd_print_time(COLUMN_SECOND, time.seconds);
				}
			} else{
				time.milliseconds--;
				lcd_print_time(COLUMN_MILLISEC, time.milliseconds);
			}
		break;
	}
}

/* converts time values to represent seconds and minutes in base and hours in base 24 */
void convertTime(){
	if (time.seconds >= 60){
		time.seconds -= 60;
		time.minutes += 1;
	}
	if (time.minutes >= 60){
		time.minutes -= 60;
		time.hours +=1;
	}
	if (time.hours >= 24){
		time.hours = time.hours%24;
	}
	lcd_print_time(COLUMN_MILLISEC, time.milliseconds);
	lcd_print_time(COLUMN_SECOND, time.seconds);
	lcd_print_time(COLUMN_MINUTE, time.minutes);
	lcd_print_time(COLUMN_HOUR,time.hours);
}

// TODO: update this function later
/* allows user to select whether stop-watch increments or decrements time. Count up on 1, Down on 2 */
void countSelect(){
	unsigned char count_input;
	sprintf(string_buffer, "select 1 or 2 ");
	lcd_gotoxy(0,0);
	lcd_puts(string_buffer);
	while(1){
		count_input = USART_Receive();
		if (count_input == '1'){
			current_count = UP;
			return;
		}
		else if (count_input == '2'){
			current_count = DOWN;
			return;
		}
	}
}

/* change state of system between PROGRAM, RUNNING, or PAUSED states */
void change_state(State state){
	/* define state machine with 3 states: PAUSED, RUNNING, and PROGRAM (entered if # key is pressed) */
	switch (state) {
		case PAUSED:
			current_state = PAUSED;
			UCSR0B |= (1<<RXEN0); 							//enable RX receiving while paused
			TIMSK1 &= ~(1 << OCIE1A);						// disable interrupts to stop timer
			sprintf (string_buffer, "Paused");
			lcd_gotoxy(0,1);
			lcd_puts(string_buffer);						// print Paused on second row of LCD
			break;

		case RUNNING:
			current_state = RUNNING;
			UCSR0B &= ~(1<<RXEN0); 						//Disable RX receiving while running
			TIMSK1 &= ~(1 << OCIE1A);					// disable timer interrupts to print to display
			sprintf (string_buffer, "      ");
			lcd_gotoxy(0,1);
			lcd_puts(string_buffer);					// print  "      " and second row of LCD
			TIMSK1 |= (1 << OCIE1A);					// enable timer interrupts to start timer
			break;

		case PROGRAM:
			current_state = PROGRAM;
			PORTC &= ~(1 << PORTC1);					// disable internal pull up for switch button
			countSelect();										// request counting type from user (increments or decrements?)
			sprintf(string_buffer, "  :  :  :       ");
			lcd_gotoxy(0,0);
			lcd_puts(string_buffer);					// display "  :  :  :       " to indicate start time must be inputted

			/* record data being inputted */
			int input_time[] = {0,0,0,0,0,0,0,0};
			int receivedBytes = 0;
			while (current_state == PROGRAM){
				input = USART_Receive();

				if(input >= '0' && input <= '9'){
				/* display inputted start-time as user enters values and clear on overflow of LCD screen */

					if(receivedBytes > 8){							  	// screen overflow
						for(int i =0; i < 8; i++){
							input_time[i] = 0;								  // disregard inputted values
						}
						sprintf(string_buffer, "00:00:00:00");
						lcd_gotoxy(0,0);
						lcd_puts(string_buffer);							// display "00:00:00:00" on screen
						receivedBytes = 0;										// disregard bytes that where received
					}

					for(int i = receivedBytes; i>0; i--){
						input_time[i] = input_time[i-1];
					}
					input_time[0] = input - '0';
					sprintf(string_buffer, "%d%d:%d%d:%d%d:%d%d", input_time[7],input_time[6],input_time[5],input_time[4],input_time[3],input_time[2],input_time[1],input_time[0]);
					lcd_gotoxy(0,0);
					lcd_puts(string_buffer);   							// display programmed start-time
					receivedBytes++;

				} else if(input == '*' && receivedBytes != 0) {
					/* user has completed input, start counting */

					time.milliseconds = input_time[1]*10 + input_time[0];
					time.seconds = input_time[3]*10 + input_time[2];
					time.minutes = input_time[5]*10 + input_time[4];
					time.hours = input_time[7]*10 + input_time[6];
					convertTime();

					/* start timer*/
					PORTC |= (1 << PORTC1); // enable button
					change_state(RUNNING);
				}
			}
			break;

	}
}


int main(void)
{
	/*init LCD Display */
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();

	/* init timer counter for clock counting */
	TCCR1B = (1 << CS10) | (1 << WGM12);		// set prescaling 1
	OCR1A = 9999;														// set counter to be accurate to 100us (i.e OCR1A = 1/(1*1*(1/100us)) - 1 )
	sei();																	// must enable global interrupts, equivalent to SREG|= (1<<7)

	/* init switch button */
	DDRC &= ~(1 << PORTC1);						// configure switch button input
	PORTC |= (1 << PORTC1);						// enable internal pull up for switch button


	/* init led test to record serial status */
	DDRD |= (1<<PORTD4);						// set led as output
	PORTD &= ~(1<<PORTD4);						// set led low

	/* init UART comm with baud rate 1000 */
	uart_RX_init(1000);

	/* init first state as paused */
	timer_reset();
	change_state(PAUSED);



	while(1){

		/* go to next state if switch button pressed */
		if(!(PINC & (1<<PORTC1))){
			if(current_state == RUNNING){
				change_state(PAUSED);
			}
			else if(current_state == PAUSED){
				change_state(RUNNING);
			}

			while(!(PINC & (1<<PORTC1))); // fix debounce
		}

		if(current_state == PAUSED){
			input = USART_Receive();
			if (input == '#'){
				change_state(PROGRAM);
			}
		}
	}
}
