/*
	This file is part of k8047.

    k8047 is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    k8047 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with k8047.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>

//Grundeinstellungen
int count=1;
int interval=10;
char device[256]="/dev/usb/hiddev0";
int rc1=30;
int rc2=30;
int rc3=30;
int rc4=30;
int led=0;
float mycal[4][4]={
	{1,1,1,1},
	{1,1,1,1},
	{1,1,1,1},
	{1,1,1,1}
};
char titel[80];
int titel_b=0;

float ref_u=0;
int ref=0;

//config lesen
config(char *filename) {
	char key[30],value[51];
	FILE *datei;
	key[29]=0x0;
	value[50]=0x0;

	datei = fopen (filename, "r");
	if(datei != NULL) {
		while (EOF!= fscanf (datei, "%29s %49s", key, value)) {
			//printf ("%s --- %s\n", key, value);

			if (0==strcmp("count",key)) {
				count =atoi(value);
				continue;
			}

			if (0==strcmp("interval",key)) {
				interval =atoi(value);
				continue;
			}

			if (0==strcmp("led",key)) {
				led =atoi(value);
				continue;
			}

			if (0==strcmp("rc1",key)) {
				rc1 =atoi(value);
				continue;
			}
			if (0==strcmp("rc2",key)) {
				rc2 =atoi(value);
				continue;
			}
			if (0==strcmp("rc3",key)) {
				rc3 =atoi(value);
				continue;
			}
			if (0==strcmp("rc4",key)) {
				rc4 =atoi(value);
				continue;
			}
			if (0==strcmp("cal_c1",key)) {
				sscanf(value,"%f,%f,%f,%f", 
						&mycal[0][0], &mycal[0][1], &mycal[0][2], &mycal[0][3]);
				continue;
			}
			if (0==strcmp("cal_c2",key)) {
				sscanf(value,"%f,%f,%f,%f", 
						&mycal[1][0], &mycal[1][1], &mycal[1][2], &mycal[1][3]);
				continue;
			}
			if (0==strcmp("cal_c3",key)) {
				sscanf(value,"%f,%f,%f,%f", 
						&mycal[2][0], &mycal[2][1], &mycal[2][2], &mycal[2][3]);
				continue;
			}
			if (0==strcmp("cal_c4",key)) {
				sscanf(value,"%f,%f,%f,%f", 
						&mycal[3][0], &mycal[3][1], &mycal[3][2], &mycal[3][3]);
				continue;
			}
		} 		
		fclose (datei);
	} else {
		fprintf(stderr,"Error: Kann Konfiguration nicht laden.\n");
	}

}

//Komandozeile
opt(int argc, char **argv) {
	char c;
	int option_index = 0;
	struct option long_options[] =
	{
		{"help",       no_argument,       0, 'h'},
		{"led",        no_argument,       0, 'l'},
		{"interval",   required_argument, 0, 'i'},
		{"dev",        required_argument, 0, 'd'},
		{"count",      required_argument, 0, 'c'},
		{"ranges",     required_argument, 0, 'r'},
		{"config",     required_argument, 0, 'f'},
		{"cal",        required_argument, 0, 'k'},
		{"title",      optional_argument, 0, 't'},
		//{"quiet",   no_argument,        &q, 1},
	//	{"q",       no_argument,        &q, 1},
		{0, 0, 0, 0}
	};
	while ((c = getopt_long (argc, argv, "hli:c:d:r:f:k:t::",long_options,&option_index)) != -1)
    	switch (c) {
			case 'h':
				printf("k8047 [options] [data]\
				\noptions:\
				\n-h, --help       help\
				\n-i, --interval   read interval (ms) [10]\
				\n-c, --count      count reads [1]\
				\n-d, --dev        device [/dev/usb/hiddev0]\
				\n-r, --ranges     ranges [30,30,30,30]\
				\n-l, --led        record led\
				\n-f, --config     config file []\
				\n-k, --cal        ref (V) [0]\
				\n-t, --title      write head []\
				");
				exit(0);
			case 'l':
				led=1;
				break;
			case 'i':	//zeitverzögerung
				interval = atoi(optarg);
				break;
			case 'c':	//anzahl der schleifen
				count = atoi(optarg);
				break;
			case 'd':	//das hid gerät
				strncpy(device,optarg,256);
				break;
			case 'r':	//die messbereiche
				sscanf(optarg, "%i,%i,%i,%i", &rc1, &rc2, &rc3, &rc4);
				break;
			case 'f':	//configuration laden
				config(optarg); 
				break;
			case 'k':	//calibration
				ref_u=atof(optarg);
				ref=1;
				break;
			case 't':	//eine kopfzeile schreiben
				if(optarg !=NULL) {
					strncpy(titel,optarg,78);
				}
				titel_b=1;
				break;
	}
}	

main(int argc,char **argv)
{
	int co;
	float c1,c2,c3,c4;
	float sc1=0 ,sc2=0 ,sc3=0 ,sc4=0 ;

	opt(argc,argv);

	k8047_init(device);
	k8047_wcal(mycal);

	//	3V 6V 15V 30V 
	k8047_write(led,rc1,rc2,rc3,rc4);

	if (titel_b) {
		k8047_head(titel);
	}

	int c;
	for (c=0 ; c < count ; c++) {
		k8047_read(&co, &c1, &c2, &c3, &c4);
		printf("%i %7.3f %7.3f %7.3f %7.3f\n",co, c1, c2, c3, c4);

		if(ref) {
			sc1=sc1+c1;
			sc2=sc2+c2;
			sc3=sc3+c3;
			sc4=sc4+c4;
		}

		usleep(interval*1000);
	}

	if(ref) {
		sc1=sc1/count;
		sc2=sc2/count;
		sc3=sc3/count;
		sc4=sc4/count;
		printf("      %7.3f %7.3f %7.3f %7.3f\n", sc1, sc2, sc3, sc4);
		printf("      %7.3f %7.3f %7.3f %7.3f\n", 
				ref_u/sc1, ref_u/sc2, ref_u/sc3, ref_u/sc4);
	}

	exit(0);
}

