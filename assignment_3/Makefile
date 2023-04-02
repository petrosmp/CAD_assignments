all: str_util netlist gate_test trim_test subsys_test search_test

str_util: str_util.h str_util.c
	gcc -Wall -shared -fpic -o libstr.so $(word 2,$^) -g

netlist: netlist.h netlist.c libstr.so
	gcc -Wall -shared -fpic -o libnetlist.so -L. -Wl,-rpath=. $(word 2,$^) -lstr -g

gate_test: gate_test.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lstr -lnetlist -g

trim_test: trim_test.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lstr -g

subsys_test: subsys_test.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lstr -lnetlist -g

search_test: search_test.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lstr -lnetlist -g

clean:
	rm -f libstr.so
	rm -f libnetlist.so
	rm -f gate_test
	rm -f netlist.txt
	rm -f gate_test
	rm -f trim_test
	rm -f subsys_test
	rm -f search_test
