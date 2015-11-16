#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#pragma once

void init_lcd(void);
void stringout(char *);
void moveto(unsigned char);

void writecommand(unsigned char);
void writedata(unsigned char);