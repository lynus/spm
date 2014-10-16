#include "rule.hpp"
#include "utils.hpp"
#include <unistd.h>
#include <string.h>
using namespace std;
int main(int argc, char *argv[])
{
	int opt;
	char *save_file = NULL;
	char *list_file = NULL;
	while((opt=getopt(argc,argv,"o:l:")) != -1) {
		switch (opt) {
		case 'o':
			save_file=(char *)malloc(sizeof(char)*255);
			strncpy(save_file, optarg, 255);
			break;
		case 'l':
			list_file=(char *)malloc(sizeof(char)*255);
			strncpy(list_file, optarg, 255);
			break;
		case '?':
			cerr<<"unknown options"<<endl;
			cerr<<argv[0]<<" -o save_rule -l rule_list"<<endl;
			exit(-1);
		}
	}
	if (save_file == NULL || list_file== NULL) {
		cerr<<argv[0]<<" -o save_rule -l rule_list"<<endl;
		exit(-1);
	}
	rule_map rm;
	string line;
	ifstream in(list_file);
	while(!getline(in, line).eof()) {
		cout<<"merging "<<line<<"...";
		rule_map r(line.c_str());
		rm+=r;
		cout<<endl<<rm.get_nr_rule()<<endl;
	}
	rm.dump(false);
	rm.save(save_file);
}



