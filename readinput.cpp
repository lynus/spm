#include <fstream>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#include <cerrno>
using namespace std;
void getinfo(char *buf, float *ts, unsigned long *bnr, int *size)
{
	char *plus,*equ;
	plus = strchr(buf,'+');
	if (*(plus+1) =='?')
		*ts = 0.0f;
	else
	sscanf(plus+1,"%f",ts);
	equ = strchr(buf,'=');
	equ = strchr(equ+1,'=');
	sscanf(equ+1,"%lu",bnr);
	equ = strchr(equ+1,'=');
	sscanf(equ+1,"%d",size);
}
	
int main(int argc,char **argv)
{
	int size,line=0,max_line=0;
	unsigned long bnr;
	float ts=0.0,delta;
	fstream fin;
	char buf[256];
	fin.open(argv[1],ios::in);
	if (!fin.is_open()) {
		cout<<strerror(errno)<<endl;
		exit(-1);
	}
	if ( argc == 3 ) 
		max_line = atoi(argv[2]);

	while (!fin.eof()) {
		fin.getline(buf,256);
		if (strlen(buf) == 0) 
			break;
		line++;
		getinfo(buf, &delta, &bnr,&size);
		ts+=delta;
//		printf("%d\tts: %.9f\tbnr: %d\tsize: %d\n",line, ts, bnr, size);
		if (bnr >(unsigned long)(1UL<<35)) {
			line--;
			continue;
		}
		printf("%.9f\t%lu\t%d\n",ts,bnr,size);
		if (max_line && max_line == line)
			break;
	}

	fprintf(stderr,"%d lines saves.\n",line);
	return 0;
}

