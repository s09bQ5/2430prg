/*
 * driver.h - parallel port driver definition
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
#ifndef __2430PRG_DRIVER_H__
#define __2430PRG_DRIVER_H__

enum _prg_register {
	PRG_REGISTER_DATA = 0x00,
	PRG_REGISTER_STATUS = 0x01,
	PRG_REGISTER_CONTROL = 0x02
};

typedef enum _prg_register prg_register_t;

struct _prg_driver {
	unsigned int (*open)(void* param);
	unsigned int (*close)();
	unsigned int (*init)();
	unsigned int (*read)(prg_register_t reg, void* param);
	unsigned int (*write)(prg_register_t reg, void* param); 
};

extern struct _prg_driver ppdev_driver;
extern struct _prg_driver io_driver;

#endif /*__2430PRG_DRIVER_H__*/
