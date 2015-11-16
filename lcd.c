#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include "lcd.h"
#pragma once

/*
  init_lcd - Do various things to initialize the LCD display
*/
void init_lcd()
{
	//Enable ports for output
	DDRD |= (0xF0);
	DDRB |= (0x03);
	
    _delay_ms(15);              // Delay at least 15ms

	//Use these statements ot initialize the lcd display.

    // Use writenibble to send 0011
    writenibble(0x30);
    _delay_ms(4);               // Delay at least 4msec

    // Use writenibble to send 0011
    writenibble(0x30);
    _delay_us(100);             // Delay at least 100usec

    // Use writenibble to send 0011
    writenibble(0x30);

    // Use writenibble to send 0010    // Function Set: 4-bit interface
    writenibble(0x20);
    _delay_ms(2);
    
    writecommand(0x28);         // Function Set: 4-bit interface, 2 lines
    _delay_ms(2);

    writecommand(0x0f);         // Display and cursor on
    _delay_ms(2);

}

/*
  stringout - Print the contents of the character string "str"
  at the current cursor position.
*/
void stringout(char *str)
{
	int i;
	//Write each character to the screen using an if statement that loops through the string.
	for(i = 0; i < strlen(str); i++)
	{
		//Write the data with write data function and the character at the array value.
		writedata(str[i]);
	}
}

/*
  moveto - Move the cursor to the postion "pos"
*/
void moveto(unsigned char pos)
{
	//Use write command to write move cursor statement using the offset.
	writecommand(pos);	
}

/*
  writecommand - Output a byte to the LCD display instruction register.
*/
void writecommand(unsigned char x)
{
	//Set Port B to 0 for command write.
	PORTB &= ~(0x01);
	
	//Write nibbles in and left shift bottom nibbles 4 to allow write nibble to import it
	writenibble(x);
	writenibble(x << 4);
	
	_delay_ms(2);
};

/*
  writedata - Output a byte to the LCD display data register
*/
void writedata(unsigned char x)
{
	//Set Port B to 1 for data write
	PORTB |= 0x01;
	
	//Write nibbles in and left shift bottom nibbles 4 to allow write nibble to import it
	writenibble(x);
	writenibble(x << 4);
	
	_delay_ms(2);
	
}

/*
  writenibble - Output four bits from "x" to the display
*/
void writenibble(unsigned char x)
{
	//Place the top nibble in D ports
	PORTD &= ~(0xF0);
	PORTD |= (x & 0xF0);
	
	//Flips the 2nd bit in port B to enable data transmission.
	PORTB |= 0x02;
	//Delay to give the required buffer time for clock cycle to see it and execute.
	_delay_us(1);
	//Turn off the bit to signify transfer is over.
	PORTB &= ~(0x02);
}