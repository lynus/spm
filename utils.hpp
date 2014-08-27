#ifndef UTIL_H
#define UTIL_H
#define MIN(x,y) (x)<(y)?(x):(y)
#define MAX(x,y) (x)>(y)?(x):(y)
typedef typename std::vector<event>::iterator event_it;
void load_data(char const* fname,std::vector<event> &events);

void get_window_range(event_it start,event_it end,event_it &win_start,event_it &win_end);
void init_occ(occurs &occ, bnr_t bnr); 

void get_bnrs_by_occur(timestamp &ts, rule &r,
		std::vector<bnr_t> &bnrs, occurs &occ, int part);
rule *make_rule(rule &r, bnr_t bnr, int part);

void get_new_occurs(occurs &old, occurs &new_occ, bnr_t bnr);
 
int contained(const seq &, const std::vector<seq> &seccs);
#endif
