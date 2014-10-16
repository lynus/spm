#include "config.hpp"
#include <string>
#include <map>
#include <fstream>
#include <cstring>
using namespace std;
string config_file;
static string  trim(const string &s) 
{
	string r;
	 if (s.empty()) {  
		 return r;  
	 }  
	 r = s;
	r.erase(0,r.find_first_not_of(" "));  
	r.erase(r.find_last_not_of(" ") + 1);  
	return r;  
}  

 config::config()
{
	string line,key,value;
	map<string,string> option;
	char buf[256],t[32];
	int pos;
	instance = this;
	if (config_file.size() == 0)
		config_file = string("config.ini");
	ifstream fconfig(config_file.c_str());
	if (!fconfig.is_open())
		goto nofile;
	while(!fconfig.eof()) {
		fconfig.getline(buf,256);
		line = string(buf);
		pos = line.find('=');
		key= trim(line.substr(0,pos));
		if (key[0] == '#')
			continue;
		value = trim(line.substr(pos+1));
		option[key]=value;
	}
	REGISTER_FLOAT(Tl_min);
	REGISTER_FLOAT(Tl_max);
	REGISTER_FLOAT(min_supp);
	REGISTER_FLOAT(min_conf);
	REGISTER_FLOAT(Tw);
	REGISTER_FLOAT(Tsw);
	REGISTER_INT(abs_min_supp);
	REGISTER(verbose, bool);
	REGISTER_INT(seg_len);
	//REGISTER_INT(abs_min_conf);
	REGISTER_STRING(rule_file);
	REGISTER_STRING(rule_map);
	return;
	
nofile:
	Tl_min = 0.0;
	Tl_max = 0.01;
	Tw = 0.1;
	min_supp = 0.001;
	min_conf = 0.1;
	abs_min_supp = 10;
	rule_file = "/tmp/rules.dat";
	seg_len = 10000;

	return;
}
