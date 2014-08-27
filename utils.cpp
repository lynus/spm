#include <fstream>
#include <cerrno>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <list>
#include <map>
#include "rule.hpp"
#include "utils.hpp"
#include "config.hpp"
using namespace std;
extern rule_map *lookup;
#define is_freq(ante,secc)	lookup->lookup_ante(secc,ante)
void load_data(char const* fname, vector<event> &events)
{
	fstream fin(fname,ios::in);
	event t;
	float occ, last_occ = -1.0f;
	int   bnr,inx=0,size;
	if (!fin.is_open()) {
		cout<<strerror(errno)<<endl;
		exit(-1);
	}
	while(!fin.eof()) {
		fin>>occ>>bnr>>size;
		if (!fin) break;
		if (occ <= last_occ) 
			occ = last_occ+0.001f;
		t.index = inx;
		t.occur = occ;
		t.bnr = bnr;
		events.push_back(t);
		inx++;
		last_occ = occ;
	}
}


bool seq_lessthan(seq &s1, seq &s2)
{
	seq::iterator it1 = s1.begin();
	seq::iterator it2 = s2.begin();
	int flag=0;
	while (it1 !=s1.end() && it2 !=s2.end()) {
		if (*it1 == *it2) {
			it1++; 
			it2++;
		}else if (*it1 < *it2)
			flag = 1;
		else
			flag = -1;
		break;
	}
	switch (flag) {
	case 1:
		return true;
	case -1:
		return false;
	default:
		if (it1 == s1.end()&& it2 != s2.end())
			return true;
		else
			return false;
	}
}

void get_window_range(event_it start, event_it end, event_it &win_start, event_it &win_end)
{
	win_start = start+1;
	while (win_start != end && (*win_start).occur- (*start).occur <CONFIG(Tl_min))
		win_start++;
	if (win_start == end){
		win_end = end;
		return;
	}
	win_end = win_start;
	while (win_end != end && ((*win_end).occur- (*start).occur) <=CONFIG(Tl_max))
		win_end++;

}
void dump(const rule_set &rs,bool detail = true)
{
	vector<rule>::iterator it = rs.set->begin();
	seq ance,secc;
	if (detail) {
		while (it != rs.set->end()) {
			it->get_seq(ance,secc);
			for(seq::iterator _it=ance.begin(); _it !=ance.end(); _it++)
				cout<<*_it<<'\t';
			cout<<"==>";
			for(seq::iterator _it=secc.begin(); _it !=secc.end(); _it++)
				cout<<'\t'<<*_it<<'\t';
			cout<<"supp: "<<it->get_supp()<<"   conf: "<<it->get_conf()<<endl;
			it++;
		}
	}
	std::cerr<<"nr of rule set: "<<rs.set->size()<<endl;
}
//查找在event list中bnr出现的位置填充到occ位置上
void init_occ(occurs &occ, bnr_t bnr)
{
	extern events_t events;
	assert(occ.size() ==0);
	for (auto it = events.begin(); it != events.end(); it++) {
		if (it->bnr == bnr)
			occ.push_back(pair<int,int>(it->index, it->index));
	}
}
/*根据ts指向的(Ts, Te),在[Te-Tw, Ts+Tw]范围内找到所有
 *大于max_bnr的bnr(并且没有出现在sc),将其放入bnrs，
 *同时将bnr对应的更新过的(Ts, Te)放入occ中
 *将seq保存入sc,避免后来又重复计算
 */

void get_bnrs_by_occur(timestamp &ts, rule &r, vector<bnr_t> &bnrs,
		occurs &occ, int part)
{
	bnr_t max_bnr = r.get_max_bnr(part);
	extern events_t events;
	float lower,upper;
	set<bnr_t> bnr_set;
	if (part == rule::ANTE) {
		lower = ts.t1e-CONFIG(Tw);
		upper = ts.t1s+CONFIG(Tw);
		upper = MIN(upper, ts.t2s);
	}else {
		lower = ts.t2e-CONFIG(Tw);
		upper = ts.t2s+CONFIG(Tw);
		lower = MAX(lower, ts.t1e);
	}
	event_indx i = part==rule::ANTE?ts.i1e+1 : ts.i2e+1;
	bnr_t s = r.get_secc().front();
	while(i < events.size() && events[i].occur < upper) {
		if (events[i].bnr > max_bnr && bnr_set.find(events[i].bnr) == bnr_set.end()
				&&is_freq(events[i].bnr, s)) {
			bnr_set.insert(events[i].bnr);
			bnrs.push_back(events[i].bnr);
			occ.push_back(pair<int, int>(part==rule::ANTE?ts.i1s:ts.i2s, i));
		}
		i++;
	}
	
	i = part==rule::ANTE?ts.i1s-1 : ts.i2s-1;
	while(i > 0 && events[i].occur > lower) {
		if (events[i].bnr >max_bnr && bnr_set.find(events[i].bnr) == bnr_set.end()
				&&is_freq(events[i].bnr,s)) {
			bnr_set.insert(events[i].bnr);
			bnrs.push_back(events[i].bnr);
			occ.push_back(pair<int,int>(i,part==rule::ANTE?ts.i1e:ts.i2e));
		}
		i--;
	}
}
rule *make_rule(rule &r, bnr_t bnr, int part)
{
	seq s;
	rule *ret;
	if (part == rule::ANTE) {
		s = r.get_ante();
		s.push_back(bnr);
		ret = new rule(s, r.get_secc());
	}
	else {
		s = r.get_secc();
		s.push_back(bnr);
		ret = new rule(r.get_ante(), s);
	}
	return ret;
}
//在old occurs中查找包含有bnr的occur，将其放入new中
void get_new_occurs(occurs &old, occurs &new_occ, bnr_t bnr)
{
	extern events_t events;
	float lower,upper;
	event_indx i;
	set<pair<int,int>> occ_set;
	auto it = old.begin();
	while(it != old.end()) {
		lower = events[it->second].occur - CONFIG(Tw);
		upper = events[it->first].occur + CONFIG(Tw);
		i = it->first;
		while(i <events.size() && events[i].occur < upper) {
			if (events[i].bnr == bnr) {
				if (i <= it->second 
					&& occ_set.find(pair<int,int>(it->first,it->second)) == occ_set.end()) {
					occ_set.insert(pair<int,int>(it->first, it->second));
					new_occ.push_back(pair<int, int>(it->first, it->second));
					goto next;
				}
				else if(occ_set.find(pair<int, int>(it->first, i)) == occ_set.end()) {
					occ_set.insert(pair<int,int>(it->first, i));
					new_occ.push_back(pair<int, int>(it->first, i));
					goto next;
				}
			}
			i++;
		}
		i = it->first;
		while(i >= 0 && events[i].occur > lower) {
			if (events[i].bnr == bnr 
				&& occ_set.find(pair<int,int>(i,it->second)) == occ_set.end()) {
				occ_set.insert(pair<int,int>(i, it->second));
				new_occ.push_back(pair<int, int>(i,it->second));
				goto next;
			}
			i--;
		}
next:		it++;
	}
}

static int _contained(const seq &s, const seq &s1)
{
	auto it1 = s.begin();
	auto it2 = s1.begin();
	while(it1 != s.end() && it2 != s1.end()) {
		if ( *it1 == *it2) {
			it1++; it2++;
		}else if ( *it1 > *it2){
			it2++;
		}else {
			return 0;
		}
	}
	if (it1 == s.end())
		return 1;
	return 0;
}

int contained(const seq &s, const vector<seq> &seccs)
{
	int ret = 0;
	for (auto it = seccs.begin(); it != seccs.end(); it++) {
		if ( _contained(s, *it)){
			ret = 1;
			break;
		}
	}
	return ret;
}
