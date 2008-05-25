/*
 * io.c - direct io driver implementation
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
 
 #include <sys/perm.h>
 #include <sys/io.h>
 
 #include "driver.h"
 
 static int io_port_address = 0x378;
 
 unsigned int io_open(void* param)
 {
 	int * p = param;
 	
 	if(param == NULL)
 		return 0;
 		
 	io_port_address = *p;
 	
 	return 1;
 }
 
 unsigned int io_close()
 {
 	int ret = ioperm(io_port_address,4,0) | iopl(0);
 	return (!ret ? 1 : 0);
 }
 
 unsigned int io_init()
 {
 	int ret = iopl(3) | ioperm(io_port_address,4,1);
 	return (!ret ? 1 : 0);
 }
 
 unsigned int io_write(prg_register_t reg, void* param)
 {
 	outb(*((unsigned char*)param),io_port_address + reg);
 	return 1;
 }
 
 unsigned int io_read(prg_register_t reg, void* param)
 {
 	*((unsigned char*)param) = inb(io_port_address + reg);
 	return 1;
 }
 
 struct _prg_driver io_driver = {
 	io_open,
 	io_close,
 	io_init,
 	io_read,
 	io_write
 };
