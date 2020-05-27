#include "include.h"
#include "arm.h"
#include "point.h"
#include "segment.h"

#include <string>
#include <map> 

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glui.h>

#include <cmath>
#include <unistd.h>

std::map<std::string, vector<Segment*> > readSkeletonFile(const std::string &filename);

static std::map<std::string, Arm> arms;

Arm mainArm;
Arm secArm;

Point3f goal;

float angle = 0.0;
float zangle = 0.0;



bool showText = false;
const char stringText[] = "Hello World!";

float xy_aspect;
int   last_x, last_y;
float rotationX = 0.0, rotationY = 0.0;


int   light0_enabled = 1;
int   light1_enabled = 1;
float light0_intensity = 1.0;
float light1_intensity = .4;
int   main_window;
float scale = 1.0;

float view_rotate[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
float obj_pos[] = { 0.0, 0.0, 0.0 };

GLUI *gluiSidePanel, *gluiBottomPanel;

GLfloat light0_ambient[] =  {0.1f, 0.1f, 0.3f, 1.0f};
GLfloat light0_diffuse[] =  {.6f, .6f, 1.0f, 1.0f};
GLfloat light0_position[] = {.5f, .5f, 1.0f, 0.0f};

GLfloat light1_ambient[] =  {0.1f, 0.1f, 0.3f, 1.0f};
GLfloat light1_diffuse[] =  {.9f, .6f, 0.0f, 1.0f};
GLfloat light1_position[] = {-1.0f, -1.0f, 1.0f, 0.0f};

GLfloat lights_rotation[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

void glutKeyboardHandler(unsigned char Key, int x, int y) {
  switch(Key) {
    case 27:
    case 'q':
      exit(0);
      break;
    };

  glutPostRedisplay();
}

void glutIdleHandler() {
  if (glutGetWindow() != main_window) glutSetWindow(main_window);

  //~ GLUI_Master.sync_live_all();
  glutPostRedisplay();
  usleep(50000); // 50 ms
}

void glutMouseHandler(int button, int button_state, int x, int y ) {
}

void glutMotionHandler(int x, int y ) {
  glutPostRedisplay();
}

void glutReshapeHandler( int x, int y ) {
  int tx, ty, tw, th;
  GLUI_Master.get_viewport_area( &tx, &ty, &tw, &th );
  glViewport( tx, ty, tw, th );

  xy_aspect = (float)tw / (float)th;

  glutPostRedisplay();
}

void drawAxes(float scale) {
  glDisable(GL_LIGHTING);

  glPushMatrix();
  glScalef(scale, scale, scale);

  glBegin(GL_LINES);

  glColor3f( 1.0, 0.0, 0.0 );
  glVertex3f( .8f, 0.05f, 0.0 );  glVertex3f( 1.0, 0.25f, 0.0 ); // Letter X
  glVertex3f( 0.8f, .25f, 0.0 );  glVertex3f( 1.0, 0.05f, 0.0 );
  glVertex3f( 0.0, 0.0, 0.0 );  glVertex3f( 1.0, 0.0, 0.0 ); // X axis

  glColor3f( 0.0, 0.0, 1.0 );
  glVertex3f( 0.05f, .8f, 0.0 );  glVertex3f( 0.25f, 1.0, 0.0 ); // Letter Y
  glVertex3f( .15f, 0.9f, 0.0 );  glVertex3f( 0.05f, 1.0, 0.0 );
  glVertex3f( 0.0, 0.0, 0.0 );  glVertex3f( 0.0, 1.0, 0.0 ); // Y axis

  glColor3f( 0.0, 1.0, 0.0 );
  glVertex3f( 0.0, 0.0, 0.0 );  glVertex3f( 0.0, 0.0, 1.0 ); // Z axis
  glEnd();

  glPopMatrix();

  glEnable( GL_LIGHTING );
}

void drawSkeleton() {
    for (auto it = arms.begin(); it != arms.end(); it++) {
        std::string key = it->first;
        Arm & arm = it->second;
        arm.draw();
    }

  int segs = 4;

  glPushMatrix();
    glTranslatef( goal[0], goal[1], goal[2] );
    glutSolidSphere( .05, segs, segs );
  glPopMatrix();

  Point3f secGoal = Point3f(goal[0], goal[1], -goal[2]);
  glPushMatrix();
    glTranslatef( secGoal[0], secGoal[1], secGoal[2] );
    glutSolidSphere( .05, segs, segs );
  glPopMatrix();

}

void updateSkeleton() {
    angle += 360.0/60.0;
    zangle += 360.0f/100.0f;

    goal = Point3f(cos(angle * M_PI / 180.0f), sin(angle * M_PI / 180.0f), (sin(zangle * M_PI / 180.0f)));
    //Point3f goal(cos(angle*PI/180.0f), sin(angle*PI/180.0f), 0);

    goal.normalize();
    goal *= 1.2;

    goal += Vector3f(-.7, 0, -0.2);
    //goal = Vector3f(0, 0, 7);
    mainArm.solve(goal, 100);

    Point3f secGoal = Point3f(goal[0], goal[1], -goal[2]);
    secArm.solve(secGoal, 100);
}

void setUpSkeleton() {
    auto segments = readSkeletonFile("skeletons/human.csv");

    for (auto it = segments.begin(); it != segments.end(); it++) {
        std::string key = it->first;
        std::vector<Segment*> & segs = it->second;
        arms[key].set_segments(segs);
    }

    mainArm.set_segments(segments["lfoot"]);
    secArm.set_segments(segments["rfoot"]);
}

void glutDisplayHandler() {
  glClearColor( .9f, .9f, .9f, 1.0f );
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glFrustum( -xy_aspect*.04, xy_aspect*.04, -.04, .04, .1, 15.0 );

  glMatrixMode( GL_MODELVIEW );

  glLoadIdentity();
  glMultMatrixf( lights_rotation );
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

  glLoadIdentity();
  glTranslatef( 0.0, 0.0, -2.6f );
  glTranslatef( obj_pos[0], obj_pos[1], -obj_pos[2] );
  glMultMatrixf( view_rotate );

  glScalef( scale, scale, scale );

  drawAxes(.52f);

  glPushMatrix();
  //~ glTranslatef( -1., 0.0, 0.0 );
  //~ glRotatef(90, 0.0, 1.0, 0.0);
  drawSkeleton();
  glPopMatrix();

  if (showText) {
    glDisable( GL_LIGHTING ); // Disable lighting while we render text
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluOrtho2D( 0.0, 100.0, 0.0, 100.0  );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glColor3ub( 0, 0, 0 );
    glRasterPos2i( 10, 10 );

    // Render the live character array 'text'
    for (unsigned int i = 0; i < strlen(stringText); i++ ) {
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, stringText[i]);
    }
  }

  glEnable( GL_LIGHTING );

  glutSwapBuffers();
}

enum {
	STEP_BUTTON,
	QUIT_BUTTON,
};

void gluiControlCallback(int control_id) {
	switch (control_id) {
		case STEP_BUTTON:
			updateSkeleton();
			break;
		case QUIT_BUTTON:
			exit(EXIT_SUCCESS);
			break;
	}
}

int main(int argc, char* argv[]) {

  setUpSkeleton();

  // Initialize GLUT and create window

  glutInit(&argc, argv);
  glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
  glutInitWindowPosition( 50, 50 );
  glutInitWindowSize( 800, 600 );

  main_window = glutCreateWindow( "Skeleton" );
  glutDisplayFunc( glutDisplayHandler );
  GLUI_Master.set_glutReshapeFunc( glutReshapeHandler );
  GLUI_Master.set_glutKeyboardFunc( glutKeyboardHandler );
  GLUI_Master.set_glutSpecialFunc( NULL );
  GLUI_Master.set_glutMouseFunc( glutMouseHandler );
  glutMotionFunc( glutMotionHandler );

  // Set up OpenGL lights

  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);

  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

  glEnable(GL_LIGHT1);
  glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
  glLightfv(GL_LIGHT1, GL_POSITION, light1_position);

  // Enable z-buffering

  glEnable(GL_DEPTH_TEST);

  // Set up GLUI

  printf( "GLUI version: %3.2f\n", GLUI_Master.get_version() );

  // Side subwindow

  gluiSidePanel = GLUI_Master.create_glui_subwindow( main_window, GLUI_SUBWINDOW_RIGHT );

  // Quit button

	gluiSidePanel->add_button ("Step", STEP_BUTTON, gluiControlCallback);
	gluiSidePanel->add_button ("Quit", QUIT_BUTTON, gluiControlCallback);


  // Link window to GLUI
  gluiSidePanel->set_main_gfx_window( main_window );


  // Bottom subwindow

  gluiBottomPanel = GLUI_Master.create_glui_subwindow( main_window, GLUI_SUBWINDOW_BOTTOM );
  gluiBottomPanel->set_main_gfx_window( main_window );

  GLUI_Rotation *view_rot = new GLUI_Rotation(gluiBottomPanel, "Objects", view_rotate );
  view_rot->set_spin( 1.0 );
  new GLUI_Column( gluiBottomPanel, false );

  GLUI_Translation *trans_xy = new GLUI_Translation(gluiBottomPanel, "Objects XY", GLUI_TRANSLATION_XY, obj_pos );
  trans_xy->set_speed( .005 );
  new GLUI_Column( gluiBottomPanel, false );
  GLUI_Translation *trans_x = new GLUI_Translation(gluiBottomPanel, "Objects X", GLUI_TRANSLATION_X, obj_pos );
  trans_x->set_speed( .005 );
  new GLUI_Column( gluiBottomPanel, false );
  GLUI_Translation *trans_y = new GLUI_Translation( gluiBottomPanel, "Objects Y", GLUI_TRANSLATION_Y, &obj_pos[1] );
  trans_y->set_speed( .005 );
  new GLUI_Column( gluiBottomPanel, false );
  GLUI_Translation *trans_z = new GLUI_Translation( gluiBottomPanel, "Objects Z", GLUI_TRANSLATION_Z, &obj_pos[2] );
  trans_z->set_speed( .005 );

  // We register the idle callback with GLUI, *not* with GLUT
  GLUI_Master.set_glutIdleFunc(glutIdleHandler);

  // GLUT main loop
  glutMainLoop();

  return EXIT_SUCCESS;
}

