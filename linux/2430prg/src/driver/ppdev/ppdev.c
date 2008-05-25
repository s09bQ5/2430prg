/*
 * ppdev.c - ppdev driver implementation
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
 
 #include "driver.h"
 
 static int ppdev_handler = -1;
 
 unsigned int ppdev_open(void* param)
 {
 	char* device;
 	
 	if(ppdev_handler > 0)
 		return 1;
 	
 	if(param == NULL)
 		return 0;
 		
 	device = (char*)param;
 	
	ppdev_handler = open(device,O_RDWR,1);
	
	return (ppdev_handler > 0 ? 1 : 0);
 }
 
 unsigned int ppdev_init()
 {
	int mode = IEEE1284_MODE_COMPAT;
	int tristate = 0;
	int ret = 0;
	
	if(ppdev_handler < 0)
		return 0;
		
	if(ioctl(ppdev_handler,PPCLAIM,0))
		return 0; 
		
	ioctl(ppdev_handler,PPEXCL,0);
	
	
	ret = ioctl(ppdev_handler,PPSETMODE,&mode);
	if(!ret)
		ret = ioctl(ppdev_handler,PPDATADIR,&tristate);
	
	return (!ret ? 1 : 0);
	
 }
 
 unsigned int ppdev_close()
 {
 	int ret = 0;
 	
	if(ppdev_handler < 0)
		return 0;
	
	ret = ioctl(ppdev_handler,PPRELEASE,0);
	
	close(ppdev_handler);
	
	return (!ret ? 1 : 0);
 }
 
 unsigned int ppdev_read(prg_register_t reg, void* param)
 {
 	unsigned int cmd;
 	unsigned int ret;
 	
 	switch(reg){
 		case PRG_REGISTER_DATA:
 			cmd = PPRDATA;
 			break;
 			
 		case PRG_REGISTER_STATUS:
 			cmd = PPRSTATUS;
 			break;	
 			
 		case PRG_REGISTER_CONTROL:
 		default:
 			return 0;
 				
 	}
 	
 	ret = ioctl(ppdev_handler, cmd, param);
 	
 	return (!ret ? 1 : 0);
 }
 
 unsigned int ppdev_write(prg_register_t reg, void* param)
 {
 	unsigned int cmd;
 	unsigned int ret;
 	
 	switch(reg){
 		case PRG_REGISTER_DATA:
 			cmd = PPWDATA;
 			break;
 			
 		case PRG_REGISTER_STATUS:
 		case PRG_REGISTER_CONTROL:
 		default:
 			return 0;
 				
 	}
 	
 	ret = ioctl(ppdev_handler, cmd, param);
 	
 	return (!ret ? 1 : 0);
 }
 
 struct _prg_driver ppdev_driver = {
 	ppdev_open,
 	ppdev_close,
 	ppdev_init,
 	ppdev_read,
 	ppdev_write
 };
