#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
using namespace std;
int main(int argc, char **argv) 
{
	char *in_trace;
	if (argc <2) {
		cerr<<"need input trace file"<<endl;
		exit(-1);
	}
	in_trace = argv[1];
	ifstream in(in_trace);
	string line;
	int last_bnr = 0, bnr, len;
	float last_ts = 0.0f, ts;
	while(!getline(in,line).eof()) {
		stringstream ss(line);
		ss>>ts>>bnr>>len;	
		cout<<ts<<'\t'<<bnr-last_bnr<<'\t'<<len<<endl;
		last_bnr = bnr;
	}
}



