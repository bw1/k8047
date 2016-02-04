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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/hiddev.h>
#include <linux/input.h>
#include <time.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>


int hid_fd; 
struct hiddev_report_info rep_info_i,rep_info_u;	
struct hiddev_usage_ref_multi ref_multi_i,ref_multi_u;

struct Tch {
	unsigned char b1,b2;
}; 

union Treg {
	unsigned short w;
	struct Tch c;
}; 

//aktueller bereich der einzelnen kanäle
int range[4];

//multiplikator per chanel per bereich
float cal[4][4];

k8047_wcal(float ca[4][4]) {
	int a,b;
	for(a=0; a <4; a++) 
			for(b=0; b <4; b++) {
		cal[a][b]=ca[a][b];
	}
}

float k8047_cal(int ch, int ber) {
	int b=3;
	switch (ber) {
		case 3:
			b=0;
			break;
		case 6:
			b=1;
			break;
		case 15:
			b=2;
			break;
		case 30:
			b=3;
			break;
	}
//	printf( "\ncal %i %i %f\n",ch,b,cal[ch][b]);
	return cal[ch][b];
}


k8047_init(char *dev) {
	if( (hid_fd=open(dev ,O_RDONLY)) <0) {
		fprintf(stderr,"Error: Öffnen der Datei fehlgeschlagen\n");
		exit(1);
	}

	struct hiddev_devinfo device_info;		
	ioctl(hid_fd, HIDIOCGDEVINFO, &device_info);
	if( device_info.vendor!=0x10cf || device_info.product!=0xffff8047) {
		fprintf(stderr,"Error: Produzent 0x%x Gerät 0x%x\n", device_info.vendor, device_info.product);
		exit(1);
	}
	
	rep_info_u.report_type=HID_REPORT_TYPE_OUTPUT;
	rep_info_i.report_type=HID_REPORT_TYPE_INPUT;
	rep_info_u.report_id=rep_info_i.report_id=HID_REPORT_ID_FIRST;
	rep_info_u.num_fields=rep_info_i.num_fields=1;

	ref_multi_u.uref.report_type=HID_REPORT_TYPE_OUTPUT;
	ref_multi_i.uref.report_type=HID_REPORT_TYPE_INPUT;
	ref_multi_u.uref.report_id=ref_multi_i.uref.report_id=HID_REPORT_ID_FIRST;
	ref_multi_u.uref.field_index=ref_multi_i.uref.field_index=0;
	ref_multi_u.uref.usage_index=ref_multi_i.uref.usage_index=0;
	ref_multi_u.num_values=ref_multi_i.num_values=8;

	int a,b;
	for(a=0; a<4; a++) range[a]=30;

	for(a=0; a<4; a++) 
		for(b=0; b<4; b++) cal[a][b]=1;

}

//	3V 6V 15V 30V 
//schreiben
k8047_write(int led, int c1, int c2, int c3, int c4) {
	char b1, b2;
	int err=0;

	//eingangs controlle
	if (!(c1 == 3 || c1 == 6 || c1 == 15 || c1 == 30)) {
		c1=30;
		err=-1;
	}
	range[0]=c1;

	if (!(c2 == 3 || c2 == 6 || c2 == 15 || c2 == 30)) {
		c2=30;
		err=-1;
	}
	range[1]=c2;

	if (!(c3 == 3 || c3 == 6 || c3 == 15 || c3 == 30)) {
		c3=30;
		err=-1;
	}
	range[2]=c3;

	if (!(c4 == 3 || c4 == 6 || c4 == 15 || c4 == 30)) {
		c4=30;
		err=-1;
	}
	range[3]=c4;

	// bit drehen
	union Treg r; 

	unsigned short m;
	r.w=0;

	// Record LED schalten
	m= 1 << 7;
	r.w= (led!=0) ?  r.w | m : r.w & ~m; 
	
	int r0[4] = {0,3,8,14};
	int r1[4] = {1,4,9,15};
	int r2[4] = {2,5,10,6};
	int a;
	for (a=0; a < 4; a++) {
		if(range[a] == 30) {
			r.w=r.w |(1 << r0[a]);
			r.w=r.w |(1 << r1[a]);
			r.w=r.w |(1 << r2[a]);
		}
		if(range[a] == 15) {
			r.w=r.w |(1 << r1[a]);
			r.w=r.w |(1 << r2[a]);
		}
		// ??
		if(range[a] == 6) {
			r.w=r.w |(1 << r2[a]);
			r.w=r.w |(1 << r0[a]);
		}
		if(range[a] == 3) {
			r.w=r.w |(1 << r1[a]);
			r.w=r.w |(1 << r0[a]);
		}
	}

	/*
	m=r.w;
	char ch;
	for(a=0; a < 16; a++) {
		if ( a % 4 == 0) printf(" ");

		ch= ((m &1) ==1 ) ? '1' : '0';
		printf("%c",ch);

		m = m >> 1;
	}
	printf("\n");

	printf("w: 0x%04x  b1: 0x%02x  b2: 0x%02x\n",r.w ,r.c.b1, r.c.b2);
	*/

	//und schreiben
	ref_multi_u.values[0]=0x05;
	ref_multi_u.values[1]=r.c.b1;
	ref_multi_u.values[2]=r.c.b2;

	ioctl(hid_fd,HIDIOCSUSAGES, &ref_multi_u);
	ioctl(hid_fd,HIDIOCSREPORT, &rep_info_u);

	usleep(500);

	// fertig
	return err;
}

//lesen
k8047_read(int *c, float *c1, float *c2, float *c3, float *c4) {
	int i;
	ioctl(hid_fd,HIDIOCGREPORT, &rep_info_i);
	ioctl(hid_fd,HIDIOCGUSAGES, &ref_multi_i); 

	/*
	printf("<- ");
	for(i=0;i<8;i++) printf("%02X ",ref_multi_i.values[i]);
	printf("\n");
	*/

	float f;
	union Treg r; 
	
	r.c.b1=ref_multi_i.values[0];
	r.c.b2=ref_multi_i.values[1];

	*c=r.w;
//	printf("%i  ",r.w);
	for(i=0; i<4 ; i++) {
		float f2;
		f2=k8047_cal(i,range[i]);
		f= range[i]*ref_multi_i.values[i+2];
		f= f/255*f2;
		//printf("c%i: %7.3f ",i+1,f);
//		printf("%7.3f %i %i ",f2,i,range[i]);
		switch(i){
			case 0:
				*c1=f;
				break;
			case 1:
				*c2=f;
				break;
			case 2:
				*c3=f;
				break;
			case 3:
				*c4=f;
				break;
		}
	}
//	printf("<-cal\n");
}

// Grundinfos Ausgeben
k8047_head(char *str) {
	int a;
	if (str!=NULL) if (strlen(str)>0) {
		printf("# %79s\n",str);
	}

	time_t now;
	now=time(NULL);
	printf("# %s",asctime(localtime(&now)));

	printf("# ranges ");
	for(a=0; a<4; a++) printf("%i ",range[a]);
	printf("\n");

}
