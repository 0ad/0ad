/**
 * Returns a pair of independent and normally distributed (zero mean, variance 1) random numbers.
 * Uses the Polar-Rejection method.
 */
function randomNormal2D()
{
	let s, a, b;
	do
	{
		a = 2 * Math.random() - 1;
		b = 2 * Math.random() - 1;
		s = a * a + b * b;
	} while (s>=1 || s==0)
	s = Math.sqrt(-2 * Math.log(s) / s);
	return [a * s, b * s];
}
