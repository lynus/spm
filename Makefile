objs= main.o utils.o rule.o config.o
boost_lib_path= ~/boost/lib
boost_inc_path= ~/boost/include
tmp_dir= /tmp
cflag= --std=c++11
ifeq ($(O),1)
cflag+= -O3
else
cflag+= -g
endif
all: $(objs)
	g++ $(cflag) -o mine -g $(objs) -L ${boost_lib_path} -lboost_serialization -static -lpthread

%.o:%.cpp config.hpp
	g++ -c $(cflag) $< -I ${boost_inc_path}   -o $@ 

clean:
	rm mine readtrace *.o cscope.*

clean_tmp:
	rm *.o

count:
	find -name "*cpp" -o -name "*hpp"|xargs wc -l

cscope:
	find -name "*cpp" -o -name "*hpp" >cscope.files
	cscope -q -R -b  -i cscope.files

readtrace: readinput.o
	g++ -o $@ $<
	
