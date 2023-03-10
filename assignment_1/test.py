import platform
import subprocess

polynomials = [
    [8, 3, 6, 2, 0, 12],                    # root = -1
    [3, -124, 439, 1234, -10439, 931678],   # root ~ 37
    [1, -42, 861, -9534, 60696, -149688],   # root ~ 5 
    [1, -42, 1, -40, -78, -246],            # root = 42
    [1, -1337, 1, -1335, -2668, -8023]      # root = 1337
]
delta = 0.001
x0s = [0, 1, 10000000]
iters = [20, 100000]
executable = "./find_roots.exe" if platform.system() == "Windows" else "./find_roots"

for polynomial in polynomials:
    for x0 in x0s:
        for iter in iters:
            print(f"p{polynomials.index(polynomial)+1}, x0={x0}, max_iters={iter}")
            args = f"{executable} {' '.join([str(x) for x  in polynomial])} -d {delta} -i {iter} -x {x0}".split()
            popen = subprocess.Popen(args, stdout=subprocess.PIPE)
            popen.wait()
            output = popen.stdout.read().decode()
            print(output)
    print("==============================================================================")
