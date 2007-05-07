/*=============================================================
|
| APE - Another Particle Editor
|==============================================================
|
| Desc: main.cpp
=============================================================*/

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "Vector3D.h"
#include "Sprite.h"
#include "timer_.h"
#include "tex_.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <GL/glut.h>
#include <GL/glui.h>

//--------------------------------------------------------
//  Defines
//--------------------------------------------------------

#define APE_TITLE   "APE: Another Particle Editor v0.3.0"
#define APE_CREDITS "By Ben Vinegar, ben@0ad.wildfiregames.com"
#define APE_SPRITE_DIR "./sprites"
#define APE_DEFAULT_SPRITE "1.bmp"

#define APE_MAX_PARTICLES 10000
#define APE_MIN_PARTICLES 10

#define APE_MAX_LIFETIME 20.0f
#define APE_MIN_LIFETIME 1.0f

//--------------------------------------------------------
//  Global Variables
//--------------------------------------------------------

int   main_window;

CSprite g_Sprite;
CParticleEmitter * g_Emitter[1];
CParticleSystem g_PSystem;

// index of currently active/editable particle
int g_pIndex = 0;

// A convenient global structure for holding all
// the GLUI widget pointers.
struct { 
	GLUI *mainWindow;
	// GLUI *aboutWindow;

	GLUI_Spinner *origin[3];
	GLUI_Spinner *originSpread[3];
	GLUI_Spinner *velocity[3];
	GLUI_Spinner *veloSpread[3];
	GLUI_Spinner *gravity[3];
	GLUI_Spinner *colorStart[3];
	GLUI_Spinner *colorEnd[3];
	GLUI_Spinner *lifetime[2];
	GLUI_Spinner *spriteSize[2];
	GLUI_Spinner *numParticles;
} g_GLUI;

char g_loadedSpriteFilename[30];
GLUI_EditText *g_spriteTextBox;

unsigned int g_frames = 0;

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

// Event functions linked to by GLUI
void loadSprite(int param);
void clearSpriteTextBox(int param);
void writeToFile(int param);
void showAboutWindow(int param);

void colorStartEvent(int index);
void colorEndEvent(int index);
void lifetimeEvent(int param);
void minParticlesEvent(int param);
void gravityEvent(int param);
void veloEvent(int param);
void veloSpreadEvent(int param);
void originEvent(int param);
void originSpreadEvent(int param);
void spriteSizeEvent(int param);

void quit(int retVal);

// GLUT specific
void myGlutIdle();
void myGlutReshape(int x, int y);
void myGlutIdle();
void myGlutDisplay();


// Initialize all GLUI windows, set callbacks, etc.
void initGLUI();

//--------------------------------------------------------
//  Implementation
//--------------------------------------------------------

void loadSprite(int param) {
	char filename[30];
	
	// build path name
	strcpy(filename, APE_SPRITE_DIR);
	strcat(filename, "/");
	strcat(filename, g_spriteTextBox->get_text());

	// check if file exists first
	FILE *in = fopen(filename, "r");
	if (in == NULL) {
		printf("Error: %s does not exist.\n", filename);
		g_spriteTextBox->set_text(g_loadedSpriteFilename);
		return;
	}

	// load texture
	GLuint tex;
	tex_load(filename, &tex);

	if (tex < 1) {
		printf("Error: %s is not a bitmap.\n", filename);
		g_spriteTextBox->set_text(g_loadedSpriteFilename);
		return;
	}

	g_Emitter[g_pIndex]->SetTexture(tex);
	//g_Sprite.SetTexture(tex);
	strcpy(g_loadedSpriteFilename, g_spriteTextBox->get_text()); 
}

void clearSpriteTextBox(int param) {
	g_spriteTextBox->set_text("");
}

void writeToFile(int param) {
	FILE *out = fopen("new.ape", "w");
	fwrite(&g_Emitter[g_pIndex], sizeof(CParticleEmitter), 1, out);
	fclose(out);
}

void quit(int retVal) {
	float fps = g_frames / get_time();
	FILE *out = fopen("out.txt", "w");
	fprintf(out, "%f\n", fps);
	exit(retVal);
}

// called during color start spinner event
void colorStartEvent(int param) {
	for (int i = 0; i < 3; i++) {
		if (g_GLUI.colorStart[i]->get_float_val() < g_GLUI.colorEnd[i]->get_float_val()) {
			g_GLUI.colorStart[i]->set_float_val(g_GLUI.colorEnd[i]->get_float_val());
		}
	}

	float r, g, b;
	r = g_GLUI.colorStart[0]->get_float_val();
	g = g_GLUI.colorStart[1]->get_float_val();
	b = g_GLUI.colorStart[2]->get_float_val();

	g_Emitter[g_pIndex]->SetStartColour(r, g, b, 1.0f);
}

// called during color end spinner event
void colorEndEvent(int index) {
	for (int i = 0; i < 3; i++) {
		if (g_GLUI.colorEnd[i]->get_float_val() > g_GLUI.colorStart[i]->get_float_val()) {
			g_GLUI.colorEnd[i]->set_float_val(g_GLUI.colorStart[i]->get_float_val());
		}
	}

	float r, g, b;
	r = g_GLUI.colorEnd[0]->get_float_val();
	g = g_GLUI.colorEnd[1]->get_float_val();
	b = g_GLUI.colorEnd[2]->get_float_val();

	g_Emitter[g_pIndex]->SetEndColour(r, g, b, 1.0f);
}

// called during lifetime spinner event
void lifetimeEvent(int param) {
	if (g_GLUI.lifetime[0]->get_float_val() > g_GLUI.lifetime[1]->get_float_val())
		g_GLUI.lifetime[1]->set_float_val(g_GLUI.lifetime[0]->get_float_val());
	else if (g_GLUI.lifetime[1]->get_float_val() < g_GLUI.lifetime[0]->get_float_val())
		g_GLUI.lifetime[0]->set_float_val(g_GLUI.lifetime[1]->get_float_val());

	g_Emitter[g_pIndex]->SetMinLifetime(g_GLUI.lifetime[0]->get_float_val());
	g_Emitter[g_pIndex]->SetMaxLifetime(g_GLUI.lifetime[1]->get_float_val());
}


void numParticlesEvent(int param) {
	float numParticles;
	numParticles = g_GLUI.numParticles->get_float_val();

	g_Emitter[g_pIndex]->SetMinParticles(numParticles);
}

void gravityEvent(int param) {
	CVector3D grav;
	grav.X = g_GLUI.gravity[0]->get_float_val();
	grav.Y = g_GLUI.gravity[1]->get_float_val();
	grav.Z = g_GLUI.gravity[2]->get_float_val();

	g_Emitter[g_pIndex]->SetGravity(grav);
}

void veloEvent(int param) {
	CVector3D velo;
	velo.X = g_GLUI.velocity[0]->get_float_val();
	velo.Y = g_GLUI.velocity[1]->get_float_val();
	velo.Z = g_GLUI.velocity[2]->get_float_val();

	g_Emitter[g_pIndex]->SetVelocity(velo);
}

void veloSpreadEvent(int param) {
	CVector3D spread;
	spread.X = g_GLUI.veloSpread[0]->get_float_val();
	spread.Y = g_GLUI.veloSpread[1]->get_float_val();
	spread.Z = g_GLUI.veloSpread[2]->get_float_val();

	g_Emitter[g_pIndex]->SetVelocitySpread(spread);
}

void originEvent(int param) {
	CVector3D origin;
	origin.X = g_GLUI.origin[0]->get_float_val();
	origin.Y = g_GLUI.origin[1]->get_float_val();
	origin.Z = g_GLUI.origin[2]->get_float_val();

	g_Emitter[g_pIndex]->SetOrigin(origin);
}

void originSpreadEvent(int param) {
	CVector3D spread;
	spread.X = g_GLUI.originSpread[0]->get_float_val();
	spread.Y = g_GLUI.originSpread[1]->get_float_val();
	spread.Z = g_GLUI.originSpread[2]->get_float_val();

	g_Emitter[g_pIndex]->SetOriginSpread(spread);
}

void spriteSizeEvent(int param) {
	float width, height;

	width  = g_GLUI.spriteSize[0]->get_float_val();
	height = g_GLUI.spriteSize[1]->get_float_val();

	g_Emitter[g_pIndex]->SetWidth(width);
	g_Emitter[g_pIndex]->SetHeight(height);
}
/***************************************** myGlutIdle() ***********/

void myGlutIdle( void )
{
  /* According to the GLUT specification, the current window is 
     undefined during an idle callback.  So we need to explicitly change
     it if necessary */
  if ( glutGetWindow() != main_window ) 
    glutSetWindow(main_window);  

  glutPostRedisplay();
}


/**************************************** myGlutReshape() *************/

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

/***************************************** myGlutDisplay() *****************/

void myGlutDisplay( void )
{
	colorStartEvent(0);
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glTranslatef( 0.0, 0.0, -7.0 );
 

	//g_Emitter[g_pIndex]->SetMinLifetime(g_minLifetime);
	//g_Emitter[g_pIndex]->SetMaxLifetime(g_maxLifetime);

	//g_Sprite.Render();
	//g_Emitter[g_pIndex]->Frame();

	g_PSystem.Frame();
	++g_frames;

	glutSwapBuffers(); 
}


/**************************************** main() ********************/

void main(int argc, char* argv[])
{
  /****************************************/
  /*   Initialize GLUT and create window  */
  /****************************************/

	glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowPosition( 50, 50 );
	glutInitWindowSize( 600, 600 );
 
	main_window = glutCreateWindow(APE_TITLE);
	glutDisplayFunc( myGlutDisplay );
	glutReshapeFunc( myGlutReshape );  

	glDisable(GL_LIGHTING);


	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

  /****************************************/
  /*         Here's the GLUI code         */
  /****************************************/

	initGLUI();

	// have the system create a new emitter
	g_Emitter[g_pIndex] = g_PSystem.CreateNewEmitter();

	// load default sprite
	g_spriteTextBox->set_text(APE_DEFAULT_SPRITE);
	
	loadSprite(-1);
	g_Sprite.SetSize(1.0f, 1.0f);
	
//	g_Emitter[g_pIndex]->SetSprite(&g_Sprite);
	g_Emitter[g_pIndex]->Init();

	glutMainLoop();
}


void initGLUI() {
	// create master window
	g_GLUI.mainWindow = GLUI_Master.create_glui( "Control Panel" );

	GLUI* glui = g_GLUI.mainWindow;

	// create origin panel and spinner contents
	GLUI_Panel *origin_panel = glui->add_panel("Origin");
	g_GLUI.origin[0] = glui->add_spinner_to_panel(origin_panel, "X", 
		GLUI_SPINNER_FLOAT, NULL, -1, originEvent);
	g_GLUI.origin[1] = glui->add_spinner_to_panel(origin_panel, "Y", 
		GLUI_SPINNER_FLOAT, NULL, -1, originEvent);
	g_GLUI.origin[2] = glui->add_spinner_to_panel(origin_panel, "Z", 
		GLUI_SPINNER_FLOAT, NULL, -1, originEvent);

	// create origin spread panel and spinner contents
	GLUI_Panel *origin_spread_panel = glui->add_panel("Origin Spread");
	g_GLUI.originSpread[0] = glui->add_spinner_to_panel(origin_spread_panel, "X", 
		GLUI_SPINNER_FLOAT, NULL, -1, originSpreadEvent);
	g_GLUI.originSpread[1] = glui->add_spinner_to_panel(origin_spread_panel, "Y", 
		GLUI_SPINNER_FLOAT, NULL, -1, originSpreadEvent);
	g_GLUI.originSpread[2] = glui->add_spinner_to_panel(origin_spread_panel, "Z",
		GLUI_SPINNER_FLOAT, NULL, -1, originSpreadEvent);

	// create velocity panel and spinner contents
	GLUI_Panel *velo_panel = glui->add_panel("Velocity");
	g_GLUI.velocity[0] = glui->add_spinner_to_panel(velo_panel, "X", 
		GLUI_SPINNER_FLOAT, NULL, -1, veloEvent);
	g_GLUI.velocity[1] = glui->add_spinner_to_panel(velo_panel, "Y", 
		GLUI_SPINNER_FLOAT, NULL, -1, veloEvent);
	g_GLUI.velocity[2] = glui->add_spinner_to_panel(velo_panel, "Z", 
		GLUI_SPINNER_FLOAT, NULL, -1, veloEvent);

	// create velocity spread panel and spinner contents
	GLUI_Panel *velo_spread_panel = glui->add_panel("Velocity Spread");
	g_GLUI.veloSpread[0] = glui->add_spinner_to_panel(velo_spread_panel, "X",
		GLUI_SPINNER_FLOAT, NULL, -1, veloSpreadEvent);
	g_GLUI.veloSpread[1] = glui->add_spinner_to_panel(velo_spread_panel, "Y", 
		GLUI_SPINNER_FLOAT, NULL, -1, veloSpreadEvent);
	g_GLUI.veloSpread[2] = glui->add_spinner_to_panel(velo_spread_panel, "Z", 
		GLUI_SPINNER_FLOAT, NULL, -1, veloSpreadEvent);

	// create gravity panel and spinner contents
	GLUI_Panel *grav_panel = glui->add_panel("Gravity");
	g_GLUI.gravity[0] = glui->add_spinner_to_panel(grav_panel, "X", 
		GLUI_SPINNER_FLOAT, NULL, -1, gravityEvent);
	g_GLUI.gravity[1] = glui->add_spinner_to_panel(grav_panel, "Y", 
		GLUI_SPINNER_FLOAT, NULL, -1, gravityEvent);
	g_GLUI.gravity[2] = glui->add_spinner_to_panel(grav_panel, "Z", 
		GLUI_SPINNER_FLOAT, NULL, -1, gravityEvent);

	// add separator
	glui->add_column(true);

	// create start colour panels and spinners
	GLUI_Panel *start_color_panel = glui->add_panel("Start Colour");
	g_GLUI.colorStart[0] = glui->add_spinner_to_panel(start_color_panel, "R", 
		GLUI_SPINNER_FLOAT, NULL, -1, colorStartEvent);
	g_GLUI.colorStart[1] = glui->add_spinner_to_panel(start_color_panel, "G", 
		GLUI_SPINNER_FLOAT, NULL, -1, colorStartEvent);
	g_GLUI.colorStart[2] = glui->add_spinner_to_panel(start_color_panel, "B", 
		GLUI_SPINNER_FLOAT, NULL, -1, colorStartEvent);

	// create end colour panel and spinners
	GLUI_Panel *end_color_panel = glui->add_panel("End Colour");
	g_GLUI.colorEnd[0] = glui->add_spinner_to_panel(end_color_panel, "R", 
		GLUI_SPINNER_FLOAT, NULL, -1, colorEndEvent);
	g_GLUI.colorEnd[1] = glui->add_spinner_to_panel(end_color_panel, "G", 
		GLUI_SPINNER_FLOAT, NULL, -1, colorEndEvent);	
	g_GLUI.colorEnd[2] = glui->add_spinner_to_panel(end_color_panel, "B", 
		GLUI_SPINNER_FLOAT, NULL, -1, colorEndEvent);
   
	// set limits, speed for all colour spinners
	for (int i = 0; i < 3; i++) {
		g_GLUI.colorStart[i]->set_float_limits(0.0f, 1.0f);
		g_GLUI.colorEnd[i]->set_float_limits(0.0f, 1.0f);
		g_GLUI.colorStart[i]->set_speed(0.5);
		g_GLUI.colorEnd[i]->set_speed(0.5);
		g_GLUI.colorStart[i]->set_float_val(1.0f);
		g_GLUI.colorEnd[i]->set_float_val(1.0f);
	}

	// create lifetime panel and spinners
	GLUI_Panel *lifetimes = glui->add_panel("Lifetimes");
	g_GLUI.lifetime[0] = glui->add_spinner_to_panel(lifetimes, "Min", 
		GLUI_SPINNER_FLOAT, NULL, -1, lifetimeEvent);
	g_GLUI.lifetime[1] = glui->add_spinner_to_panel(lifetimes, "Max", 
		GLUI_SPINNER_FLOAT, NULL, -1, lifetimeEvent);

	// set lifetime limits
	for (i = 0; i < 2; i++) {
		g_GLUI.lifetime[i]->set_float_limits(1.0f, 20.0f);
	}

	// create number of particles panel and spinner 
	GLUI_Panel *num_part_panel = glui->add_panel("# of Particles");
	g_GLUI.numParticles = glui->add_spinner_to_panel(num_part_panel, "",
		GLUI_SPINNER_INT, NULL, -1, numParticlesEvent);
	g_GLUI.numParticles->set_int_limits(2, APE_MAX_PARTICLES);

	// create sprite panel and spinners
	GLUI_Panel *sprite_panel = glui->add_panel("Sprite");
	g_GLUI.spriteSize[0] = glui->add_spinner_to_panel(sprite_panel, "Width", 
		GLUI_SPINNER_FLOAT, NULL, -1, spriteSizeEvent);
	g_GLUI.spriteSize[0]->set_float_val(1.0f);
	g_GLUI.spriteSize[1] = glui->add_spinner_to_panel(sprite_panel, "Height", 
		GLUI_SPINNER_FLOAT, NULL, -1, spriteSizeEvent);
	g_GLUI.spriteSize[1]->set_float_val(1.0f);

	g_GLUI.spriteSize[0]->set_float_limits(0.05, 5);
	g_GLUI.spriteSize[1]->set_float_limits(0.05, 5);
	
	// load / save panel
	glui->add_separator_to_panel(sprite_panel);
	g_spriteTextBox = glui->add_edittext_to_panel(sprite_panel, "Filename");
	GLUI_Button *load_button = glui->add_button_to_panel(sprite_panel, "Load", -1, loadSprite);
	glui->add_button_to_panel(sprite_panel, "Clear", -1, clearSpriteTextBox);
	GLUI_Button *save_button = glui->add_button("Save Effect", -1, writeToFile);
	save_button->disable();

	glui->add_button("Quit", -1, quit);

	glui->set_main_gfx_window( main_window );
	

	// register idle callback with GLUI -- NOT GLUT
	GLUI_Master.set_glutIdleFunc( myGlutIdle ); 
}
