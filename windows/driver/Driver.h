//
//    CC2430Cable parallel interface driver
//
//    Driver.h
//    Copyright (C) 2007  Otavio Ribeiro
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#pragma once

#define INITGUID

#include <initguid.h>
#include <ntddk.h>
#include <wdf.h>
#include <parallel.h>


#include "CableDevice.h"
#include "CableQueue.h"
#include "ParPortEnum.h"
#include "ParPort.h"
#include "CC2430Cable.h"

DRIVER_INITIALIZE DriverEntry;

NTSTATUS ParallelCableAdd(IN WDFDRIVER Driver,
						  IN PWDFDEVICE_INIT DeviceInit
						  );