APE Source Code Readme
---------------------------------------

To compile this source code, you'll first need GLUI, a simple GLUT-based
GUI library used by APE.

GLUI Home:
http://www.cs.unc.edu/~rademach/glui/

Note: The "familiar" files found in the /ext directory are not
identical counterparts to those found in the Prometheus build. In order
to abstract away APE from Prometheus, I took as few class and files
necessary and wrapped several functions in order to make it easy to
switch particle-related code (CParticle, CParticleEmitter, CParticleSystem)
between the two projects. Currently, a few #defines need to be changed
in order to accomplish this.


Sincerely,
Ben Vinegar

@Contact: benvinegar () hotmail ! com