/********************************************
*
*  Name: Danny Pan
*  Section:	3:30 Wed lab. 11-12:30 T/TH 
*  Assignment: Thermostat
*
********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include "lcd.h"
#include "ds1631.h"

void stateupdate();
void readbits();
void enablePINTERRUPTS();
void screenprintTop();
void screenprintBot();
void LEDcheck();

void calctemp(unsigned char realTemp[]);

void serial_init(unsigned int MYUBRR);
char read_char();
void tx_char(char ch);
void transmit_char(short trueTemp);

ISR(PCINT0_vect);
ISR(PCINT1_vect);

volatile char heatState = 0; // 0 = cold, 1 = hot. 
// Variables for state changes in the rotary encoder.
char oldState[2] = {0, 0};
char newState[2] = {0, 0};
//LCD variables
char TopScreen[16];
char BotScreen[16];

//Temperature variables used on the LCD
volatile short trueTemp = 78;
short oldTrueTemp = 0;

volatile short rmtTemp = 102;
short oldrmtTemp = 0;

volatile short highTemp = 82;
short oldHighTemp = 0;

volatile short lowTemp = 72;
short oldLowTemp = 0;

//Serial read and transmit variables
volatile char read[4];
volatile char readCount = 0;
volatile char readState = 0;

//Temp Sensor variable
unsigned char readTemp[2];

//These were commented out because the syntax for define didn't compile with my
//compiler. I manually computed the MYUBRR value and inserted it in.
//#define FOSC 16000000
//#define BAUD 9600
//#define MYUBRR FOSC/16/BAUD-1

int main(void)
{
	//Set LED ports for out put
	DDRD |= ( (1<<PD2) | (1<<PD3) );
	//Enable tri-state gate port for output and set it to initial 0
	DDRC |= (1<<PC3);
	PORTC &= ~(1<<PC3);
	
	//Enable pull up resistors for the buttons and rotary encoder
	PORTB |= ( ( 1<<PB3 ) | (1<<PB4) );
	PORTC |=  ( ( 1 << PC1 ) | (1 << PC2) );

	//Void function to enable general interrupts, the LCD screen, serial ports, and the temperature sensor
	init_lcd();
    enablePINTERRUPTS();
	serial_init(104);
	ds1631_init();         //  Initialize  the  DS1631
	ds1631_conv();         // Set  the  DS1631  to do  conversions
	
	//Clear LCD screen for output	
	writecommand(0x01);
		
	while (1)
	{
		// Call function for temperature sensor to read the temperature
        ds1631_temp(readTemp);
        // Calculate Temperature to calculate temperature and store it in trueTemp
        calctemp(readTemp);
        // function to check temperature vs high and low values to turn on the proper LED
    	LEDcheck();
    	
    	// readState turns to 1 if 4 bits are received and conversion occurs and stored in rmtTemp
    	if (readState == 1)
    	{
			rmtTemp = ((read[1] - 0x30)*100) + ((read[2] - 0x30)*10) + (read[3] - 0x30);
			
			if (read[0] == '-')
			{
				rmtTemp *= -1;
			}
			
			readState = 0;
		}
		
		//if temperature is updated, the new temp is transmitted.
    	if (trueTemp != oldTrueTemp)
    	{		
    		transmit_char(trueTemp);
    	}
        
        // Both statements check to see if anything changed and the values are updated if changes occur.
		if ( (oldTrueTemp != trueTemp) || (oldrmtTemp != rmtTemp) )
		{
			screenprintTop();
		}
		else if ( (oldHighTemp != highTemp) || (oldLowTemp != lowTemp) )
		{
			screenprintBot();
		}
    }
    
    return 0;
}

//Rotary encoder ISR function.
ISR(PCINT1_vect){
	//Function to read bits.
	readbits();
	{		
		if (heatState == 0) //Cold button is pushed
		{
			//Decrement low temp when equal to high so it doesn't pass high temp boundary.
			if ( highTemp == lowTemp)
			{
				lowTemp--;
			}
			
			if ( (oldState[1] == 0) && (oldState[0] == 0) ) /* 00 State */
			{
				if( (newState[1] == 0) && (newState[0] == 1) ) /* Clockwise (Positive) condition */
				{
					lowTemp += 1;
				}
				else if( (newState[1] == 1) && (newState[0] == 0) ) /* Negative Conditions */
				{
					lowTemp -= 1;
				}			
				stateupdate();
			}
	
			else if ( (oldState[1] == 0) && (oldState[0] == 1) ) /* 01 State */
			{
				if( (newState[1] == 1) && (newState[0] == 1) ) /* Clockwise (Positive) condition */
				{
					lowTemp += 1;
				}
				else if( (newState[1] == 0) && (newState[0] == 0) )  /* Negative Conditions */
				{
					lowTemp -= 1;
				}
				stateupdate();
			}	
	
			else if ( (oldState[1] == 1) && (oldState[0] == 1) ) /* 11 State */
			{
				if( (newState[1] == 1) && (newState[0] == 0) ) /* Clockwise (Positive) condition */
				{
					lowTemp += 1;
				}
				else if( (newState[1] == 0) && (newState[0] == 1) ) /* Negative Conditions */
				{
					lowTemp -= 1;
				}			
				stateupdate();
			}
	
			else if ( (oldState[1] == 1) && (oldState[0] == 0) ) /* 10 State */
			{
				if( (newState[1] == 0) && (newState[0] == 0) ) /* Clockwise (Positive) condition */
				{
					lowTemp += 1;
				}
				else if( (newState[1] == 1) && (newState[0] == 1) ) /* Negative Conditions */
				{
					lowTemp -= 1;
				}			
				stateupdate();
			}
		}
	
		else if (heatState == 1) //when high temp button is pressed
		{
			//increment highTemp so it doesn't drop below lowTemp when equal.
			if ( highTemp == lowTemp)
			{
				highTemp++;
			}
			
			if ( (oldState[1] == 0) && (oldState[0] == 0) ) /* 00 State */
			{
				if( (newState[1] == 0) && (newState[0] == 1) ) /* Clockwise (Positive) condition */
				{
					highTemp += 1;
				}
				else if( (newState[1] == 1) && (newState[0] == 0) ) /* Negative Conditions */
				{
					highTemp -= 1;
				}			
				stateupdate();
			}
	
			else if ( (oldState[1] == 0) && (oldState[0] == 1) ) /* 01 State */
			{
				if( (newState[1] == 1) && (newState[0] == 1) ) /* Clockwise (Positive) condition */
				{
					highTemp += 1;
				}
				else if( (newState[1] == 0) && (newState[0] == 0) )  /* Negative Conditions */
				{
					highTemp -= 1;
				}
				stateupdate();
			}	
	
			else if ( (oldState[1] == 1) && (oldState[0] == 1) ) /* 11 State */
			{
				if( (newState[1] == 1) && (newState[0] == 0) ) /* Clockwise (Positive) condition */
				{
					highTemp += 1;
				}
				else if( (newState[1] == 0) && (newState[0] == 1) ) /* Negative Conditions */
				{
					highTemp -= 1;
				}			
				stateupdate();
			}
	
			else if ( (oldState[1] == 1) && (oldState[0] == 0) ) /* 10 State */
			{
				if( (newState[1] == 0) && (newState[0] == 0) ) /* Clockwise (Positive) condition */
				{
					highTemp += 1;
				}
				else if( (newState[1] == 1) && (newState[0] == 1) ) /* Negative Conditions */
				{
					highTemp -= 1;
				}			
				stateupdate();
			}
		}
	}
}

//ISR function for buttons.
ISR(PCINT0_vect){

	//ISR check to see which button is pushed the that heatState is changed accordingly
	//and also used with the rotary encoder to change high and low temp values.
	//If and else if statement so we don't have a both buttons pressed issue.
	if ( ( PINB & (1<<PB3) ) == 0 ) // Hot Button
	{
		heatState = 1;
	}
	else if ( ( PINB & (1<<PB4) ) == 0) //Cold Button
	{
		heatState = 0;
	}
}

//ISR function to chars from the serial interface when data is received.
ISR(USART_RX_vect)
{
	//Temporary variable used to received the specified character.
	char tempRead = read_char();
	//Character is then stored into a read array where readCount tracks the string count.
	read[readCount] = tempRead;
	
	//Count increment to move to the next character
	readCount++;
	
	//Conversion occurs when all 4 bytes are received from the transmitted unit.
	if (readCount == 4)
	{
		//Readcount is reset and state is turned to 1 signalling conversion.
		readCount = 0;
		readState = 1;
	}
}

// Turns on the necessary ports and enables sei();
void enablePINTERRUPTS()
{
	//Individual port interrupt are turned on and also for the specific bits.
	//Used for the rotary encoder and the buttons
	PCICR |= (( 1 << PCIE1 ) | ( 1 <<PCIE0) );
	PCMSK1 |= ( (1<<PCINT10) | (1<<PCINT9) );
	PCMSK0 |= ( (1<<PCINT3) | (1<<PCINT4) );
	//SEI is called to enable interrupts
	sei();
}

//Reads the PINC bits and shifts them to the 0th bit.
void readbits()
{
	newState[0] = PINC & (1 << PC1);
	newState[0] = (newState[0] >> 1);
	newState[1] = PINC & (1 << PC2);
	newState[1] = (newState[1] >> 2);
}

//Update old state variables with new state variable bits.
void stateupdate()
{
	oldState[1] = newState[1];
	oldState[0] = newState[0];
}

//Function to print the top of the screen.
void screenprintTop()
{	
	//Top is cleared with spaces so whole screen doesn't crash.
	moveto(0x80);
	snprintf(TopScreen, 16, "                  ");
	stringout(TopScreen);
	
	//New values are printed.
	moveto(0x80);
	snprintf(TopScreen, 16, "Temp:%d Rmt:%d" , trueTemp, rmtTemp);
	stringout(TopScreen);
	
	//Old values are updated
	oldTrueTemp = trueTemp;
	oldrmtTemp = rmtTemp;
}

void screenprintBot()
{
	//Clears bottom screen with spaces to minimize flicker
	moveto(0xc0);
	snprintf(BotScreen, 16, "                  ");
	stringout(BotScreen);
	
	//Print new data
	moveto(0xc0);
	snprintf(BotScreen, 16, "High:%d Low:%d" , highTemp, lowTemp);
	stringout(BotScreen);
	
	//Updates the temperature values.
	oldHighTemp = highTemp;
	oldLowTemp = lowTemp;
}

void LEDcheck()
{
	//LEDS's enable if the temp surpasses high and drops below temp. LEDs are turned off if neither happen.
	if (trueTemp > highTemp)
	{
		PORTD |= (1<<PD2);
		PORTD &= ~(1<<PD3);
	}
	else if (trueTemp < lowTemp )
	{
		PORTD |= (1<<PD3);
		PORTD &= ~(1<<PD2);
	}
	else
	{
	    PORTD &= ~(1<<PD2);
		PORTD &= ~(1<<PD3);
	}
}

//Calculate temperature
void calctemp(unsigned char realTemp[])
{
	//received temperature value is stored using temp value. Multiply by 10 in order to eliminate
	//the decimal point.
	int temp = realTemp[0] * 10;
	
	//Add the half value is it exists in the second part of the register.
	if (realTemp[1] == 0x80)
	{
		temp += 5;
	}
	
	//Convert from fahrenheit to celsius using 9/5 C.
	temp *= 9;
	temp /= 5;
	
	//Round up or down if the decimal bit is greater or equal to 5
	if (temp % 10 >= 5)
	{
		//Use modulo function and temperature to add difference in numbers to round.
		temp = temp + (10 - (temp % 10));
	}
	else
	{
		//Subtract remainder to round to nearest whole number.
		temp = temp - (temp % 10);
	}
	
	//Divide answer by 10 to compensate for the multiply at the beginning and add 32 to convert.
	//Store it in trueTemp in.
	trueTemp = (temp / 10) + 32;
}

void serial_init(unsigned int MYUBRR)
{
	//Enable interrupt bit.
	UCSR0B |= (1<<RXCIE0);
	//Enable transmit and receive registers.
	UCSR0B |= ( (1<<RXEN0) | (1<<TXEN0) );
	//set the BAUD rate, 104 after math in the situation
	UBRR0 = MYUBRR;
	//Enables 8 transmit bits
	UCSR0C= ((1<<UCSZ01) | (1<<UCSZ00));
}

//Read character that reads that transmitted data.
char read_char()
{	
	//Wait until transmission finishes and returns the character
	while ( !(UCSR0A & (1<<RXC0)) ) {}
	return UDR0;
}

//Transmit character
void transmit_char(short trueTemp)
{
	/Creature temporary 4 character array to store values.
	char transmit[4];
	
	//Check to see negative or positive temperature in order to add positive or negative signs
	if (trueTemp < 0)
	{
		transmit[0] = '-';
		tx_char(transmit[0]);
	}
	else
	{
		transmit[0] = '+';
		tx_char(transmit[0]);
	}
		//First transmit bit is the 100's place, so divide by 100 and integer arithmetic automatically rounds down.
		transmit[1] = (trueTemp/100);
		//Transmit with +48 offset to account for ASCII
		tx_char(transmit[1] + 48);
		//Subtract the hundreds place temp with previous variable and divide by 10 to get 10's place digit.
		transmit[2] = ( (trueTemp - (transmit[1]*100) ) / 10);
		//Transmit with +48 offset to account for ASCII
		tx_char(transmit[2] + 48);
		//Subtract 100's and 10's place digits to obtain 1's place digit.
		transmit[3] = ( trueTemp - (transmit[1]*100) - (transmit[2]*10) );
		//Transmit with +48 offset to account for ASCII
		tx_char(transmit[3] + 48);
}

//Transmit character function.
void tx_char(char ch)
{
	//Wait until transmission finishes to place new value into the register.
	while ( (UCSR0A & (1<<UDRE0)) == 0 ) {}
	UDR0 = ch;
}