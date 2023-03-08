import numpy as np
import matplotlib.pyplot as plt

class Polyonym:
    """
        A class representing a polyonym of arbitrary degree
    """

    def __init__(self, coeffs):
        """
            Create a new polyonym object with the given coefficients.
        """

        self.coeffs = coeffs

    def f(self, x):
        """
            Generate a calculatable function with the global coeffs
        """

        r_val = 0
        for i in range(len(self.coeffs)):
            r_val += self.coeffs[i] * np.power(x, len(self.coeffs)-i-1)

        return r_val

    def plot(self, p_range=[-1000, 1000], points=100000):
        """
            Plot the polyonym at the given (or the default) range.
            
            Uses matplotlib's plot function and will therefore block
            execution until the plot window is closed.
        """

        x_values = np.linspace(p_range[0], p_range[1], points)
        plt.plot(x_values, self.f(x_values))

        # Add horizontal line at y=0
        plt.axhline(y=0, color='black', linestyle='--')
        
        plt.show()

    def real_roots(self):
        """
            Find the real root(s) of the polyonym.

            Uses numpy's roots(), real() and imag() functions
        """

        roots = np.roots(self.coeffs)
        real_roots = np.real(roots[np.isclose(np.imag(roots), 0)])
        print(f"Real Roots: {real_roots}")

# only here for testing purposes
if __name__ == "__main__":
    p = Polyonym([3, -124, 439, 1234, 10439, -931678])
    p.real_roots()
    p.plot()
