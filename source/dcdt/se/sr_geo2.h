
/** \file sr_geo2.h 
 * 2d geometric primitives */

# ifndef SR_GEO2_H
# define SR_GEO2_H

# define SR_CCW(ax,ay,bx,by,cx,cy)  ((ax*by)-(bx*ay)+(bx*cy)-(cx*by)+(cx*ay)-(ax*cy))

/*! Returns true if segments (p1,p2) and (p3,p4) intersect, and false otherwise. */
bool sr_segments_intersect ( double p1x, double p1y, double p2x, double p2y,
                             double p3x, double p3y, double p4x, double p4y );

/*! Returns true if segments (p1,p2) and (p3,p4) intersect, and false otherwise. 
    In case of intersection, p will be the intersection point. */
bool sr_segments_intersect ( double p1x, double p1y, double p2x, double p2y,
                             double p3x, double p3y, double p4x, double p4y,
                             double& x, double &y );

/*! Returns true if lines (p1,p2) and (p3,p4) intersect, and false otherwise. */
bool sr_lines_intersect ( double p1x, double p1y, double p2x, double p2y,
                          double p3x, double p3y, double p4x, double p4y );

/*! Returns true if lines (p1,p2) and (p3,p4) intersect, and false otherwise. 
    In case of intersection, p will be the intersection point. */
bool sr_lines_intersect ( double p1x, double p1y, double p2x, double p2y,
                          double p3x, double p3y, double p4x, double p4y,
                          double& x, double &y );

/*! Orthogonal projection of p in the line (p1,p2). The projected point becomes q. */
void sr_line_projection ( double p1x, double p1y, double p2x, double p2y,
                          double px, double py,
                          double& qx, double& qy );

/*! Returns true if the orthogonal projection of p is inside (within epsilon distance)
    the segment (p1,p2). In such a case, the projected point becomes q, otherwise
    false is returned. */
bool sr_segment_projection ( double p1x, double p1y, double p2x, double p2y,
                             double px, double py,
                             double& qx, double& qy, double epsilon );

/*! Returns the square of the distance between p1 and p2 */
double sr_dist2 ( double p1x, double p1y, double p2x, double p2y );

/*! Returns the minimum distance between p and segment (p1,p2) */
double sr_point_segment_dist ( double px, double py, 
                               double p1x, double p1y, double p2x, double p2y );

/*! Returns the minimum distance between p and line (p1,p2) */
double sr_point_line_dist ( double px, double py, double p1x, double p1y, double p2x, double p2y );

/*! Returns true if the distance between p1 and p2 is smaller (or equal) than epsilon */
bool sr_next ( double p1x, double p1y, double p2x, double p2y, double epsilon );

/*! Returns >0 if the three points are in counter-clockwise order, <0 if
    the order is clockwise and 0 if points are collinear. */
double sr_ccw ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y );

/*! Returns true if p is in the segment (p1,p2), within precision epsilon, and false
    otherwise. More precisely, true is returned if the projection of p in the segment (p1,p2)
    has a distance to p less or equal than epsilon. */
bool sr_in_segment ( double p1x, double p1y, double p2x, double p2y,
                     double px, double py, double epsilon );

/*! Returns true if p is in the segment (p1,p2), within precision epsilon, and false
    otherwise. More precisely, true is returned if the projection of p in the segment
    p1,p2) has a distance to p less or equal than epsilon. Parameter dist2 contains
    the square of that distance. */
bool sr_in_segment ( double p1x, double p1y, double p2x, double p2y, double px, double py,
                     double epsilon, double& dist2 );

/*! Returns true if p is inside (or in the border) of triangle (p1,p2,p3), otherwise false
    is returned. The test is based on 3 CCW>=0 tests and no epsilons are used. */
bool sr_in_triangle ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y,
                      double px, double py );

/*! Returns true if p is inside the circle passing at p1, p2, and p3, otherwise false
    is returned. Points p1, p2 and p3 must be in ccw orientation.
    This is a fast 4x4 determinant evaluation that will return true only
    if p is strictly inside the circle, testing if determinant>1.0E-14 */
bool sr_in_circle ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y,
                    double px, double py );

/*! Returns (u,v,w), w==1-u-v, u+v+w==1, such that p1*u + p2*v + p3w == p */
void sr_barycentric ( double p1x, double p1y, double p2x, double p2y, double p3x, double p3y,
                      double px, double py, double& u, double& v, double& w );

//============================== end of file ===============================

# endif // SR_GEO2_H
