all: str_util netlist translate_to_gates

str_util: str_util.h str_util.c
	gcc -Wall -shared -fpic -o libstr.so $(word 2,$^) -g

netlist: netlist.h netlist.c libstr.so
	gcc -Wall -shared -fpic -o libnetlist.so -L. -Wl,-rpath=. $(word 2,$^) -lstr -g

translate_to_gates: translate_to_gates.c
	gcc -Wall -L. -Wl,-rpath=. $< -o $@ -lstr -lnetlist -g

clean:
	rm -f libstr.so
	rm -f libnetlist.so
	rm -f translate_to_gates
	rm -f netlist.txt
	rm -f gates_only.txt
