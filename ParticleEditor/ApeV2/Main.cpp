#include <stdio.h>
#include "AttributeEditor.h"

int   wireframe = 0;
int   main_window;

GLUI *glui;

CAttributeEditor *pAttributeEditor;

void myGlutIdle( void )
{
	/* According to the GLUT specification, the current window is 
		undefined during an idle callback.  So we need to explicitly change
		it if necessary */
	if ( glutGetWindow() != main_window ) 
		glutSetWindow(main_window);  

	glutPostRedisplay();
}

void myGlutReshape( int x, int y )
{
	float xy_aspect;

	xy_aspect = (float)x / (float)y;
	glViewport( 0, 0, x, y );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glFrustum( -xy_aspect*.08, xy_aspect*.08, -.08, .08, .1, 15.0 );

	glutPostRedisplay();
}

void myGlutDisplay( void )
{
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();                   // Reset modelview matrix for new frame.

	glTranslatef(0.0f, 0.0f, -5.0f);

	//// Set root skeleton's orientation and position
	//glTranslatef(pEmitter->getPosX(), pEmitter->getPosY(), pEmitter->getPosZ());	

	//glRotatef(g_ViewRot.z, 0.0f, 0.0f, 1.0f);
	//glRotatef(g_ViewRot.y, 0.0f, 1.0f, 0.0f);
	//glRotatef(g_ViewRot.x, 1.0f, 0.0f, 0.0f);

	pAttributeEditor->UpdateAttributeEditor();

	glutSwapBuffers(); 
}

int main(void)
{
	// Init the Attribute Editor
	pAttributeEditor = CAttributeEditor::GetInstance();
	glui = pAttributeEditor->InitAttributeEditor();

	

	// Attribute Editor Code:

	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowPosition( 50, 50 );
	glutInitWindowSize( 800, 600 );
 
	main_window = glutCreateWindow( "Ape V2" );
	glutDisplayFunc( myGlutDisplay );
	glutReshapeFunc( myGlutReshape );  

	glui->set_main_gfx_window( main_window );
 
	/* We register the idle callback with GLUI, *not* with GLUT */
	GLUI_Master.set_glutIdleFunc( myGlutIdle );

	glutMainLoop();

	return 0;
}