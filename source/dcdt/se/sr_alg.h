
/** \file sr_alg.h 
 * few algebra functions */

# ifndef SR_ALG_H
# define SR_ALG_H

/*! Solve polynomials of 2nd degree. The coefficients are given in c,
    where c[i] is the coefficient of the i-th power of the unknown variable.
	The roots are stored in s, and the number of stored roots is returned. */
int sr_solve_quadric_polynomial ( double c[3], double s[2] );

/*! Solve polynomials of 3rd degree. The coefficients are given in c,
    where c[i] is the coefficient of the i-th power of the unknown variable.
	The roots are stored in s, and the number of stored roots is returned. */
int sr_solve_cubic_polynomial ( double c[4], double s[3] );

/*! Solve polynomials of 4th degree. The coefficients are given in c,
    where c[i] is the coefficient of the i-th power of the unknown variable.
	The roots are stored in s, and the number of stored roots is returned. */
int sr_solve_quartic_polynomial ( double c[5], double s[4] );

/*! Returns <0 if (x,y) is inside the ellipse with radius (a,b), returns
    0 if it is on the ellipse, and returns >0 if it is outside */
float sr_in_ellipse ( float x, float y, float a, float b );

/*! This function replaces the given point (x,y) by the closest point
    on the ellipse whose semi-axes are of length a and b. The solution
    is derived analyticaly, solving a quartic polynomial */
void sr_get_closest_on_ellipse ( float a, float b, float& x, float& y );

//============================== end of file ===============================

# endif // SR_ALG_H
