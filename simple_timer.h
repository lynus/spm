#ifndef SIMPLE_TIMER
#define SIMPLE_TIMER
#include <sys/time.h>
#define SW_INIT()	\
	struct timeval sw_cur_tp;	
#define SW_MARK(tp)	\
	struct timeval sw_##tp;	\
	gettimeofday(&sw_##tp, NULL);	
	
#define SW_PASSED(tv,tp_begin, tp_end)	\
	double tv;	\
	tv = sw_##tp_end.tv_sec - sw_##tp_begin.tv_sec	\
		+(double)(sw_##tp_end.tv_usec - sw_##tp_begin.tv_usec)/1000000;

#endif
