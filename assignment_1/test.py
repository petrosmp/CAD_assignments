import platform
import subprocess

polynomials = [
    [8, 3, 6, 2, 0, 12],
    [3, -124, 439, 1234, -10439, 931678],
    [4, -1, -5, 13, -17, 12, 4],
]
deltas = [0.1, 0.001]
x0s = [0, 1, 10000000]
iters = [20, 100, 10000]
executable = "./find_roots.exe" if platform.system() == "Windows" else "./find_roots"

for polynomial in polynomials:
    for delta in deltas:
        for iter in iters:
            for x0 in x0s:
                args = f"{executable} {' '.join([str(x) for x  in polynomial])} -d {delta} -i {iter} -x {x0}".split()
                popen = subprocess.Popen(args, stdout=subprocess.PIPE)
                popen.wait()
                output = popen.stdout.read().decode()
                print(output)
    print("==============================================================================")



