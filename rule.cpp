#include <list>
#include <assert.h>
#include <vector>
#include <map>
#include <fstream>
#include <algorithm>
#include "rule.hpp"
#include "config.hpp"
using namespace std;
using namespace boost::archive;

bool compare_by_supp(const rule &r1, const rule &r2) {
	return r1.supp < r2.supp;
}

int seq_compare(const seq &s1, const seq &s2)
{
	/* assume s1 s2 is sorted, apply lexicograhical compare */
	seq::const_iterator it1 = s1.begin();
	seq::const_iterator it2 = s2.begin();
	while ( it1 != s1.end() && it2 != s2.end() ) {
		if ( *it1 < *it2 ) {
			return -1;
		}else if( *it1 > *it2) {
			return 1;
		}else {
			it1++;
			it2++;
		}
	}
	if (it1 == s1.end() && it2 == s2.end())
		return 0;
	if (it1 != s1.end())
		return 1;
	return -1;
}
bool compare_by_ante(const rule &r1, const rule &r2) {
	int ret = seq_compare(r1.ante, r2.ante);
	if (ret <0 ) 
		return true;
	else
		return false;
}
_rule::_rule(const seq &an, const seq &se)
{
	is_ante_sorted = true;
	is_secc_sorted = true;
	/* maybe assigning value cost too much, need to profile */
	ante= an;
	secc= se;
//	ante.sort();
//	secc.sort();
}
 _rule::_rule()
{
	ante= seq();
	secc= seq();
	is_ante_sorted = false;
	is_secc_sorted = false;
}

void _rule::seq_sort(seq &s,int part)
{
	if (part == 1) //sort ancedant seq
		if (is_ante_sorted == true)
			return;
		else {
			s.sort();
			is_ante_sorted = true;
		}
	else if (is_secc_sorted == true)
		return;
	else {
		s.sort();
		is_secc_sorted = true;
	}
}

void rule::push_occur(float _t1s, float _t1e, float _t2s, float _t2e, int *index)
{
	timestamp t(_t1s,_t1e,_t2s,_t2e, index);
	ts.push_back(t);
	supp++;
}

void rule::push_occur(event_indx start, event_indx end, timestamp ts, int part)
{
	extern events_t events;
	int index[4];
	if (part == ANTE) {
		index[0] = start; index[1] = end;
		index[2] = ts.i2s; index[3] = ts.i2e; 
		push_occur(events[start].occur, events[end].occur,
				ts.t2s, ts.t2e, index);
	}else {
		index[0] = ts.i1s; index[1] = ts.i1e;
		index[2] = start; index[3] = end;
		push_occur(ts.t1s, ts.t1e, events[start].occur, 
				events[end].occur, index);
	}
}

	

void rule::assign(const seq &an, const seq &se)
{
	ante = an;
	secc = se;
	ante.sort();
	secc.sort();
	is_ante_sorted= is_secc_sorted = true;
	ts.clear();
	supp = 0;
	conf = 0.0;
}

void rule::merge(const rule &r)
{
	this->supp += r.supp;
	for(auto it = r.ts.begin(); it != r.ts.end(); it++) 
		ts.push_back(*it);
}


bnr_t rule::get_max_bnr(int part)
{
	if (part == ANTE) {
		assert(is_ante_sorted);
		return ante.back();
	}
	else {
		assert(is_secc_sorted);
		return secc.back();
	}
}
rule_set::rule_set()
{
	set = new vector<rule>();
	index = map<_rule, int>();
}
rule_set::~rule_set()
{
	delete set;
}
bool _rule::operator< (const _rule &r) const
{
		int t;
		assert(is_ante_sorted);
		assert(is_secc_sorted);
		//turns out override operator can access private field
		if ( (t= seq_compare(this->ante, r.ante)) == -1) 
			return true;
		if (t == 1)
			return false;
		if ( seq_compare(this->secc, r.secc)== -1)
			return true;
		else
			return false;
}

int rule_set::get_rule_index(const seq &ance, const seq &secc)
{
	_rule r(ance,secc);
	map<_rule,int>::iterator it = index.find(r);
	if (it == index.end() ) 
		return NOT_FOUND;
	else 
		return it->second;
}

int rule_set::get_rule_index(const rule &r)
{
	return get_rule_index(r.get_ante(), r.get_secc());
}


rule & rule_set::get_rule(int index)
{
	return (*set)[index];
}
void rule_set::push_rule(const rule &r)
{
	set->push_back(r);
	int ind = set->size()-1;
	index[r] = ind;
}
int rule_set::merge_rule(const rule &r)
{
	int rule_inx;
	if ( (rule_inx = get_rule_index(r)) == NOT_FOUND) {
		push_rule(r);
		return 1;
	}
	else {
		get_rule(rule_inx).merge(r);
		return 0;
	}
}
void rule_set::filter(int nr_events,int cntr=1)
{
	/* set is sequential container so erase() is not effective */
	vector<rule> *_rs = new vector<rule>();
	vector<rule>::iterator it = this->set->begin();
	int thre;
	rule _r;
	seq an,se;
	map<_rule,int>::iterator key;
	if (cntr == 1) thre = CONFIG(min_supp) * nr_events;
	if (cntr == 2) thre = nr_events;
	for( ; it != this->set->end(); it++) {
		if ( (*it).get_supp() >= thre ){
			_rs->push_back(*it);
			it->get_seq(an,se);
			_r.assign(an,se);
			assert((key=index.find(_r)) != index.end());
			index.erase(key);
		}
	}
	delete set;
	set = _rs;
}

long rule_set::filter_save(int nr_events, int cntr=1)
	//some time there are too many rules in rule set
	//I have to filter the rules into disk by serializing
	//and restore rule_set by reading the disk data
{
	ofstream ofs(CONFIG(rule_file));
	text_oarchive oa(ofs);
	int thre;
	long ret = 0;
	if (cntr == 1) thre = CONFIG(min_supp) * nr_events;
	if (cntr == 2) thre = nr_events;
	for(vector<rule>::iterator it=set->begin();it != set->end();it++) {
		if ( it->get_supp() >= thre) {
			oa<< *it;
			ret++;
		}
	}
	return ret;
}
int rule_set::restore(long nr_rules)
{
	ifstream ifs(CONFIG(rule_file));
	text_iarchive ia(ifs);
	rule _r;
	int nr_restore = 0;
//	set->clear();
//	index.clear();
	for (int i=1; i<=nr_rules; i++) {
		ia>>_r;
		nr_restore += merge_rule(_r);
	}
	return nr_restore;
}

void rule_set::sort_by_supp()
{
	int i = 0;
	index.clear();
	sort(set->begin(), set->end(), compare_by_supp);
	for(auto it = set->begin(); it != set->end(); it++, i++) 
		index[*it] = i;
}

void rule_set::sort_by_ante()
{
	int i = 0;
	index.clear();
	sort(set->begin(), set->end(), compare_by_ante);
	for(auto it = set->begin(); it != set->end(); it++, i++) 
		index[*it] = i;
}
int rule_set::next_rule(vector<rule>::iterator &it)
{
	it++;
	if (it == set->end())
		return -1;
	return 0;
}
int rule_set::pop_rule(vector<rule>::reverse_iterator &it)
{
	it++;
	set->pop_back();
	if (it == set->rend())
		return -1;
	return 0;
}

