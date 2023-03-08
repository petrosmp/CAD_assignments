from polyonym import Polyonym
import sys

# read the coeffs from the command line
coeffs = []
for i in sys.argv[1:]:
    coeffs.append(int(i))

# create the polyonym
p = Polyonym(coeffs)

# solve and plot it (plot() blocks execution)
p.real_roots()
print(p.f(5))
p.plot()
