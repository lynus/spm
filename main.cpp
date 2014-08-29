#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <unistd.h>
#include <string.h>
#include "rule.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "simple_timer.h"
using namespace std;
config *config::instance = NULL;
events_t	events;
float conf_vec[] = {0.4f, 0.5f, 0.6f, 0.7f, 0.8f};
rule_map *lookup;
//float conf_vec[] = {0.2f,0.3f,0.4f,0.5f,0.6f};
/* 参数occ记录了伴随当前rule的前件序列在event list出现的位置
 * 它用来计算conf,以及用来在生成新前件序列时候生成新的occ用于
 * 下一层的扩展
 */
int left_expand(rule &r, rule_set &rs, occurs &occ)
{
	map<bnr_t, int> map_times;
	map<bnr_t, rule> map_rule;
	map<bnr_t, set<pair<int,int>>> map_occ;
	if (CONFIG(verbose) )
	{
		seq s = r.get_ante();	
		cerr<<"left-expand ";
		for(auto it = s.begin(); it!=s.end(); it++)
			cerr<< *it<<"  ";
		cerr<<"==>";
		s = r.get_secc();
		for(auto it = s.begin(); it!=s.end(); it++)
			cerr<< *it;
		cerr<<"  "<<r.get_supp()<<"  "<<r.get_conf()<<'\n';
	}

	for(auto time_it = r.ts.begin(); time_it != r.ts.end(); time_it++) {
		vector<bnr_t> bnrs;
		occurs  tmp_occurs;
		tmp_occurs.clear();
		bnrs.clear();
		/*根据time_it指向的(Ts, Te),在[Te-Tw, min(Ts-Tw,T2s)]范围内找到所有
		 *大于当前rule r前件的最大的bnr的bnr(并且没有出现在sc),将其放入bnrs，
		 *同时将bnr对应的更新过的(Ts, Te)放入tmp_occurs中
		 *将seq保存入sc,避免后来又重复计算
		 */
		get_bnrs_by_occur(*time_it, r, bnrs, tmp_occurs, rule::ANTE);
		assert(bnrs.size() == tmp_occurs.size() );
		auto bit = bnrs.begin();
		auto oit = tmp_occurs.begin();
		event_indx index[4]= { -1, -1, time_it->i2s, time_it->i2e};
		timestamp t(0.0f,0.0f, time_it->t2s, time_it->t2e, index);
		for(; bit != bnrs.end(); bit++, oit++) {
		/* 将time_it附近找到的bnr遍历一遍，
		 * 将更新bnr出现次数, 更新bnr对应的新rule
		 */
			if (map_occ[*bit].find(*oit) == map_occ[*bit].end()) {
				map_occ[*bit].insert(*oit);
				map_times[*bit]++;
				auto rit = map_rule.find(*bit);
				if (rit != map_rule.end()) {
					rit->second.push_occur(oit->first, oit->second, t, rule::ANTE);
				}else {
					rule *pr;
					pr = make_rule(r,*bit,rule::ANTE);
					pr->push_occur(oit->first, oit->second, t, rule::ANTE);
					map_rule[*bit] = *pr;
					delete pr;
				}
			}
		}
	}
	int flag = -1;
	/*遍历所有新找到的bnr,将所有符合supp>abs_min_supp和conf<min_conf要求的rule进行更深入扩展

	 *若没有bnr符合条件,则返回-1,说明当前rule是dead end
	 *若下层left_expand返回-1,保存当前rule
	 */
	for(auto rit = map_rule.begin(); rit != map_rule.end(); rit++) {
		if (rit->second.get_supp() >= CONFIG(abs_min_supp)) {
			occurs _occ;
			//occ 是待扩展rule前件序列在event list 所有的出现记录
			//需要从occ中找到所有出现过新bnr的出现记录
			_occ.clear();
			get_new_occurs(occ, _occ, rit->first);
			float conf = (float)rit->second.get_supp()/_occ.size();
			rit->second.conf = conf;
			if (conf >= 0.85f){
				rs.push_rule(rit->second);
				continue;
			}
			if (conf >= conf_vec[rit->second.get_ante().size()-1] /*CONFIG(min_conf)*/ 
					&& rit->second.get_ante().size() <= 5) {
				left_expand(rit->second, rs, _occ);
			}
			if (conf >= CONFIG(min_conf))
				rs.push_rule(rit->second);
		}
	}
	return flag;
}

/*
int right_expand(rule &r, rule_set &rs, vector<seq> &seccs)
{
	map<bnr_t,rule> map_rule;
	float win = CONFIG(Tw);
	CONFIG(Tw) = CONFIG(Tsw);
	if (CONFIG(verbose) == 1)
	{
		seq s = r.get_ante();	
		cerr<<"right-expand ";
		for(auto it = s.begin(); it!=s.end(); it++)
			cerr<< *it<<"  ";
		cerr<<"==>";
		s = r.get_secc();
		for(auto it = s.begin(); it!=s.end(); it++)
			cerr<< *it<< "  ";
		cerr<<"  "<<r.get_supp()<<"  "<<r.get_conf()<<'\n';
	}
	for (auto time_it = r.ts.begin(); time_it != r.ts.end(); time_it++) {
		vector<bnr_t> bnrs;
		occurs tmp_occurs;
		tmp_occurs.clear();
		bnrs.clear();
		get_bnrs_by_occur(*time_it, r.get_secc(), r.get_max_bnr(rule::SECC),
				bnrs, tmp_occurs, rule::SECC);
		assert(bnrs.size() == tmp_occurs.size() );
		auto bit = bnrs.begin();
		auto oit = tmp_occurs.begin();
		event_indx index[4] = { time_it->i1s, time_it->i1e, -1, -1};
		timestamp t(time_it->t1s, time_it->t1e, 0.0f, 0.0f, index);
		for(; bit != bnrs.end(); bit++, oit++) {
			auto rit = map_rule.find(*bit);
			if (rit != map_rule.end()) {
				rit->second.push_occur(oit->first, oit->second, t, rule::SECC);
			} else {
				rule *pr;
				pr = make_rule(r, *bit, rule::SECC);
				pr->push_occur(oit->first, oit->second, t, rule::SECC);
				map_rule[*bit] = *pr;
				delete pr;
			}
		}
	}
	int flag = 0;
	for(auto rit = map_rule.begin(); rit != map_rule.end(); rit++) {
		if (rit->second.get_supp() >= CONFIG(abs_min_supp)) {
			float conf = r.get_conf() * rit->second.get_supp() / r.get_supp();
			rit->second.conf = conf;
			if (contained(rit->second.get_secc(), seccs))
				continue;
			if (conf >= 0.6f ) {
				right_expand(rit->second, rs, seccs);
				if (!contained(rit->second.get_secc(),seccs))
					rs.push_rule(rit->second);
			}
			if (!contained(rit->second.get_secc(),seccs))
				seccs.push_back(rit->second.get_secc());
		}
	}
	CONFIG(Tw) = win;
	return flag;
}
*/
long getTwoItemRule_seg(rule_set &rs, events_t::iterator begin, events_t::iterator end, events_t::iterator last)
{
	rule_set __rs;
	seq ance,secc;
	set<bnr_t> bnr_set;
	int rule_ind;
	rule r;
	int indx[4];
	for (events_t::iterator eit = begin ; eit != end; eit++) {
		bnr_set.clear();
		ance.clear();
		ance.push_back((*eit).bnr);
		vector<event>::iterator win_start_it, win_end_it;
		get_window_range(eit, end, win_start_it, win_end_it);
		assert( win_start_it->occur>=eit->occur||win_start_it == win_end_it);
		assert( (win_end_it-1)->occur-eit->occur <= CONFIG(Tl_max)||win_start_it == win_end_it);
		for (vector<event>::iterator it=win_start_it;
				it != win_end_it; it++) {
			secc.clear();
			if( it->bnr != eit->bnr && bnr_set.find(it->bnr) == bnr_set.end()) {
				secc.push_back((*it).bnr);
				bnr_set.insert(it->bnr);
				indx[0] = eit->index; indx[1] = indx[0];
				indx[2] = it->index;  indx[3] = indx[2];
				rule_ind = __rs.get_rule_index(ance,secc);
				if (rule_ind != NOT_FOUND) {
					__rs.get_rule(rule_ind).push_occur((*eit).occur,(*eit).occur,	
					(*it).occur, (*it).occur, indx );
				}else {
					r.assign(ance,secc);
					r.push_occur((*eit).occur,(*eit).occur, (*it).occur,
							(*it).occur, indx );
					__rs.push_rule(r);
				}
			}
		}
	}
	/* filter out infrequent rules 
	 filter_save(threshold, control_value) */
	long nr_rules_left, nr_rules_restored;
	nr_rules_left = __rs.filter_save(CONFIG(abs_min_supp), 2);
	nr_rules_restored = rs.restore(nr_rules_left);
	cerr<<"saved "<<nr_rules_restored<<" rules"<<endl;
	//dump(rs);	
	return nr_rules_restored;
}
void show_usage(const char *s)
{
	cerr<<"usage: "<<s<<" -t trace_file [-r rule_map_file]"<<endl;
	exit(-1);
}
int main(int argc ,char *argv[]) 
{
	rule_set rs;
	config theconfig;
	long left,saved=0;
	events_t::iterator begin, end;
	SW_MARK(start);
	char *trace_file=NULL;
	int opt;
	while((opt= getopt(argc,argv, "r:t:")) != -1) {
		switch (opt) {
			case 'r':
				CONFIG(rule_map) = string(optarg);
				break;
			case 't':
				trace_file = (char *)malloc(sizeof(char)*32);
				strncpy(trace_file, optarg, 32);
				break;
			case '?':
				show_usage(argv[0]);
				break;
		}
	}
	if (trace_file==NULL){
		cerr<<"not specify trace file"<<endl;
		show_usage(argv[0]);
	}
	load_data(trace_file,events);
	{
		fstream fout("/tmp/rule_name",ios::out);
		fout<<CONFIG(rule_map)<<endl;
		fout.close();
	}
	begin = events.begin();
	left = events.size();
	long total = events.size();
	while(left) {
		if (left<CONFIG(seg_len)){ 
			end = begin + left;
			left = 0;
		}
		else {
			end = begin + CONFIG(seg_len);
			left -= CONFIG(seg_len);
		}
		saved += getTwoItemRule_seg(rs,begin,end, events.end());
		begin = end;
		double progress = 1 - (double)left/total;
		cerr<< "processed "<< progress*100<<'%'<<"\033[A\033[100D";
	}
	SW_MARK(twoitem);
	//rs.sort_by_supp();
	rs.sort_by_ante();
	lookup = new rule_map(rs);
	lookup->load_lookup();
	//dump(rs);
	cerr<<"total saved rules: "<<saved<<endl;
	cerr<<"====================="<<endl;
	cerr<<"start left expand"<<endl;
	rule_set left_rs;
	auto  rit = rs.get_last_iter();
	do {
		seq ante;
		occurs occ;
		if (ante != rit->get_ante()) {
			ante = rit->get_ante();
			occ.clear();
			init_occ(occ,ante.front());
		}
		rit->set_conf((float)rit->get_supp()/occ.size());
		if (rit->get_conf() < CONFIG(min_conf))
			continue;
		if (rit->get_conf() >= 0.85f) {
			left_rs.push_rule(*rit);
			continue;
		}
		left_expand(*rit, left_rs, occ);
	}while (rs.pop_rule(rit) != -1);
	SW_MARK(leftexp);
	left_rs.sort_by_ante();

	dump(left_rs,false);
	rule_map pm(left_rs);
	pm.dump(false);
	pm.save(CONFIG(rule_map).c_str());
	SW_PASSED(tv_twoitem, start, twoitem);
	SW_PASSED(tv_leftexp,twoitem, leftexp);
	cerr<<"mining two-item used "<<tv_twoitem<<" seconds"<<endl;
	cerr<<"left-expand used "<<tv_leftexp<<" seconds"<<endl;
 }
