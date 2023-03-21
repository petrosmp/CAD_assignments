all: str_util netlist custom_full_adder

str_util: str_util.h str_util.c
	gcc -Wall -shared -fpic -o libstr.so $(word 2,$^) -g

netlist: netlist.h netlist.c libstr.so
	gcc -Wall -shared -fpic -o libnetlist.so -L. -Wl,-rpath=. $(word 2,$^) -lstr -g

custom_full_adder: custom_full_adder.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lnetlist -g

clean:
	rm -f libstr.so
	rm -f libnetlist.so
	rm -f custom_full_adder
	rm -f netlist.txt
