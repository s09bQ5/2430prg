/*
 * parport.h - Parallel port utility functions
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
 
#ifndef __2430PRG_PARPORT_H__
#define __2430PRG_PARPORT_H__

//
// STATUS REGISTER BIT POSITION AND MASK
//
#define CC2430_CABLE_V1_MASK			0xA0
#define CC2430_CABLE_V1					0x80
#define CC2430_VCC_SENSE				0x08

#define CC2430_DONE						0x10

//
// DATA REGISTER BIT POSITION
//
#define CC2430_D6						0x40
#define CC2430_RESET					0x08
#define CC2430_CLK						0x02
#define CC2430_PROG						0x10
#define CC2430_DIN						0x01

typedef struct _cc2430_cable_status{
	//
	// Cable Hardware Version
	//
	unsigned char version;
	//
	// If the CC2430 development board is connected and powered up
	//
	unsigned char deviceConnected;
} cc2430_cable_status;


void parport_register(struct _prg_driver);
int parport_init();
int parport_start_debug_mode();
int parport_reset();
int parport_open();
void parport_close();
int parport_set_bytemode();
int parport_detect_cable(cc2430_cable_status*);
int parport_write_byte(unsigned char);
int parport_read_byte(unsigned char*);


#endif // __2430PRG_PARPORT_H__
