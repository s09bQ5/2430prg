/*
 * cc2430.c - CC2430 programming protocol
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

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 
 #include "config.h"
 #include "cc2430.h"
 #include "driver.h"
 #include "parport.h"
  
 int cc2430_chip_erase()
 {
 	unsigned char discard;
 	
 	parport_write_byte(0x14);
 	parport_read_byte(&discard);
 		
 	return 0;
 }

 int cc2430_write_conf(unsigned char conf, unsigned char* ret)
 {
 	unsigned char discard;
 	
 	parport_write_byte(0x1D);
 	parport_write_byte(conf);
 	parport_read_byte(&discard);
 	parport_write_byte(0x24);
 	parport_read_byte(ret);
 		
 	return 0;	
 }
 
 unsigned short int cc2430_get_chip_info()
 {
 	unsigned char byte = 0;
 	unsigned short int ret = 0;
 	
 	parport_write_byte(0x68);
 	parport_read_byte(&byte);
 	
 	ret |= (byte & 0xff);
 	ret <<= 8;	

 	parport_read_byte(&byte);
 	
 	ret |= (byte & 0xff);
 	
 	return ret;
 }
 
 unsigned short int cc2430_get_pc()
 {
 	unsigned char byte = 0;
 	unsigned short int ret = 0;
 	
 	parport_write_byte(0x28);
 	parport_read_byte(&byte);
	
 	ret |= byte;
 	ret <<= 8;	

 	parport_read_byte(&byte);

 	ret |= byte;
 	
 	return ret;
 }
 
 unsigned char cc2430_get_status(unsigned char* ret)
 {
 	parport_write_byte(0x34);
    parport_read_byte(ret);
 		
 	return 0;
 }
 
 unsigned char cc2430_resume()
 {
 	unsigned char discard;
 	
 	parport_write_byte(0x4C);
    parport_read_byte(&discard);
 		
 	return 0;
 }
 
 unsigned char cc2430_send_inst3(unsigned char in0, unsigned char in1, unsigned char in2, unsigned char* out)
 {
 	parport_write_byte(0x57);
 	parport_write_byte(in0);
	parport_write_byte(in1);
 	parport_write_byte(in2);
    parport_read_byte(out);
 		
 	return 0;
 }
 
 unsigned char cc2430_send_inst2(unsigned char in0, unsigned char in1, unsigned char* out)
 {
 	parport_write_byte(0x56);
 	parport_write_byte(in0);
 	parport_write_byte(in1);
    parport_read_byte(out);
 		
 	return 0;
 }
 
 unsigned char cc2430_send_inst1(unsigned char in0, unsigned char* out)
 {
 	parport_write_byte(0x55);
 	parport_write_byte(in0);
    parport_read_byte(out);
    
 	return 0;
 }
 
 unsigned char cc2430_init_clock()
 {
	 unsigned char byte;
	 unsigned char trial = 20;
	 
	 // enable the 32MHz clock
	 cc2430_send_inst3(0x75, 0xC6, 0x00,&byte);	//mov CLKCON,#00
	 
	 //wait for clock
	 do{
		 cc2430_send_inst2(0xE5, 0xBE, &byte);	//mov A,SLEEP
		 usleep(1);
		 trial --;
	 } while( (!(byte & 0x40)) && trial > 0);
	 
	 return (byte & 0x40 ? 0 : 1);
 }
 
 unsigned char cc2430_set_flash_timing(enum flash_timing t)
 {
	 unsigned char byte;
	 unsigned char discard;
	 
	 cc2430_send_inst3(0x75, 0xAB, t,&discard);	//mov FWT, timing
	 cc2430_send_inst2(0xE5, 0xAB, &byte);		//mov A, FWT
	 
	 return byte;
 }
 
 unsigned long int cc2430_read_content(char** buffer,
		 						   		unsigned int content_len,
		 						   		unsigned int start_address_page,
		 						   		unsigned char bank)
 {
	 unsigned char discard;
	 unsigned char byte;
	 char* b = *buffer;
	 unsigned long int address = (start_address_page & 0x7E00);
	 
	 if(buffer == NULL)
		 return -1;
	 
	 //
	 // we can only read 64Kb, to read more we have to change the bank
	 // and only to the 128Kb version 
	 //
	 if(content_len > 0xFFFF){
		 return -1;
	 }
	 
	 //
	 // set up the upper 32Kb mapping. Only for the 128Kb devices.
	 // Will be ignored by the others
	 //
	 cc2430_send_inst3(0x75,0xC7,(bank*16) + 1,&discard);									//MOV MEMCTR, (bank*16) + 1
	 cc2430_send_inst3(0x90,((address >> 8) & 0xFF),(address & 0xFF),&discard);				//MOV DPTR, address
	 
	 while((b - *buffer) < content_len){
		 cc2430_send_inst1(0xE4,&discard);													//CLR A
		 cc2430_send_inst1(0x93,&byte);														//MOVC A, @A+DPTR
		 cc2430_send_inst1(0xA3,&discard);													//INC DPTR
		 *b = byte;
		 b++;
	 }
 	
	 return (b - *buffer);
 }
 

 unsigned int cc2430_read_debug_string(char** buffer,
 								unsigned int content_len,
 						   		unsigned int address)
 {
	 unsigned char discard;
	 unsigned char byte;
	 char* b = *buffer;
	 
	 if(buffer == NULL)
		 return -1;
	 
	 //
	 // we can only read 64Kb, to read more we have to change the bank
	 // and only to the 128Kb version 
	 //
	 if(content_len > 0xFFFF){
		 return -1;
	 }
	 
	 //
	 // set up the upper 32Kb mapping. Only for the 128Kb devices.
	 // Will be ignored by the others
	 //
	 cc2430_send_inst3(0x90,((address >> 8) & 0xFF),(address & 0xFF),&discard);				//MOV DPTR, address
	 
	 while((b - *buffer) < content_len){
		 cc2430_send_inst1(0xE0,b);															//MOVX A, @DPTR
		 cc2430_send_inst1(0xA3,&discard);													//INC DPTR
		 b++;
		 
		 if(byte == 0)
		 	break;
	 }
	 
	 return (b - *buffer);
 }
 						   		
 unsigned int cc2430_read_xdata(char** buffer,
 								unsigned int content_len,
 						   		unsigned int address)
{
	 unsigned char discard;
	 char* b = *buffer;
	 
	 if(buffer == NULL)
		 return -1;
	 
	 //
	 // we can only read 64Kb, to read more we have to change the bank
	 // and only to the 128Kb version 
	 //
	 if(content_len > 0xFFFF){
		 return -1;
	 }
	 
	 //
	 // set up the upper 32Kb mapping. Only for the 128Kb devices.
	 // Will be ignored by the others
	 //
	 cc2430_send_inst3(0x90,((address >> 8) & 0xFF),(address & 0xFF),&discard);				//MOV DPTR, address
	 
	 while((b - *buffer) < content_len){
		 cc2430_send_inst1(0xE0,b);										//MOVX A, @DPTR
		 cc2430_send_inst1(0xA3,&discard);								//INC DPTR
		 b++;
	 }
	 
	 return (b - *buffer);
} 						   		
 
unsigned int cc2430_write_xdata(char * binary_content,
 									  unsigned int content_len,
 									  unsigned int address)
 {
 	unsigned char discard;
	char* b = binary_content;
 	
 	cc2430_send_inst3(0x90,HIWORD(address),LOWORD(address),&discard);			//MOV DPTR, address
 	
	while( (b - binary_content) < content_len){
		 cc2430_send_inst2(0x74,*b,&discard);					//MOV A,#imm
		 cc2430_send_inst1(0xF0,&discard);						//MOVX @DPTR, A
		 cc2430_send_inst1(0xA3,&discard);						//INC DPTR
		 b++;
 	}
 	
 	return (b - binary_content);
 }
 
 unsigned int cc2430_write_content(char* binary_content, 
		 							unsigned int content_len, 
		 							unsigned int flash_word_size,
		 							unsigned int flash_words_per_page,
		 							unsigned int start_address_page)
 {
 	unsigned char start_address = HIWORD(start_address_page) & 0x7E;
 	unsigned int length = content_len;
 	unsigned int i = 0;
 	unsigned int bytes_per_page = flash_words_per_page * flash_word_size;
 	char* content, *c;
 	unsigned char discard;
 	unsigned int jmp_addr;
 	unsigned char status;
 	
 	char asm_code[] = {
 	 	0x75, 0xAD, 0x00/*address*/, 		//    mov FADDRH,#imm
 	 	0x75, 0xAC, 0x00,					//    mov FADDRL,#00
 	 	0x75, 0xAE, 0x01,					//    mov FCTL,#01H
 	 	
 	 	0xE5, 0xAE,							//a:  mov A,FCTL
 	 	0x20, 0xE7, 0xFB,					//	  jb FCTL.BUSY,a
 	 	
 	 	0x90, 0xF0, 0x00,					//    mov DPTR,#0F000H
 	 	
 	 	0x7F, HIWORD(flash_words_per_page), //    mov R7, #imm
 	 	0x7E, LOWORD(flash_words_per_page), //    mov R6, #imm
 	 	0x75, 0xAE, 0x02,				    //    mov FCTL, #02H
 	 	
 	 	0x7D, flash_word_size,				//b:  mov R5, #imm
 	 	0xE0,								//c:  mov A, @DPTR
 	 	0xA3,								//	  inc DPTR
 	 	0xF5, 0xAF,							//    mov FWDATA, A
 	 	0xDD, 0xFA,							//    djnz R5, c
 	 	
 	 	0xE5, 0xAE,							//d:  mov A,FCTL
 	 	0x20,0xE6,0xFB,						//    jb FCTL.SWBSY,d
 	 	0xDE, 0xF1,							//    djnz R6, b
 	 	0xDF, 0xEF,							//	  djnz R7, b
 	 	
 	 	0xA5
 	};
 	
 	while( ((length / flash_word_size) % flash_words_per_page) != 0)
 		length ++;
 		
 	//
 	// align with pages
 	//
 	content = malloc(length);
 	memset(content, 0xFF, length);
 	memcpy(content, binary_content, content_len);
 	
	printf("length = 0x%x\n",length);
 		
 	for(c = content; (c - content) < length; c += bytes_per_page){
 		
 		printf("writing data\n");
 		cc2430_write_xdata(c,bytes_per_page,0xF000);
 		printf("writing code\n");
 		asm_code[2] = ((HIWORD((c - content)) / flash_word_size) & 0x7E) + start_address;
 		cc2430_write_xdata(asm_code,sizeof(asm_code),0xF000 + bytes_per_page);
	 	cc2430_send_inst3(0x75,0xC7,0x51,&discard);								//MOV MEMCTR, #imm
	 	jmp_addr = 0xF000 + bytes_per_page;
 		cc2430_send_inst3(0x02,HIWORD(jmp_addr),LOWORD(jmp_addr),&discard);		//ljmp 0xF000 + bytes_per_page
 		printf("executing and copying to flash memory\n");
 		cc2430_resume();
 		
 		do{
 			cc2430_get_status(&status);
 		} while(!(status & 0x20)); 
 	}
 	
 	i = (c - content);
 	free(content);
 	
	return i;
 }
