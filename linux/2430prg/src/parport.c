/*
 * parport.c - Parallel port utility functions
 *
 * Copyright Otávio Ribeiro @ 2007
 * otavio@otavio.eng.br
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>
#include <linux/parport.h>
#include <time.h>

#include "driver.h"
#include "parport.h"

static struct _prg_driver * parport_driver = NULL;
static struct timespec interval;

unsigned int frequency = 0;

void parport_register(struct _prg_driver drv)
{
	parport_driver = &drv;
	
	interval.tv_sec=0;
	interval.tv_nsec=10;
}

int parport_open(char* param)
{
	if(parport_driver == NULL)
		return 0;
		
	return parport_driver->open(param);
}

int parport_init()
{
	if(parport_driver == NULL)
		return 0;
		
	return parport_driver->init();
}

void parport_close()
{
	if(parport_driver != NULL)
		parport_driver->close();
}

int parport_detect_cable(cc2430_cable_status* cable_status)
{
	unsigned char status = 0;
	unsigned char byte = CC2430_PROG;
	int ret = 0;

	if(cable_status == NULL || parport_driver == NULL)
		return ret;
	
	if(parport_driver->write(PRG_REGISTER_DATA,&byte)){	
		usleep(5);
		
		if(parport_driver->read(PRG_REGISTER_STATUS,&status)){
			if( (status & CC2430_CABLE_V1_MASK) == CC2430_CABLE_V1){
				cable_status->version = 1;
			} else {
				cable_status->version = 0;
			}	
			ret = 1;
		}
	}

	return ret;
}

int parport_reset()
{
	unsigned char byte = 0;

	if(!parport_driver->write(PRG_REGISTER_DATA,&byte)){
		return 0;
	}
	
	usleep(1);
	
	byte |= CC2430_RESET;

	if(!parport_driver->write(PRG_REGISTER_DATA,&byte)){
		return 0;
	}
}

int parport_start_debug_mode()
{
	unsigned char byte = CC2430_PROG | CC2430_RESET;
	int i = 0;
	
	if(parport_driver == NULL)
		return 0;
			
	if(!parport_driver->write(PRG_REGISTER_DATA,&byte)){
		return 0;
	}
	
	usleep(5);
	
	byte &= ~CC2430_RESET;
		
	if(!parport_driver->write(PRG_REGISTER_DATA,&byte)){
		return 0;
	}
	
    for(i = 0; i < 4; i++){

		usleep(5);
				
    	if(byte & CC2430_CLK)
    		byte &= ~CC2430_CLK;
    	else 
    		byte |= CC2430_CLK;
    		
		if(!parport_driver->write(PRG_REGISTER_DATA,&byte)){
			return 0;
		}
    }        

	usleep(5);
			
    // assert the reset pin - lets start
    byte |= CC2430_RESET;
	if(!parport_driver->write(PRG_REGISTER_DATA,&byte)){
		return 0;
	}
	
	usleep(5);
	
	return 1;
}

int parport_write_byte(unsigned char write_byte)
{
	int i = 0;
	unsigned char byte,b;
	unsigned char param;
	struct timespec interval;
	unsigned char j;
	
	b = write_byte;
	
	// clear the prog bit
	byte = CC2430_RESET;

	for(i = 0; i < 8; i++){
		
		byte |= CC2430_CLK;
		
		// just write the byte
		param = byte | (b >> (7-i)&0x1);
		
		for(j = 10; j > 0; j--){
			parport_driver->write(PRG_REGISTER_DATA,&param);
		}	
		
		//parport_wait();
		
		byte &= ~CC2430_CLK;
		for(j = 10; j > 0; j--){
			parport_driver->write(PRG_REGISTER_DATA,&byte);
		}
	}
	return 1;
}

int parport_read_byte(unsigned char* read_byte)
{	
	int i = 0;
	unsigned char byte, read = 0;
	unsigned char b = 0, j;
	
	byte = CC2430_RESET | CC2430_PROG;
		
	for(i = 0; i < 8; i++){
			
		// assert the clock pin
		byte |= CC2430_CLK;
		
		// clear the clock pin
		for(j = 10; j > 0; j--){
			parport_driver->write(PRG_REGISTER_DATA,&byte);
		}	

		//parport_wait();
				
		//read the data
		for(j = 10; j > 0; j--){
			parport_driver->read(PRG_REGISTER_STATUS,&b);
		}	
		
		read <<= 1;
		read |= (b >> 4) & 1;
		
		byte &= ~CC2430_CLK;
		for(j = 10; j > 0; j--){
			parport_driver->write(PRG_REGISTER_DATA,&byte);
		}
	}	
	
	*read_byte = read;
	
	return 1;
}
