/*
 * GccApplication1.c
 *
 * Created: 11/28/2019 1:18:39 PM
 * Author : atschoen
 */

#include <avr/io.h>
#define F_CPU 1000000
#include <stdint.h>
#include <util/delay.h>

// define the keypad information
#define NUM_OF_ROWS 4
#define NUM_OF_COLS 3
//BD0-> Row 2
//PB4 -> Row 3
//PB3 -> Col3
//PB2 -> Row4
//PB1 -> Col1
//PB0 -> Row1
//PD6 -> Col 2
unsigned char row_ports[] = {'B','D','B','B'};
unsigned char col_ports[] = {'B','D','B'};

int rows[] = {PORTB0, PORTD0, PORTB4, PORTB2};
int cols[] = {PORTB1, PORTD6, PORTB3};

unsigned char keypad[] ={	'1', '2', '3',
							'4', '5', '6',
							'7', '8', '9',
							'*', '0', '#'};

/*
read input from the keypad
return: char that has been pressed of '~' error key
*/
unsigned char read_keypad(){
	int inputB;
	int inputD;
	int key_pressed = 0;
	int key_count = 0;
	int row_pressed;
	int col_pressed;

	// set col to z
	for(int C = 0; C<NUM_OF_COLS; C++){
		// set all columns to high Z HIGHz
		if(col_ports[C] == 'B'){
			DDRB &= ~(1<<cols[C]); // set input
			PORTB &= ~(1<<cols[C]); // set high Z
		}
		else if(col_ports[C] == 'D'){
			DDRD &= ~(1<<cols[C]); // set input
			PORTD &= ~(1<<cols[C]); // set high Z
		}
	}

	for(int R = 0; R<NUM_OF_ROWS; R++){
		//set all rows as input with pull up
		if(row_ports[R] == 'B'){
			DDRB &= ~(1<<rows[R]); // set input
			PORTB |= (1<<rows[R]); // set pull up
		}
		else if(row_ports[R] == 'D'){
			DDRD &= ~(1<<rows[R]); // set input
			PORTD |= (1<<rows[R]); // set pull up
		}
	}

	// loop through all the columns, going through each in low state
	for(int c = 0; c<NUM_OF_COLS; c++){
		// Set column as output low
		if(col_ports[c] == 'B'){
			DDRB |= (1<<cols[c]); // set output
			PORTB &= ~(1<<cols[c]); // set low
		}
		else if(col_ports[c] == 'D'){
			DDRD |= (1<<cols[c]); // set output
			PORTD &= ~(1<<cols[c]); // set low
		}

		inputB = PINB;
		inputD = PIND;
		// loop through rows checking input
		for(int r = 0; r<NUM_OF_ROWS; r++){
			if(row_ports[r] == 'B'){
				key_pressed = ( inputB & (1<<rows[r])); // if low then there is a key pressed
			}
			else if(row_ports[r] == 'D'){
				key_pressed = ( inputD & (1<<rows[r])); // if low then key is pressed
			}

			if(key_pressed == 0){
				row_pressed = r;
				col_pressed = c;
				key_count++;
			}
		}
		// reset column to Z state
		if(col_ports[c] == 'B'){
			DDRB &= ~(1<<cols[c]); // set input
			PORTB &= ~(1<<cols[c]); // set high Z
		}
		else if(col_ports[c] == 'D'){
			DDRD &= ~(1<<cols[c]); // set input
			PORTD &= ~(1<<cols[c]); // set high Z
		}
	}

	// if only 1 key pressed return the pressed key char
	if(key_count == 1){
		return keypad[row_pressed*NUM_OF_COLS + col_pressed];
	}

	// if no keys pressed or multiple keys pressed return the error char
	else{
		return '~';
	}
}

/*
	Uart serial setup for transmitting port
	input: desired value for baud rate
*/
void uart_TX_init(uint16_t baud)
{
	UCSRA |= (1<<TXC); // clear flag
	UCSRB |= (1<<TXEN); //Turn on TX pin to be used as TX
	UCSRC = (1<<UCSZ1)|(1<<UCSZ0); //0000 00110 Asynch,No parity, 1 stop bit, 8 bit data, falling clock edge

	int UBRR_calc = (F_CPU / (16*baud)) - 1 ;   //baud rate calculation
	UBRR_calc = 250;
	UBRRL=UBRR_calc;
	UBRRH=UBRR_calc>>8; 		   //set baud rate in the registers

	DDRD|= (1<<PORTD1);	    //set TX pin as output
}

/*
	Send data over TX serial port
	input: data that you want to send
*/
void uart_sendB(uint8_t data)
{
	while(!(UCSRA&((1<<UDRE)))) ;  //check if UDRE is empty, ready to receive new data
	UDR=data; //deliver the data to be transmitted
	while(!(UCSRA&((1<<TXC))));	//check if TX has completed, TXC will be set when data has been sent
	UCSRA|=(1<<TXC);  // we are not using interrupt to clear the TXC, need to write a �1� to clear it
}


int main(void)
{
	uart_TX_init(1000); // set up serial
    while (1)
    {
		unsigned char j = read_keypad(); // read keypad input
		if(j != '~'){
			uart_sendB(j);
			_delay_ms(500); // software debounce of keypad
		}
    }
}
