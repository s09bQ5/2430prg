/*
 * 2430prg - Programming CC2430 devices using the CC2430 cable project
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <sys/resource.h>
#include <locale.h>

#include "config.h"
#include "driver.h"
#include "cc2430.h"
#include "parport.h"


#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#else
#  define _(String) (String)
#endif

#define PARPORT_DEFAULT_DEVICE "/dev/parport0"

#define FLAG_BULK_ERASE			0x01
#define FLAG_PROTECT			0x02
#define FLAG_USE_IO				0x04
#define FLAG_USE_PPDEV			0x08

enum input_format {
	FORMAT_BIN = 1,
	FORMAT_HEX
};

enum option {
	OPT_DEV = 0,
	OPT_BULK_ERASE,
	OPT_PROTECT,
	OPT_CHECK,
	OPT_VERSION,
	OPT_HELP,
	OPT_READ,
	OPT_FILE_FORMAT,
	OPT_DEBUG,
	OPT_IO_PORT,
	OPT_UNKNOWN
};

enum param {
	no_argument,
	required_argument,
	optional_algument
};

struct available_file_format {
	enum input_format format_id;
	char* format;
};

static struct available_file_format formats[] = {
	{FORMAT_BIN,"bin"},
	{FORMAT_HEX,"hex"},
	{0,NULL}
};

struct available_option {
	char* option;
	enum option option_id;
	enum param param_info;
};

static struct available_option options[] = {
	{"-d",OPT_DEV,required_argument},			//overwrite the default parallel port device
	{"-a",OPT_IO_PORT,optional_algument},		//parallel port io address to be used with a direct access
	{"-e",OPT_BULK_ERASE,no_argument}, 			//if protected, bulk erase the device before programming
	{"-p",OPT_PROTECT,no_argument},				//after programming, protected the device
	{"-c",OPT_CHECK,no_argument},				//only check the cable and devie without programming anything
	{"-r",OPT_READ,no_argument},				//try to read the device to the file instead writing
	{"-f",OPT_FILE_FORMAT,required_argument},	//input file format
	{"-t",OPT_DEBUG,no_argument},				//test mode used to debug our software
	{"--version",OPT_VERSION,no_argument},		//show the version message
	{"--help",OPT_HELP,no_argument},			//show the help message
	{"-v",OPT_VERSION,no_argument},				//show the version message
	{"-h",OPT_HELP,no_argument},				//show the version message
	{"",OPT_UNKNOWN,no_argument},				//unknown option
	{NULL,0,0},
};

enum mode {
	OP_READ,
	OP_WRITE,
	OP_CHECK,
	OP_DEBUG
};

struct chip_ids {
	unsigned char id;
	char* name;
};

static struct chip_ids ids[] = {
	{0x01,"CC1110"},
	{0x85,"CC2430"},
	{0x89,"CC2431"},
	{0x81,"CC2510"},
	{0x91,"CC2511"},
	{0x00,NULL},
};

//
// what we have to do
//
static enum mode todo;

//
// parport device name
//
static char device_name[PATH_MAX];

//
// io port address
//
unsigned int pport_address;

//
// file name0
//
static char file_name[PATH_MAX];

//
// flags
//
static unsigned char flags;

//
// input file format
//
static enum input_format format;

//
// binary data to write or read
//
static char* binary_content = NULL;

//
// binary content size
//
unsigned long int content_length = 0;

int _off = 0;
void __exit(int sig){
	_off = 1;
	printf("\n\nexiting...\n\n");
}

//
// Check and print chip information
//
static char check_chip_id(unsigned char chip_id)
{
	int i = 0;
	struct chip_ids* sIds = ids;
	while(sIds[i].name != NULL){
		if(chip_id == sIds[i].id){
			printf(_("%s found...\n"),sIds[i].name);
			return i;
		}
		i++;
	}
	
	printf(_("ERROR: Invalid chip ID\n"));
	return -1;
}

//
// Show version message
//
static void show_version_message()
{
	printf(_("2430prg version 1.0\n"
				"Copywrite (C) Otávio Ribeiro <otavio@otavio.eng.br> @ 2007\n\n"));
}

//
// Show usage message
//
static void show_usage_message()
{
	printf(_("2430prg [options]... FILE\n"
				"Copywrite (C) Otávio Ribeiro <otavio@otavio.eng.br> @ 2007\n\n"
				"-d\t\tOverwrite the default parallel port device to use - ppdev device name\n"
				"-a\t\tParallel port address, to be used with direct accress only\n"
				"  \t\tAddress 0x378 is used by default\n"
				"-e\t\tbulk erase the device before programming it\n"
				"-p\t\tafter programming the device protect it from reading\n"
				"-c\t\tjust check the device without read or write on it\n"
				"-f\t\tinput file format\n"
				"  \t\tbin - binary file format\n"
				"  \t\thex - intel hexadecimal file format\n"
				"-t\t\ttest mode - use it to debug\n"
				"-v\t\tshow the version of this software\n"
				"-h\t\tshow this message\n\n"));
}

static struct available_option* find_option(char* str)
{
	struct available_option *option = NULL;
	int i = 0;
	
	while(options[i].option != NULL){
		if(strcmp(options[i].option,str) == 0 || (options[i].option_id == OPT_UNKNOWN && str[0] == '-') ){
			option = &options[i];
			break;
		}
		i++;
	}
	
	return option;
}

static unsigned char find_file_format(char* param)
{
	int i = 0;
	while(formats[i].format != NULL){
		if(strcmp(formats[i].format,param) == 0){
			return formats[i].format_id;
		}
		i++;
	}
	return 0;
}

//
// Parses the command line options send to us
//
static int parse_main_options(int argc, char** argv)
{
	int i = 1;
	char* str, *param;
	struct available_option* option;
	
	if(argc <= 1){
		show_usage_message();
		return -1;
	} else {
		for(i = 1; i < argc; i++){
			str = argv[i];
			if(str == NULL)
				continue;

			option = find_option(str);
			if(option == NULL){
				strcpy(file_name, str);
				continue;
			}
			
			param = argv[i+1];

			if(param == NULL || option->param_info != required_argument || (find_option(param) != NULL && i == argc-1)){
				param = NULL;
			} else {
				i++;
			}
			
			if(option == NULL){
				printf(_("the option %s is not a valid option. Use --help to see all available options.\n"),str);
				return -1;
			}

			if(option->param_info == required_argument && param == NULL){
				printf(_("the option %s requires an argument. Use --help to see all available options.\n"),str);
				return -1;
			}
			
			switch(option->option_id){
				case OPT_DEV:
					strcpy(device_name,param);
					flags |= FLAG_USE_PPDEV;
					break;
				
				case OPT_IO_PORT:
					flags |= FLAG_USE_IO;
					if(param == NULL)
						pport_address = 0x378;
					else
						pport_address = atoi(param);
						
					break;
					
				case OPT_BULK_ERASE:
					flags |= FLAG_BULK_ERASE;
					break;
				
				case OPT_PROTECT:
					flags |= FLAG_PROTECT;
					break;
					
				case OPT_READ:
					todo = OP_READ;
					break;	
				
				case OPT_CHECK:
					todo = OP_CHECK;
					break;
					
				case OPT_DEBUG:
					todo = OP_DEBUG;
					break;
					
				case OPT_FILE_FORMAT:
					format = find_file_format(param);
					if(format == 0){
						printf(_("Invalid file format. Use --help to check the availables formats."));
						return -1;
					}
					break;

				case OPT_VERSION:
					show_version_message();
					return -1;
				
				case OPT_HELP:
					show_usage_message();
					return -1;
				
				case OPT_UNKNOWN:
					printf(_("unknown parameter %s. Ignored...\n"),str);
					break;
				
				default:
					return -1;
			}
		}
	}
	
	if( (flags & FLAG_USE_IO) && (flags & FLAG_USE_PPDEV) ){
		printf(_("The options -a and -d can not be used together.\nUse --help to see how to use this software\n"));
		return -1;
	}

	if( ( (flags & FLAG_PROTECT) || (flags & FLAG_BULK_ERASE)  ) && (todo == OP_CHECK || todo == OP_DEBUG)){
		printf(_("The options -e and -p are incompatible with -c.\nUse --help to see how to use this software\n"));
		return -1;
	}
	
	if( ( (flags & FLAG_PROTECT) || (flags & FLAG_BULK_ERASE)  ) && (todo == OP_CHECK || todo == OP_DEBUG)){
		printf(_("The options -e and -p are incompatible with -c.\nUse --help to see how to use this software\n"));
		return -1;
	}

	if(file_name[0] == 0 && todo != OP_CHECK && todo != OP_DEBUG){
		printf(_("You must provide a file name. Use --help to see how to use this software.\n"));
		return -1;
	}
	
	return 0;
}

static int read_file(char* file_name)
{
	FILE * fp;
	long int file_len;
	char * content;
	size_t read;
	int ret = 0;
	
	if((fp = fopen(file_name,"r")) == NULL){
		printf(_("Error opening file: %s\n"),file_name);
		return -1;
	}
	
	fseek(fp,0,SEEK_END);
	file_len = ftell(fp);
	fseek(fp,0,SEEK_SET);
	
	content = (char*)malloc(file_len);
	
	read = fread(content,sizeof(char),file_len,fp);
	if(read == file_len){
		if(format == FORMAT_BIN){
			binary_content = content;
			content_length = file_len;
		} else if(format == FORMAT_HEX){
			printf(_("HEX file format not implemented\n"));
			ret = -1;
		}
	} else {
		printf(_("Error reading file.\n"));
		ret = -1;
	}
	
	if(format != FORMAT_BIN || ret != 0)
		free(content);
	
	fclose(fp);
	return ret;
}

static char verify_content()
{
	char ret = 1;
	char* buffer;
	unsigned long int len = content_length;
	
	//
	// check only the first 32Kb
	//
	if(len >= 0x8000){
		len = 0x7FFF;
	}
	
	buffer = malloc(len);
	
	//
	// Read the content from device flash memory
	//
	if(cc2430_read_content(&buffer,
						len,
						0,
						0) >= len){
							
		if(memcmp(buffer,binary_content,len) == 0)
			ret = 0;					
	}	
	
	free(buffer);
	return ret;
}

static unsigned long int write_to_device()
{
	int ret = 0;
	
	if(binary_content == NULL)
		return 0;
		
	//
	// enable the 32MHz clock
	//
	if(cc2430_init_clock()){
		printf(_("Error enabling the 32MHz clock\n"));
		return 0;
	}	
	
	//
	// set up flash timing
	//
	if(cc2430_set_flash_timing(FLASH_32MHZ) != FLASH_32MHZ){
		printf(_("Error enabling the flash timer\n"));
		return 0;
	}
	
	ret = cc2430_write_content(binary_content,
							content_length,
							4,
							512,
							0);
	
	return ret;
}

void debug_device()
{
 	unsigned int jmp_addr = 0x0000;
 	char * msg;
 	unsigned char status, discard;
 	unsigned short int pc;

	//
	// enable the 32MHz clock
	//
	if(cc2430_init_clock()){
		printf(_("Error enabling the 32MHz clock\n"));
		return 0;
	}	
	
	msg = malloc(256);

 	//set PC to 0x0000
 	cc2430_send_inst3(0x02,HIWORD(jmp_addr),LOWORD(jmp_addr),&discard);		//ljmp 0x0000
 	
 	do{
	 	//start executing our code
		printf("resuming...\n");
		cc2430_resume();
		
		//wait for a breakpoint to read the message
		do{
			cc2430_get_status(&status);
		} while( !(status & 0x20) && _off != 1);
		
		pc = cc2430_get_pc();
		
		if(_off != 1){
			printf("reading message\n");
			cc2430_read_debug_string(&msg,255,0xE000);
			printf("debug message --> %s\n",msg);
		}
		
		pc++;
 		cc2430_send_inst3(0x02,HIWORD(pc),LOWORD(pc),&discard);		//ljmp PC+1
		 
 	} while(_off != 1);
 	
 	free(msg);
}

int main(int argc, char** argv)
{
	cc2430_cable_status cable_status;
	unsigned short int chip_info;
	unsigned char chip_id, chip_revision, status;
	void * drv_param;
	
	//
	// Setup with the default parallel port device.
	// That can be overwrited later using the command line.
	//
	strcpy(device_name,PARPORT_DEFAULT_DEVICE);

	flags = 0;
	todo = OP_WRITE;
	format = FORMAT_BIN;
	
	//
	// No file name
	//
	file_name[0] = 0;

	if(parse_main_options(argc,argv) < 0)
		return -1;
	
	//
	// All C applications, by default are started with 'C' locale by default.
	// The next call inform the libc to use the system environment locale.
	//
	setlocale(LC_ALL,"");
	
	signal(SIGINT,__exit);
	signal(SIGHUP,__exit);
	signal(SIGQUIT,__exit);
	signal(SIGTERM,__exit);
	
	//
	// Setup the gettext domain to look for messages catalog
	//
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain(GETTEXT_PACKAGE);
#endif
	 
    printf(_("2430prg initializing\n"));
    
    if(flags & FLAG_USE_IO){
    	printf(_("2430prg using direct access I/O driver on port: 0x%x\n"),pport_address);
    	parport_register(io_driver);
    	drv_param = &pport_address;
    } else {	
    	printf(_("2430prg using PPDEV driver on device: %s\n"),device_name);
    	parport_register(ppdev_driver);
    	drv_param = device_name;
    }	

	if(!parport_open(drv_param))
		printf(_("error opening parallel port..\n"));
		
	if(!parport_init())	
		printf(_("error initializing parallel port..\n"));
	
	// check if the cable is connected
	if(!parport_detect_cable(&cable_status)){
		printf(_("error detecting the CC2430 cable\n"));
		goto exit;
	}
	
	// cable version	
	if(cable_status.version != 0 ){
		printf(_("CC2430 cable has been found\n"));
		if(cable_status.version == 1)
			printf(_("CC2430 cable version 1.0 found\n"));
	} else {
		printf(_("CC2430 cable not connected\n"));	
		goto exit;
	}

	printf(_("Detecting CC2430 device type. Please Wait...\n"));
	
	// start the CC2430 debug mode
	if(!parport_start_debug_mode()){
		printf(_("error entering in debug mode\n"));
		goto exit;
	}
	
	//get the chip info
	chip_info = cc2430_get_chip_info();
	
	chip_revision = chip_info & 0xFF;
	chip_id = (chip_info >> 8) & 0xFF;
	
	if(check_chip_id(chip_id) < 0){
		goto exit;
	}
	
	printf(_("Chip revision = %d\n"),chip_revision);
	
	if(cc2430_get_status(&status)){
		printf(_("error reading debug status register\n"));
		goto exit;
	}
	
	printf(_("Debug Status = 0x%x\n"),status);
	
	switch(todo){
		case OP_DEBUG:
			debug_device();
			break;
			
		case OP_READ:
			break;
		
		case OP_WRITE:
		
			if(flags & FLAG_BULK_ERASE){
				printf(_("Erasing device\n"));
				if(cc2430_chip_erase()){
					printf(_("Error erasing device\n"));
					goto exit;
				}
				
				do{
					if(cc2430_get_status(&status)){
						printf(_("error reading debug status register\n"));
						goto exit;
					}
					usleep(1);
				} while(!(status & 0x80));
			}
		
			printf(_("reading file %s \n"),file_name);
			if(read_file(file_name)){
				printf(_("Error trying to read file: %s\n"),file_name);
				goto exit;
			}
			
			printf(_("Writing binary data to device\n"));
			if(write_to_device() < content_length){
				printf(_("Error writing binary data to device\n"));
				goto exit;
			} 
			
			printf(_("Verifying data...\n"));
			if(verify_content()){
				printf(_("Verification error. The data wrote to device is different from file.\n"));
				goto exit;
			}
			
			//
			// reset the device and run the uploaded code
			//
			parport_reset();
			
			break;
			
		default:
			break;
	}
									
exit:

	if(binary_content != NULL){
		free(binary_content);
	}

	parport_close();

	return 0;
}
