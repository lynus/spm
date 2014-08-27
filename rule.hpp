#ifndef RULE_H
#define RULE_H
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <fstream>
typedef unsigned int bnr_t;
typedef std::list<bnr_t> seq;
typedef int event_indx;
typedef std::vector<std::pair<event_indx, event_indx>> occurs;
typedef std::set<seq> seqcache;
class event;
typedef std::vector<event> events_t;
class timestamp {
public:
	float t1s,t1e,t2s,t2e; // antecedent/seccedent start stop time
	int i1s,i1e,i2s,i2e; // antecedent/seccedent start/stop item's index
	timestamp(float _t1s, float _t1e, float _t2s, float _t2e, int *index)
	{
		t1s= _t1s;  t1e= _t1e;
		t2s= _t2s;  t2e= _t2e;
		i1s= index[0];	i1e= index[1];
		i2s= index[2];	i2e= index[3];
	};
	timestamp(){
		t1s=t1e=t2s=t2e=0.0;
		i1s=i1e=i2s=i2e= -1;
	};
private:
	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive &ar, const unsigned int version)
	{
		ar & t1s;
		ar & t1e;
		ar & t2s;
		ar & t2e;
		ar & i1s;
		ar & i1e;
		ar & i2s;
		ar & i2e;
	};
};
typedef std::list<timestamp> timestamps;
class _rule {
public:
	_rule(const seq &an, const seq &se);
	_rule();
	bool is_ante_sorted;
	bool is_secc_sorted;
	void seq_sort(seq &s,int part);
	bool operator< (const _rule &r) const;
	inline const seq& get_ante()const { return ante;}
	inline const seq& get_secc()const { return secc;}
protected:
	seq ante;
	seq secc;
};
class rule_set;
class rule:public _rule {
public:
	rule(): _rule(),supp(0),conf(0.0f){};
	rule(const seq &an, const seq &se): _rule(an,se),supp(0),conf(0.0f) {};
	void assign(const seq &an, const seq &se);
	inline 	int get_supp() { return supp;};
	inline  float get_conf() {return conf;};
	inline void set_conf(float _conf) {conf = _conf;};
	void push_occur(float _t1s, float _t1e, float _t2s, float _t2e, int *index); 
	void push_occur(event_indx t1s, event_indx t1e,timestamp ts, int part);
	inline void get_seq(seq &an, seq &se) { an = ante; se = secc;};
	bnr_t get_max_bnr(int part);
	void merge(const rule &r);
	friend int left_expand(rule &r, rule_set &rs, occurs &occ);
	friend int right_expand(rule &r, rule_set &rs, std::vector<seq> &seccs);
	enum {ANTE=1, SECC};
protected:
	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive &ar, const unsigned int version)
	{
		ar & ts;	
		ar & conf;
		ar & supp;
		ar & ante & secc;
		ar & is_ante_sorted & is_secc_sorted;
	}
	friend bool compare_by_supp(const rule &r1, const rule &r2);
	friend bool compare_by_ante(const rule &r1, const rule &r2);
	timestamps ts;
	float conf;
	int supp;
};

class event {
public:
	event_indx index;
	float occur;
	bnr_t bnr;
};

class rule_set {
public:
	rule_set();
	~rule_set();
	int get_rule_index(const seq &ance, const seq &secc);
	int get_rule_index(const rule &r);
	rule &get_rule(int index);
	void push_rule(const rule &r);
	int merge_rule(const rule &r);
	void filter(int nr_events,int cntr);
	long filter_save(int nr_events, int cntr);
	int restore(long nr_rules);
	inline int nr_rules() { return set->size(); };
	friend void dump(const rule_set &rs, bool detail);
	void sort_by_supp();
	void sort_by_ante();
	int next_rule(std::vector<rule>::iterator &it);
	int pop_rule(std::vector<rule>::reverse_iterator &it);
	inline std::vector<rule>::iterator get_first_iter() {return set->begin();}
	inline std::vector<rule>::reverse_iterator get_last_iter() {return set->rbegin();}
private:
	std::vector<rule> *set;
	std::map<_rule, int> index;
};

class rule_map {
public:
	rule_map(rule_set &rs): nr_origin_rules(0) {
		seq ante;
		auto it = rs.get_first_iter();
		do {
			nr_origin_rules++;
			ante = it->get_ante();
			seq_map[ante].push_back(it->get_secc().front());
		}while (rs.next_rule(it) != -1);
	};
	rule_map():nr_origin_rules(0) {};
	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive &ar, const unsigned int version) {
		ar & seq_map;
		ar & nr_origin_rules;
	}

	void save(const char *fname) { 
		std::ofstream ofs(fname);
		boost::archive::text_oarchive oa(ofs);
		oa<< *this;
	}
	void load(const char *fname) {
		std::ifstream ifs(fname);
		boost::archive::text_iarchive ia(ifs);
		ia>> *this;
	}
	void load_lookup() {
		for(auto it = seq_map.begin(); it!=seq_map.end(); it++) 
			for(auto ait = it->first.begin(); ait!=it->first.end(); ait++) 
				secc_to_ante.insert(std::pair<bnr_t, bnr_t>(it->second.front(),*ait));
	}
	
	bool lookup_ante( bnr_t secc, bnr_t ante) {
		std::pair<std::multimap<bnr_t,bnr_t>::iterator,
				std::multimap<bnr_t,bnr_t>::iterator> ret;
		ret = secc_to_ante.equal_range(secc);
		assert(ret.first != secc_to_ante.end());
		for (auto it = ret.first; it != ret.second; it++) 
			if ( it->second == ante) 
				return true;
		return false;
	}


	void dump(bool detail = true) {
		if (detail) {
			std::cout<<"==============\nstart prefectch map\n===============\n";
			for (auto it = seq_map.begin();it != seq_map.end(); it++) {
				for (auto ait = it->first.begin(); ait != it->first.end();ait++)
					std::cout<< *ait<<' ';
				std::cout<<"==> ";
				for (auto ait = it->second.begin(); ait != it->second.end();ait++)
					std::cout<< *ait<<' ';
				std::cout<<std::endl;
			}
		}
		std::cout<<"nr of prefetch map "<<seq_map.size()<<std::endl;
	}
protected:
	std::map<seq, seq> seq_map;
	std::multimap<bnr_t, bnr_t> secc_to_ante;
	int nr_origin_rules;
};
#endif
