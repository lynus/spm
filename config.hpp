#ifndef CONFIG_H
#include <string>
#define CONFIG(data)	(config::get()->data)
#define REGISTER_INT(field)	field = atoi(option[string(#field)].c_str());
#define REGISTER_STRING(field)	field = option[string(#field)];
#define REGISTER_FLOAT(field)	field = atof(option[string(#field)].c_str());
#define REGISTER(field, type)	field = static_cast<type>(atoi(option[string(#field)].c_str()));

#define TL_MIN 0.0
#define TL_MAX 0.01

#define FAILED (-1)
#define NOT_FOUND (-1)
class config {
public:
	config();
	void setup();
	static config* get() {return instance;};
	float Tl_min;
	float Tl_max;
	float min_supp;
	float min_conf;
	float Tw;
	float Tsw;
	int abs_min_supp;
	bool verbose;
	//int abs_min_conf
	std::string rule_file;
	int seg_len;
	std::string rule_map;
	static config *instance;
};
#endif
