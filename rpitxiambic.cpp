#include <unistd.h>
#include "librpitx/src/librpitx.h"
#include "stdio.h"
#include <cstring>
#include <signal.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

bool running=true;

#define PROGRAM_VERSION "0.1"


void print_usage(void)
{

fprintf(stderr,\
"\nrpitxiambic -%s\n\
On GPIO 10 and 9 with active on +3.3v or invert\n\
Usage:\ntune [-w wpm] [-f frequency] [-h] \n\
-i for invert and active gpio on 0v\n\
-w int word per minute,\n\
-f float      frequency carrier Hz(50 kHz to 1500 MHz),\n\
-e exit immediately without killing the carrier,\n\
-h            help (this help).\n\
\n",\
PROGRAM_VERSION);

} /* end function print_usage */

static void terminate(int num)
{
    running=false;
	fprintf(stderr,"Caught signal - Terminating\n");
   
}

//####
static int GPIOExport(int pin)
{
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIODirection(int pin, int dir)
{
	static const char s_directions_str[]  = "in\0out";

#define DIRECTION_MAX 35
	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, &s_directions_str[0 == dir ? 0 : 3], 1 == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

static int GPIORead(int pin)
{
#define VALUE_MAX 30
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}
//####


int main(int argc, char* argv[])
{
	int a;
	int anyargs = 0;
	float SetFrequency=434e6;
	int  SetWPM=20;
	bool NotKill=false;
	int GLvl=1;

	GPIOExport(10);
	GPIODirection(10,0);//0 in 1 out
	GPIOExport(9);
	GPIODirection(9,0);

	while(1)
	{
		a = getopt(argc, argv, "w:f:ehi");
	
		if(a == -1) 
		{
			if(anyargs) break;
			else a='h'; //print usage and exit
		}
		anyargs = 1;	

		switch(a)
		{
                case 'i': // invert level active gpio
                        GLvl = 0;
                        break;
		case 'w': // cw speed
                        SetWPM = atof(optarg);
                        break;
		case 'f': // Frequency
			SetFrequency = atof(optarg);
			break;
		case 'e': // SampleRate (Only needeed in IQ mode)
			NotKill=true;
			break;
		case 'h': // help
			print_usage();
			exit(1);
			break;
		case -1:
        	break;
		case '?':
			if (isprint(optopt) )
 			{
 				fprintf(stderr, "tune: unknown option `-%c'.\n", optopt);
 			}
			else
			{
				fprintf(stderr, "tune: unknown option character `\\x%x'.\n", optopt);
			}
			print_usage();

			exit(1);
			break;			
		default:
			print_usage();
			exit(1);
			break;
		}/* end switch a */
	}/* end while getopt() */

	
	
	 for (int i = 0; i < 64; i++) {
        struct sigaction sa;

        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
    }

		generalgpio gengpio;
		gengpio.setpulloff(4);
		padgpio pad;
		pad.setlevel(7);
		clkgpio *clk=new clkgpio;
		clk->SetAdvancedPllMode(true);
		clk->SetCenterFrequency(SetFrequency,10);
		clk->SetFrequency(000);
		clk->enableclk(4);
		usleep(100000);
		clk->SetCenterFrequency(1500000000,10);
		//clk->enableclk(6);//CLK2 : experimental
		//clk->enableclk(20);//CLK1 duplicate on GPIO20 for more power ?
		int twait = (1200/SetWPM)*1000;
		int utwait = twait/50;
		int pwr [] = { 0, 1, 2, 2, 3, 3 , 4, 4, 5, 7};
		if(!NotKill)
		{
			while(running)
			{
                        	if(GPIORead(9) == GLvl){
					printf("dit\n");
					clk->SetCenterFrequency(SetFrequency,10);
					for (int i=0; i<=9; ++i){pad.setlevel(pwr[i]);usleep(utwait);}
					for (int i=0; i<32; ++i){usleep(utwait);}
					for (int i=9; i<=0; --i){pad.setlevel(pwr[i]);usleep(utwait);}
					clk->SetCenterFrequency(1500000000,10);
					usleep(twait);
				}
                                if(GPIORead(10) == GLvl){
                                        printf("dah\n");
                                        clk->SetCenterFrequency(SetFrequency,10);
                                        for (int i=0; i<=9; ++i){pad.setlevel(pwr[i]);usleep(utwait);}
                                        for (int i=0; i<32; ++i){usleep(utwait*3);}
                                        for (int i=9; i<=0; --i){pad.setlevel(pwr[i]);usleep(utwait);}
                                        clk->SetCenterFrequency(1500000000,10);
                                        usleep(twait);
                                }
				usleep(1000);
			}
			clk->disableclk(4);
			delete(clk);
		}
		else
		{
			//Ugly method : not destroying clk object will not call destructor thus leaving clk running 
		}
	
	
}	

