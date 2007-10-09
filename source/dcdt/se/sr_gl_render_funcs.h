
# ifndef SR_GL_RENDER_FUNCS_H
# define SR_GL_RENDER_FUNCS_H

/** \file sr_gl_render_funcs.h 
 * OpenGL render functions of SR shapes
 */

class SrSnShapeBase;

/*! All render functions used to render SR shapes are static methods
    of class SrGlRenderFuncs. They are automatically registered to
    the ogl render action. See SrGlRenderAction class description. */
class SrGlRenderFuncs
 { public:
    static void render_model ( SrSnShapeBase* shape );

    static void render_lines ( SrSnShapeBase* shape );

    static void render_points ( SrSnShapeBase* shape );

    /*! The default render mode is smooth, and flat has the same
        effect of smooth shading. */
    static void render_box ( SrSnShapeBase* shape );

    /*! The resolution value stored in SrSnShapeBase
        indicates how many triangles is used to
        draw the sphere. A value lower or equal to 0.2 defines
        the first level (8 triangles), then levels are increased
        each time 0.2 is added to the resolution. Resolution 1.0
        represents a well discretized sphere. */
    static void render_sphere ( SrSnShapeBase* shape );

    /*! The resolution value stored in SrSnShapeBase
        represents 1/10th of the number of edges discretizing the base
        circle of the cylinder. For ex., resolution 1.0 results in
        10 edges, and gives 40 triangles to render the cylinder. */
    static void render_cylinder ( SrSnShapeBase* shape );

    static void render_polygon ( SrSnShapeBase* shape );

    static void render_polygons ( SrSnShapeBase* shape );
 };

//================================ End of File =================================================

# endif  // SR_GL_RENDER_FUNCS_H

