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

static int mainWindow;
static bool windowVisible;
static int screenWidth = 800, screenHeight = 600;
static float screenAspect;

static int leftMouse, middleMouse, rightMouse;
static int mousePosX, mousePosY;
static bool changing = false;
static int selElement;
static std::string selElementName;

static GLint viewport[4];

GLUI *gluiSidePanel, *gluiBottomPanel;

float view_rotate[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
float obj_pos[] = { 0.0, 0.0, 5.0 };


std::map<std::string, vector<Segment*> > readSkeletonFile(const std::string &filename);

static std::map<std::string, Arm> arms;
std::map<std::string, Segment*> bones;
std::map<int, std::string> SegmentNames;

//~ Arm mainArm;
//~ Arm secArm;

Point3f goal;

static void drawAxes(float scale = .5f) {
  glDisable(GL_LIGHTING);

  glPushMatrix();
  glScalef(scale, scale, scale);

  glBegin(GL_LINES);

  glColor3f ( 1.0f, 0.00f, 0.0f );
  glVertex3f( 0.8f, 0.05f, 0.0f ); glVertex3f( 1.0f, 0.25f, 0.0f ); // Letter X
  glVertex3f( 0.8f, 0.25f, 0.0f ); glVertex3f( 1.0f, 0.05f, 0.0f );
  glVertex3f( 0.0f, 0.00f, 0.0f ); glVertex3f( 1.0f, 0.00f, 0.0f ); // X axis

  glColor3f ( 0.00f, 0.0f, 1.0f );
  glVertex3f( 0.05f, 0.8f, 0.0f ); glVertex3f( 0.25f, 1.0f, 0.0f ); // Letter Y
  glVertex3f( 0.15f, 0.9f, 0.0f ); glVertex3f( 0.05f, 1.0f, 0.0f );
  glVertex3f( 0.00f, 0.0f, 0.0f ); glVertex3f( 0.00f, 1.0f, 0.0f ); // Y axis

  glColor3f ( 0.0f, 1.0f, 0.0f );
  glVertex3f( 0.0f, 0.0f, 0.0f );  glVertex3f( 0.0f, 0.0f, 1.0f );  // Z axis
  glEnd();

  glPopMatrix();

  glEnable( GL_LIGHTING );
}

static void drawCube() {
	// Front: multicolor
	glBegin(GL_POLYGON);
	glColor3f( 1.0, 0.0, 0.0 );     glVertex3f(  0.5, -0.5, -0.5 );      // P1 es rojo
	glColor3f( 0.0, 1.0, 0.0 );     glVertex3f(  0.5,  0.5, -0.5 );      // P2 es verde
	glColor3f( 0.0, 0.0, 1.0 );     glVertex3f( -0.5,  0.5, -0.5 );      // P3 es azul
	glColor3f( 1.0, 0.0, 1.0 );     glVertex3f( -0.5, -0.5, -0.5 );      // P4 es morado
	glEnd();

	// Back: white
	glBegin(GL_POLYGON);
	glColor3f(   1.0,  1.0, 1.0 );
	glVertex3f(  0.5, -0.5, 0.5 );
	glVertex3f(  0.5,  0.5, 0.5 );
	glVertex3f( -0.5,  0.5, 0.5 );
	glVertex3f( -0.5, -0.5, 0.5 );
	glEnd();

	// Right: purple);
	glColor3f(  1.0,  0.0,  1.0 );
	glVertex3f( 0.5, -0.5, -0.5 );
	glVertex3f( 0.5,  0.5, -0.5 );
	glVertex3f( 0.5,  0.5,  0.5 );
	glVertex3f( 0.5, -0.5,  0.5 );
	glEnd();

	// Left: green
	glBegin(GL_POLYGON);
	glColor3f(   0.0,  1.0,  0.0 );
	glVertex3f( -0.5, -0.5,  0.5 );
	glVertex3f( -0.5,  0.5,  0.5 );
	glVertex3f( -0.5,  0.5, -0.5 );
	glVertex3f( -0.5, -0.5, -0.5 );
	glEnd();

	// Up: blue
	glBegin(GL_POLYGON);
	glColor3f(   0.0,  0.0,  1.0 );
	glVertex3f(  0.5,  0.5,  0.5 );
	glVertex3f(  0.5,  0.5, -0.5 );
	glVertex3f( -0.5,  0.5, -0.5 );
	glVertex3f( -0.5,  0.5,  0.5 );
	glEnd();

	// Down: red
	glBegin(GL_POLYGON);
	glColor3f(   1.0,  0.0,  0.0 );
	glVertex3f(  0.5, -0.5, -0.5 );
	glVertex3f(  0.5, -0.5,  0.5 );
	glVertex3f( -0.5, -0.5,  0.5 );
	glVertex3f( -0.5, -0.5, -0.5 );
	glEnd();
}

void drawSkeleton(bool pick=false) {
	static const int sphere_segs = 4;

	for (auto it = arms.begin(); it != arms.end(); it++) {
		std::string key = it->first;
		Arm & arm = it->second;
		arm.update();
		//~ if (!pick) arm.draw();
	}

	if (!pick) {
		for (auto it = bones.begin(); it != bones.end(); it++) {
			std::string key = it->first;
			Segment* & seg = it->second;
			if (seg) {
				Point3f end_point = seg->draw();

				glPushMatrix();
					glTranslatef(end_point[0], end_point[1], end_point[2]);
					glutSolidSphere(.05, sphere_segs, sphere_segs);
				glPopMatrix();
			}
		}

	} else {
		for (auto it = bones.begin(); it != bones.end(); it++) {
			std::string key = it->first;
			Segment* & seg = it->second;
			if (seg) {
				Point3f end_point = seg->get_start_point() + seg->get_end_point();

				glLoadName(seg->get_segment_id());
				glPushMatrix();
					glTranslatef(end_point[0], end_point[1], end_point[2]);
					glutSolidSphere(.05, sphere_segs, sphere_segs);
				glPopMatrix();
			}
		}
	}

	glLoadName(0);

	//~ if (!pick) {
		//~ glPushMatrix();
			//~ glTranslatef(goal[0], goal[1], goal[2]);
			//~ glutSolidSphere(.05, sphere_segs, sphere_segs);
		//~ glPopMatrix();

		//~ Point3f secGoal = Point3f(goal[0], goal[1], -goal[2]);
		//~ glPushMatrix();
			//~ glTranslatef(secGoal[0], secGoal[1], secGoal[2]);
			//~ glutSolidSphere(.05, sphere_segs, sphere_segs);
		//~ glPopMatrix();
	//~ }
}

void updateSkeleton() {
    if (selElement && selElementName.size()) {
        arms[selElementName].solve(goal, 100);
        glutPostRedisplay();
    }

    //~ mainArm.solve(goal, 100);

    //~ Point3f secGoal = Point3f(goal[0], goal[1], -goal[2]);
    //~ secArm.solve(secGoal, 100);
}

void setUpSkeleton() {
    auto segvectors = readSkeletonFile("skeletons/human.csv");

    for (auto it = segvectors.begin(); it != segvectors.end(); it++) {
        std::string key = it->first;
        std::vector<Segment*> & segs = it->second;
        arms[key].set_segments(segs);

        if (it->second.size()) {
            Segment * seg = it->second.back();
            bones[key] = seg;
            SegmentNames[seg->get_segment_id()] = key;
        }
    }

    //~ mainArm.set_segments(segvectors["lfoot"]);
    //~ secArm.set_segments(segvectors["rfoot"]);
}


void changeState();

void drawScene(bool pick=false) {
  //int i, j;
  //int piece;
  //char done[PIECES + 1];

  //~ float m[4][4];

  //~ build_rotmatrix(m, trackball_quat);

  //~ memcpy(view_rotate, m, sizeof(view_rotate));

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(obj_pos[0], obj_pos[1], -obj_pos[2]);
  //~ glMultMatrixf(&(m[0][0]));
  //~ glRotatef(180, 0, 0, 1);

	glMultMatrixf(view_rotate);

	glClearColor( .9f, .9f, .9f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glLoadName(0);
  drawAxes();

  //~ drawCube();

  drawSkeleton(pick);
}

void displayHandler() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45, screenAspect, 0.1, 100.0);

  drawScene();

  glutSwapBuffers();
}

int selectElement(int mousex, int mousey) {
  long hits;
  GLuint selectBuf[1024];
  GLuint closest;
  GLuint dist;

  glSelectBuffer(1024, selectBuf);
  glRenderMode(GL_SELECT);
  glInitNames();

  glPushName(~0U);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPickMatrix(mousex, screenHeight - mousey, 4, 4, viewport);
  gluPerspective(45, screenAspect, 0.1, 100.0);

  drawScene(true);

  hits = glRenderMode(GL_RENDER);
  if (hits <= 0) {
    return 0;
  }
  closest = 0;
  dist = 4294967295U;
  while (hits) {
    if (selectBuf[(hits - 1) * 4 + 1] < dist) {
      dist = selectBuf[(hits - 1) * 4 + 1];
      closest = selectBuf[(hits - 1) * 4 + 3];
    }
    hits--;
  }
  return closest;
}

static void normalize(float v[3]) {
    float w = sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
    v[0] /= w;
    v[1] /= w;
    v[2] /= w;
}

static void cross(const float a[3], const float b[3], float p[3]) {
    p[0] = a[1] * b[2] - a[2] * b[1];
    p[1] = a[2] * b[0] - a[0] * b[2];
    p[2] = a[0] * b[1] - a[1] * b[0];
}

static float dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static const float identity[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };

static void mult(const GLfloat a[16], const GLfloat b[16], GLfloat r[16]) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			r[i * 4 + j] =
				a[i * 4 + 0] * b[0 * 4 + j] +
				a[i * 4 + 1] * b[1 * 4 + j] +
				a[i * 4 + 2] * b[2 * 4 + j] +
				a[i * 4 + 3] * b[3 * 4 + j];
		}
	}
}

static bool invert(const GLfloat src[16], GLfloat inverse[16]) {
  GLfloat temp[4][4];

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      temp[i][j] = src[i * 4 + j];
    }
  }

	memcpy(inverse, identity, sizeof(identity));

  for (int i = 0; i < 4; i++) { // Look for largest element in column
    int swap = i;
    for (int j = i + 1; j < 4; j++) {
      if (fabs(temp[j][i]) > fabs(temp[i][i])) {
        swap = j;
      }
    }

    if (swap != i) { // Swap rows
      for (int k = 0; k < 4; k++) {
        double t = temp[i][k];
        temp[i][k] = temp[swap][k];
        temp[swap][k] = t;

        t = inverse[i * 4 + k];
        inverse[i * 4 + k] = inverse[swap * 4 + k];
        inverse[swap * 4 + k] = t;
      }
    }

    if (temp[i][i] == 0) { // No non-zero pivot. The matrix is singular, which shouldn't happen.
      return false;
    }

    double t = temp[i][i];
    for (int k = 0; k < 4; k++) {
      temp[i][k] /= t;
      inverse[i * 4 + k] /= t;
    }

    for (int j = 0; j < 4; j++) {
      if (j != i) {
        t = temp[j][i];
        for (int k = 0; k < 4; k++) {
          temp[j][k] -= temp[i][k] * t;
          inverse[j * 4 + k] -= inverse[i * 4 + k] * t;
        }
      }
    }
  }
  return true;
}


// Given screen x and y coordinates, compute the corresponding object space
// x and y coordinates given that the object space z is 0.9 + OFFSETZ.
// Since the tops of (most) pieces are at z = 0.9 + OFFSETZ, we use that number.

static bool computeXYCoords(int mousex, int mousey, GLfloat * selx, GLfloat * sely, GLfloat h = 0.0f) {
  GLfloat projMatrix[16];
  glGetFloatv(GL_PROJECTION_MATRIX, projMatrix);

  GLfloat modelMatrix[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);

  GLfloat m[16];
  mult(modelMatrix, projMatrix, m);
  if (!invert(m, m)) return false;

  GLfloat in_x = (2.0 * (mousex - viewport[0]) / viewport[2]) - 1.0;
  GLfloat in_y = (2.0 * ((screenHeight - mousey) - viewport[1]) / viewport[3]) - 1.0;

  // [selx, sely, h, w] = m · [in_x, in_y, z, 1]
  // After normalization, it will be [selx/w, sely/w, h/w, 1]

  // h = a + b·z
  GLfloat a = in_x * m[0 * 4 + 2] + in_y * m[1 * 4 + 2] + m[3 * 4 + 2];
  GLfloat b = m[2 * 4 + 2];

  // w = c + d·z
  GLfloat c = in_x * m[0 * 4 + 3] + in_y * m[1 * 4 + 3] + m[3 * 4 + 3];
  GLfloat d = m[2 * 4 + 3];

  //~ printf("a = %f, b = %f, c = %f, d = %f\n", a, b, c, d);

  // Now we need to solve for z: (a + b z) / (c + d z) = h
  // "h" is the height in object space we want to solve z for)

  // a + b·z = h·c + h·d·z
  // b·z - h·d·z = h·c - a
  // z = (h·c - a) / (b - h·d)

  GLfloat top = h * c - a;
  GLfloat bot = b - h * d;
  if (bot == 0.0) return false;

  GLfloat z = top / bot;

  // Now we solve for x and y.  We know that w = c + d z, so we compute it

  GLfloat w = c + d * z;

  // Now for x and y

  *selx = (in_x * m[0 * 4 + 0] + in_y * m[1 * 4 + 0] + z * m[2 * 4 + 0] + m[3 * 4 + 0]) / w;
  *sely = (in_x * m[0 * 4 + 1] + in_y * m[1 * 4 + 1] + z * m[2 * 4 + 1] + m[3 * 4 + 1]) / w;

  return true;
}

void moveSelection(float selx, float sely) {
}

static void reshapeHandler(int width, int height) {
  screenWidth = width;
  screenHeight = height;
  screenAspect = (float)width / (float)height;
  glViewport(0, 0, screenWidth, screenHeight);
  glGetIntegerv(GL_VIEWPORT, viewport);
}

void keyboardHandler(unsigned char c, int x, int y) {
  switch(c) {
    case 27:
    case 'q':
      exit(0);
      break;
    };

  glutPostRedisplay();
}

void specialHandler(int key, int x, int y) {
	printf ("GLUT: ");

	switch (key) {
		case GLUT_KEY_F1 :
			printf ("F1 function key.\n");
			break;
		case GLUT_KEY_F2 :
			printf ("F2 function key. \n");
			break;
		case GLUT_KEY_F3 :
			printf ("F3 function key. \n");
			break;
		case GLUT_KEY_F4 :
			printf ("F4 function key. \n");
			break;
		case GLUT_KEY_F5 :
			printf ("F5 function key. \n");
			break;
		case GLUT_KEY_F6 :
			printf ("F6 function key. \n");
			break;
		case GLUT_KEY_F7 :
			printf ("F7 function key. \n");
			break;
		case GLUT_KEY_F8 :
			printf ("F8 function key. \n");
			break;
		case GLUT_KEY_F9 :
			printf ("F9 function key. \n");
			break;
		case GLUT_KEY_F10 :
			printf ("F10 function key. \n");
			break;
		case GLUT_KEY_F11 :
			printf ("F11 function key. \n");
			break;
		case GLUT_KEY_F12 :
			printf ("F12 function key. \n");
			break;
		case GLUT_KEY_LEFT :
			printf ("Left directional key. \n");
			break;
		case GLUT_KEY_UP :
			printf ("Up directional key. \n");
			break;
		case GLUT_KEY_RIGHT :
			printf ("Right directional key. \n");
			break;
		case GLUT_KEY_DOWN :
			printf ("Down directional key. \n");
			break;
		case GLUT_KEY_PAGE_UP :
			printf ("Page up directional key. \n");
			break;
		case GLUT_KEY_PAGE_DOWN :
			printf ("Page down directional key. \n");
			break;
		case GLUT_KEY_HOME :
			printf ("Home directional key. \n");
			break;
		case GLUT_KEY_END :
			printf ("End directional key. \n");
			break;
		case GLUT_KEY_INSERT :
			printf ("Insert directional key. \n");
			break;
	}

	glutPostRedisplay ();
}


void motionHandler(int x, int y) {

  if (leftMouse && selElement) {
    float selx, sely;
    computeXYCoords(x, y, &selx, &sely, goal[2]);
    goal[0] = selx;
    goal[1] = sely;
    printf("Coords: %f, %f\n", selx, sely);
    updateSkeleton();
  }

  mousePosX = x;
  mousePosY = y;
  glutPostRedisplay();
}


void mouseHandler(int button, int state, int x, int y) {
	printf ("GLUT: ");

	mousePosX = x;
	mousePosY = y;

	switch (button) {
		case GLUT_LEFT_BUTTON:
			switch (state) {
				case GLUT_DOWN:
					printf ("Mouse Left Button Pressed (Down)...\n");
					leftMouse = GL_TRUE;

					selElement = selectElement(mousePosX, mousePosY);
					if (selElement) {
						selElementName = SegmentNames[selElement];
						printf("Selected: %d (%s)\n", selElement, selElementName.c_str());
					} else {
						selElementName = "";
					}

					//~ float selx, sely;
					//~ if (computeCoords(selElement, mousex, mousey, &selx, &sely)) {
					//~ 	grabPiece(selElement, selx, sely);
					//~ }

					break;
				case GLUT_UP:
					printf ("Mouse Left Button Released (Up)...\n");
					leftMouse = GL_FALSE;
					break;
			}
			glutPostRedisplay();
			break;
		case GLUT_MIDDLE_BUTTON:
			switch (state) {
				case GLUT_DOWN:
					printf ("Mouse Middle Button Pressed (Down)...\n");
					middleMouse = GL_TRUE;
					break;
				case GLUT_UP:
					printf ("Mouse Middle Button Released (Up)...\n");
					middleMouse = GL_FALSE;
					break;
			}
			glutPostRedisplay();
			break;
		case GLUT_RIGHT_BUTTON:
			switch (state) {
				case GLUT_DOWN:
					printf ("Mouse Right Button Pressed (Down)...\n");
					rightMouse = GL_TRUE;
					break;
				case GLUT_UP:
					printf ("Mouse Right Button Released (Up)...\n");
					rightMouse = GL_FALSE;
					break;
			}
			glutPostRedisplay();
			break;
	}

	motionHandler(x, y);
}

void idleHandler() {
  if (glutGetWindow() != mainWindow) glutSetWindow(mainWindow);

  glutPostRedisplay();
  //~ GLUI_Master.sync_live_all();

  if (!changing && !windowVisible) {
    glutIdleFunc(NULL);
  } else {
    usleep(50000); // 50 ms
  }
}

void changeState() {
  if (windowVisible) {
    if (!changing) {
      glutIdleFunc(NULL);
    } else {
      glutIdleFunc(idleHandler);
    }
  } else {
    glutIdleFunc(NULL);
  }
}

static const GLfloat light0_ambient[] =  {0.1f, 0.1f, 0.3f, 1.0f};
static const GLfloat light0_diffuse[] =  {.6f, .6f, 1.0f, 1.0f};
static const GLfloat light0_position[] = {.5f, .5f, 1.0f, 0.0f};

static const GLfloat light1_ambient[] =  {0.1f, 0.1f, 0.3f, 1.0f};
static const GLfloat light1_diffuse[] =  {.9f, .6f, 0.0f, 1.0f};
static const GLfloat light1_position[] = {-1.0f, -1.0f, 1.0f, 0.0f};

void init() {
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

  //~ trackball(trackball_quat, 0.0, 0.0, 0.0, 0.0);

	selElement = 0;
	selElementName = "";
}

static void Usage() {
  exit(-1);
}

void visibilityHandler(int v) {
  if (v == GLUT_VISIBLE) {
    windowVisible = true;
  } else {
    windowVisible = false;
  }
  changeState();
}

void menuHandler(int choice) {
	switch(choice) {
		case 1:
			exit(0);
			break;
	}
}

enum {
	QUIT_BUTTON,
};

void gluiControlCallback(int control_id) {
	switch (control_id) {
		case QUIT_BUTTON:
			exit(EXIT_SUCCESS);
			break;
	}
}

int main(int argc, char* argv[]) {
  setUpSkeleton();

  glutInit(&argc, argv);
  glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
  glutInitWindowPosition(50, 50);
  glutInitWindowSize(screenWidth, screenHeight);

  mainWindow = glutCreateWindow("Skeleton");

  init();

  glGetIntegerv(GL_VIEWPORT, viewport);

  glutDisplayFunc(displayHandler);
  GLUI_Master.set_glutReshapeFunc(reshapeHandler);
  GLUI_Master.set_glutKeyboardFunc(keyboardHandler);
  GLUI_Master.set_glutSpecialFunc( specialHandler );
  GLUI_Master.set_glutMouseFunc(mouseHandler);
  glutMotionFunc(motionHandler);
  glutVisibilityFunc(visibilityHandler);

  glutCreateMenu(menuHandler);
  glutAddMenuEntry("Quit", 1);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  // Enable z-buffering

  glEnable(GL_DEPTH_TEST);

  // Set up GLUI

  printf( "GLUI version: %3.2f\n", GLUI_Master.get_version() );

  // Side subwindow

  gluiSidePanel = GLUI_Master.create_glui_subwindow( mainWindow, GLUI_SUBWINDOW_RIGHT );

  // Quit button
  gluiSidePanel->add_button ("Quit", QUIT_BUTTON, gluiControlCallback);

  // Link window to GLUI
  gluiSidePanel->set_main_gfx_window( mainWindow );

  // Bottom subwindow

  gluiBottomPanel = GLUI_Master.create_glui_subwindow( mainWindow, GLUI_SUBWINDOW_BOTTOM );
  gluiBottomPanel->set_main_gfx_window( mainWindow );

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
  GLUI_Master.set_glutIdleFunc(idleHandler);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
