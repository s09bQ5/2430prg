/*
 * cc2430.h - CC2430 programming protocol declarations
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

#ifndef __2430PRG_CC2430_H__
#define __2430PRG_CC2430_H__

enum flash_timing {
	FLASH_12MHZ = 0x10,
	FLASH_13MHZ = 0x11,
	FLASH_16MHZ = 0x15,
	FLASH_24MHZ = 0x20,
	FLASH_26MHZ = 0x23,
	FLASH_32MHZ = 0x2A
};

 #define HIWORD(x) ((x >> 8) & 0xFF)
 #define LOWORD(x) (x & 0xFF)
 
 
 unsigned char cc2430_resume();
 unsigned char cc2430_get_status(unsigned char*);
 unsigned short int cc2430_get_pc();
 unsigned short int cc2430_get_chip_info();
 int cc2430_write_conf(unsigned char, unsigned char*);
 int cc2430_chip_erase();
 
 unsigned char cc2430_send_inst1(unsigned char, unsigned char*);
 unsigned char cc2430_send_inst2(unsigned char, unsigned char, unsigned char*);
 unsigned char cc2430_send_inst3(unsigned char, unsigned char, unsigned char, unsigned char*);
 
 unsigned char cc2430_init_clock();
 unsigned char cc2430_set_flash_timing(enum flash_timing);
 
 unsigned long int cc2430_read_content(char**,unsigned int,unsigned int,unsigned char);
 unsigned int cc2430_read_debug_string(char**,unsigned int,	unsigned int);
 unsigned int cc2430_read_xdata(char** ,unsigned int,unsigned int);
 unsigned int cc2430_write_xdata(char *,unsigned int, unsigned int);
 									  
 unsigned int cc2430_write_content(char* binary_content, 
		 							unsigned int content_len, 
		 							unsigned int flash_word_size,
		 							unsigned int flash_words_per_page,
		 							unsigned int start_address_page);
		 							
		 							


#endif /*__2430PRG_CC2430_H__*/
