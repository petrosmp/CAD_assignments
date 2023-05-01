all: str_util netlist get_info

str_util: str_util.h str_util.c
	gcc -Wall -shared -fpic -o libstr.so $(word 2,$^) -g

netlist: netlist.h netlist.c libstr.so
	gcc -Wall -shared -fpic -o libnetlist.so -L. -Wl,-rpath=. $(word 2,$^) -lstr -g

get_info: get_info.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lstr -lnetlist -g

clean:
	rm -f libstr.so
	rm -f libnetlist.so
	rm -f get_info
