#include "include.h"
#include "arm.h"
#include "point.h"
#include "segment.h"
#include "tinyfiledialogs.h"

#include <string>
#include <map>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glui.h>

#include <cmath>
#include <ctime>
#include <unistd.h>
#include <sys/timeb.h>

unsigned long int getMilliCount() {
	timeb tb;
	ftime(&tb);
	unsigned long int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;
}

static int mainWindow;
static bool windowVisible;
static int screenWidth = 800, screenHeight = 600;
static float screenAspect;

static bool leftMouseClick, leftMouseDoubleClick, middleMouseClick, rightMouseClick;
static int mousePosX, mousePosY;
static bool leftMouseMaybeDoubleClick;
static unsigned long int leftMouseClickTimeMs;

static const int DoubleClickClockMaxTime = 400;

static bool changing = false;
static int selElement;
static std::string selElementName;

static const int VIEWPORT_COLS = 2;
static const int VIEWPORT_ROWS = 2;
static GLint viewports[VIEWPORT_COLS * VIEWPORT_ROWS][4];
static int selViewport;

float objPos[VIEWPORT_COLS * VIEWPORT_ROWS][3];
float objRot[VIEWPORT_COLS * VIEWPORT_ROWS][16];

static const int ITERATIONS_TO_SOLVE = 20;

static const float IDENTITY_MATRIX[16] = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f };

static const float XYZ_TO_XZY_MATRIX[16] = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 1.f };

static const float XYZ_TO_ZYX_MATRIX[16] = {
	0.f, 0.f, 1.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	1.f, 0.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 1.f };

static GLUI *gluiSidePanel, *gluiBottomPanel;

std::map<std::string, vector<Segment*> > readSkeletonFile(const std::string &filename);

std::string skeletonFilename;

static std::map<std::string, std::vector<Segment*> > armsSegments;
static std::map<std::string, Arm> arms;
static std::map<std::string, Segment*> bones;
static std::map<int, std::string> SegmentNames;

static std::vector<std::array<Point3f, 2> > refSegmentLines;

float skelMinY = INFINITY;
float skelMaxY = -INFINITY;

static Point3f goal;

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

  glEnable(GL_LIGHTING);
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
	glBegin(GL_POLYGON);
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

static void drawGrid(int x0, int z0, int x1, int z1, float y) {
	glColor3f ( 0.0f, 0.0f, 0.0f );

	glBegin(GL_LINES);
		for (int i=z0; i<=z1; i++) { // Horizontal lines
			glVertex3f(x0, y, i);
			glVertex3f(x1, y, i);
		}
		for (int i=x0; i<=x1; i++) { // Vertical lines
			glVertex3f(i, y, z0);
			glVertex3f(i, y, z1);
		}
	glEnd();
}

static void drawRefLines() {
	glColor3f ( 0.0f, 0.0f, 0.0f );

	glBegin(GL_LINES);
	for (auto const & line: refSegmentLines) {
		glVertex3f(line[0][0], line[0][1], line[0][2]);
		glVertex3f(line[1][0], line[1][1], line[1][2]);
	}
	glEnd();
}

static void drawSkeleton(bool pick=false) {
	static const int sphere_segs = 8;
	glColor3f ( 1.0f, 1.0f, 1.0f );

	if (!pick) glLoadName(0);

	for (auto it = arms.begin(); it != arms.end(); it++) {
		std::string key = it->first;
		Arm & arm = it->second;
		arm.update_points();
		//~ if (!pick) arm.draw();
	}

	if (!pick) {
		glColor3f ( 1.0f, 1.0f, 0.0f );
		glPushMatrix();
			glTranslatef(0.0f, 0.0f, 0.0f);
			glutSolidSphere(.06, sphere_segs, sphere_segs);
		glPopMatrix();

		for (auto it = bones.begin(); it != bones.end(); it++) {
			std::string key = it->first;
			Segment* & seg = it->second;
			if (seg) {
				if (seg->get_blocked())
					glColor3f ( 0.5f, 0.0f, 0.2f );
				else
					glColor3f ( 0.0f, 0.5f, 0.2f );

				Point3f end_point = seg->draw();

				if (seg->get_blocked())
					glColor3f ( 1.0f, 0.0f, 0.0f );
				else
					glColor3f ( 0.0f, 1.0f, 0.0f );

				glPushMatrix();
					glTranslatef(end_point[0], end_point[1], end_point[2]);
					glutSolidSphere(.05, sphere_segs, sphere_segs);
				glPopMatrix();

				//~ glEnable(GL_LIGHTING);
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

	if (!pick) {
		if (selElement && selElementName.size()) {
			glColor3f ( 1.0f, 1.0f, 1.0f );

			glPushMatrix();
				glTranslatef(goal[0], goal[1], goal[2]);
				glutSolidSphere(.02, sphere_segs, sphere_segs);
			glPopMatrix();
		}
	}
}

static void updateSkeleton() {
    if (selElement && selElementName.size()) {
        arms[selElementName].solve(goal, ITERATIONS_TO_SOLVE);
        glutPostRedisplay();
    }
}

static void setUpSkeleton() {
    skeletonFilename = "skeletons/human.csv";
    std::map<std::string, vector<Segment*> > armsSegments = readSkeletonFile(skeletonFilename);

    for (auto it = armsSegments.begin(); it != armsSegments.end(); it++) {
        std::string key = it->first;
        std::vector<Segment*> & segs = it->second;
        arms[key].set_segments(segs);

        if (it->second.size()) {
            Segment * seg = it->second.back();
            bones[key] = seg;
            SegmentNames[seg->get_segment_id()] = key;
        } else {
            bones[key] = nullptr;
        }
    }

    for (auto it = arms.begin(); it != arms.end(); it++) {
        std::string key = it->first;
        Arm & arm = it->second;
        arm.update_points();
    }

    skelMinY = INFINITY;
    skelMaxY = -INFINITY;

    for (auto it = bones.begin(); it != bones.end(); it++) {
        std::string key = it->first;
        Segment* & seg = it->second;
        if (seg) {
            Point3f start_point = seg->get_start_point();
            skelMinY = fmin(skelMinY, start_point[1]);
            skelMaxY = fmax(skelMaxY, start_point[1]);
            Point3f end_point = start_point + seg->get_end_point();
            skelMinY = fmin(skelMinY, end_point[1]);
            skelMaxY = fmax(skelMaxY, end_point[1]);

            std::array<Point3f, 2> line = {start_point, end_point};

            refSegmentLines.push_back(line);
        }
    }
}


void changeState();

void drawScene(bool pick=false) {
  glLoadName(0);
  drawAxes();

  //~ drawCube();

  if (!pick) drawGrid(-10, -10, 10, 10, skelMinY);
  if (!pick) drawRefLines();

  drawSkeleton(pick);
}

void ViewportScissor(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
    glScissor(x, y, width, height);
}

static void adjustViewMatrices(int vp_num) {
	glMatrixMode(GL_PROJECTION);
	// Don't call glLoadIdentity!!
	switch (vp_num) {
		case 0: // Viewport 0 (Down-Left)
			gluPerspective(45, screenAspect, 0.1, 100.0);
			break;
		case 1: // Viewport 1 (Down-Right)
			gluPerspective(45, screenAspect, 0.1, 100.0);
			break;
		case 2: // Viewport 2 (Up-Left)
			gluPerspective(45, screenAspect, 0.1, 100.0);
			break;
		case 3: // Viewport 3 (Up-Right)
			gluPerspective(45, screenAspect, 0.1, 100.0);
			break;
		default:
			break;
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	switch (vp_num) {
		case 0: // Viewport 0 (Down-Left)
			glTranslatef(objPos[0][0], objPos[0][1], -objPos[0][2]);
			glRotatef(90, 1.f, 0.f, 0.f);
			glMultMatrixf(objRot[0]);
			break;
		case 1: // Viewport 1 (Down-Right)
			glTranslatef(objPos[1][0], objPos[1][1], -objPos[1][2]);
			glMultMatrixf(objRot[1]);
			break;
		case 2: // Viewport 2 (Up-Left)
			glTranslatef(objPos[2][0], objPos[2][1], -objPos[2][2]);
			glMultMatrixf(objRot[2]);
			break;
		case 3: // Viewport 3 (Up-Right)
			glTranslatef(objPos[3][0], objPos[3][1], -objPos[3][2]);
			glRotatef(90, 0.f, 1.f, 0.f);
			glMultMatrixf(objRot[3]);
			break;
		default:
			break;
	}
}

void displayHandler() {
	glDisable(GL_SCISSOR_TEST);
	glClearColor( .9f, .9f, .9f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glEnable(GL_SCISSOR_TEST);

	// Perspective view: Viewport 1 (Down-Right)
	GLint (&vp)[4] = viewports[1];
	ViewportScissor(vp[0], vp[1], vp[2], vp[3]);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	adjustViewMatrices(1);
	drawScene();

	// View from above (Y): Viewport 0 (Down-Left)
	GLint (&vp_y)[4] = viewports[0];
	ViewportScissor(vp_y[0], vp_y[1], vp_y[2], vp_y[3]);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	adjustViewMatrices(0);
	drawScene();

	// View from front (Z): Viewport 2 (Up-Left)
	GLint (&vp_z)[4] = viewports[2];
	ViewportScissor(vp_z[0], vp_z[1], vp_z[2], vp_z[3]);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	adjustViewMatrices(2);
	drawScene();

	// View from side (X): Viewport 3 (Up-Right)
	GLint (&vp_x)[4] = viewports[3];
	ViewportScissor(vp_x[0], vp_x[1], vp_x[2], vp_x[3]);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	adjustViewMatrices(3);
	drawScene();

	glutSwapBuffers();
}

int selectElement(GLint viewport[4], int mousex, int mousey) {
  long hits;
  GLuint selectBuf[1024];
  GLuint closest;
  GLuint dist;

  glSelectBuffer(1024, selectBuf);
  glRenderMode(GL_SELECT);
  glInitNames();

  glPushName(~0U);

  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPickMatrix(mousex, screenHeight - mousey, 4, 4, viewport);
  adjustViewMatrices(selViewport);
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

  memcpy(inverse, IDENTITY_MATRIX, sizeof(IDENTITY_MATRIX));

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

static bool computeXYCoords(GLint viewport[4], int mousex, int mousey, GLfloat * selx, GLfloat * sely, GLfloat h = 0.0f, const float adjMatrix[16] = IDENTITY_MATRIX) {
  GLfloat projMatrix[16];
  glGetFloatv(GL_PROJECTION_MATRIX, projMatrix);

  GLfloat modelMatrix[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);

  GLfloat finalMatrix[16];
  mult(modelMatrix, projMatrix, finalMatrix);

  GLfloat m[16];
  mult(adjMatrix, finalMatrix, m);
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

static void reshapeHandler(int width, int height) {
  screenWidth = width;
  screenHeight = height;
  screenAspect = (float)width / (float)height;

  for (int y = 0; y < VIEWPORT_ROWS; y++)
    for (int x = 0; x < VIEWPORT_COLS; x++) {
      GLint (&viewport)[4] = viewports[x + VIEWPORT_COLS*y];
      viewport[0] = x * width / VIEWPORT_COLS;
      viewport[1] = y * height / VIEWPORT_ROWS;
      viewport[2] = width / VIEWPORT_COLS;
      viewport[3] = height / VIEWPORT_ROWS;
  }

  //~ glViewport(0, 0, screenWidth, screenHeight);
  //~ glGetIntegerv(GL_VIEWPORT, viewports[0]);
}

static void keyboardHandler(unsigned char c, int x, int y) {
  switch(c) {
    case 27:
    case 'q':
      exit(0);
      break;
    };

  glutPostRedisplay();
}

static void specialHandler(int key, int x, int y) {
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

static void motionHandler(int x, int y) {
  if (leftMouseClick && selElement) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	adjustViewMatrices(selViewport);

	float selx, sely;

	unsigned long int timeMs = getMilliCount(); // To get the number of seconds used, divide by CLOCKS_PER_SEC.

	switch (selViewport) {
		case 0: // Viewport 0 (Down-Left): View from above (Y)
			computeXYCoords(viewports[selViewport], x, y, &selx, &sely, goal[1], XYZ_TO_XZY_MATRIX);
			goal[0] = selx;
			goal[2] = sely;
			break;
		case 1: // Viewport 1 (Down-Right): Perspective view
			break;
		case 2: // Viewport 2 (Up-Left): View from front (Z)
			computeXYCoords(viewports[selViewport], x, y, &selx, &sely, goal[2], IDENTITY_MATRIX);
			goal[0] = selx;
			goal[1] = sely;
			break;
		case 3: // Viewport 3 (Up-Right): View from side (X)
			computeXYCoords(viewports[selViewport], x, y, &selx, &sely, goal[0], XYZ_TO_ZYX_MATRIX);
			goal[2] = selx;
			goal[1] = sely;
			break;
		default:
			break;
	}

	if (timeMs - leftMouseClickTimeMs > DoubleClickClockMaxTime)
		updateSkeleton();
  }

  mousePosX = x;
  mousePosY = y;
  glutPostRedisplay();
}

static void mouseHandler(int button, int state, int x, int y) {
	printf ("GLUT: ");

	mousePosX = x;
	mousePosY = y;

	// This function will return the same value approximately every 72 minutes.
	// POSIX requires that CLOCKS_PER_SEC equals 1000000 independent of the actual resolution.
	unsigned long int timeMs = getMilliCount(); // To get the number of seconds used, divide by CLOCKS_PER_SEC.

	switch (button) {
		case GLUT_LEFT_BUTTON:
			switch (state) {
				case GLUT_DOWN:
					printf ("Mouse Left Button Pressed (Down)...\n");
					leftMouseClick = true;

					selViewport = (x * VIEWPORT_COLS / screenWidth) + VIEWPORT_COLS * (VIEWPORT_ROWS - 1 - (y * VIEWPORT_ROWS / screenHeight));
					printf("Viewport: %d\n", selViewport);
					selElement = selectElement(viewports[selViewport], mousePosX, mousePosY);
					if (selElement) {
						selElementName = SegmentNames[selElement];
						printf("Selected: %d (%s)\n", selElement, selElementName.c_str());
						goal = bones[selElementName]->get_start_point() + bones[selElementName]->get_end_point();

					} else {
						selElementName = "";
					}

					if (!leftMouseMaybeDoubleClick || timeMs - leftMouseClickTimeMs > DoubleClickClockMaxTime) {
						leftMouseMaybeDoubleClick = true;
						leftMouseClickTimeMs = timeMs;
					} else {
						//~ printf ("[[ Mouse Left Button Double Click: %ld ]]\n", timeMs - leftMouseClickTimeMs);

						if (selElement && selElementName.size()) {
							bones[selElementName]->set_blocked(!bones[selElementName]->get_blocked());
							printf ("[[ Bone %d (%s) set to %s ]]\n", selElement, selElementName.c_str(), bones[selElementName]->get_blocked() ? "blocked" : "not blocked");

							for (auto it = arms.begin(); it != arms.end(); it++) {
								std::string key = it->first;
								Arm & arm = it->second;
								arm.update_segments();
							}
						}

						leftMouseClick = false;
						leftMouseDoubleClick = true;
						leftMouseMaybeDoubleClick = false;
					}

					break;
				case GLUT_UP:
					printf ("Mouse Left Button Released (Up)...\n");
					leftMouseClick = leftMouseDoubleClick = false;
					selViewport = 0;
					selElement = 0;
					break;
			}
			glutPostRedisplay();
			break;
		case GLUT_MIDDLE_BUTTON:
			switch (state) {
				case GLUT_DOWN:
					printf ("Mouse Middle Button Pressed (Down)...\n");
					middleMouseClick = true;
					break;
				case GLUT_UP:
					printf ("Mouse Middle Button Released (Up)...\n");
					middleMouseClick = false;
					break;
			}
			glutPostRedisplay();
			break;
		case GLUT_RIGHT_BUTTON:
			switch (state) {
				case GLUT_DOWN:
					printf ("Mouse Right Button Pressed (Down)...\n");
					rightMouseClick = true;
					break;
				case GLUT_UP:
					printf ("Mouse Right Button Released (Up)...\n");
					rightMouseClick = false;
					break;
			}
			glutPostRedisplay();
			break;
	}

	motionHandler(x, y);
}

static void idleHandler() {
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

static void init() {
  static const GLfloat lmodel_ambient[]      = { 0.0, 0.0, 0.0, 0.0 };
  static const GLfloat lmodel_twoside[]      = { GL_FALSE };
  static const GLfloat lmodel_local[]        = { GL_FALSE };

  //~ static const GLfloat light0_ambient[]      = { 0.2, 0.2, 0.2, 1.0 };
  //~ static const GLfloat light0_diffuse[]      = { 1.0, 1.0, 1.0, 0.0 };
  //~ static const GLfloat light0_position[]     = {.5f, .5f, 1.0f, 0.0f};

  static const GLfloat light0_ambient[]      = { 0.1f, 0.1f, 0.1f, 1.0f };
  static const GLfloat light0_diffuse[]      = { 1.0f, 1.0f, 1.0f, 0.0f };
  static const GLfloat light0_position[]     = { .5f, .5f, 1.0f, 0.0f };

  static const GLfloat light1_ambient[]      = { 0.1f, 0.1f, 0.1f, 1.0f };
  static const GLfloat light1_diffuse[]      = { 1.0f, 1.0f, 1.0f, 0.0f };
  static const GLfloat light1_position[]     = { -1.0f, -1.0f, 1.0f, 0.0f };

  static const GLfloat bevel_mat_ambient[]   = { 0.0, 0.0, 0.0, 1.0 };
  static const GLfloat bevel_mat_shininess[] = { 40.0 };
  static const GLfloat bevel_mat_specular[]  = { 0.0, 0.0, 0.0, 0.0 };
  static const GLfloat bevel_mat_diffuse[]   = { 1.0, 0.0, 0.0, 0.0 };

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0);

  glClearColor(0.5, 0.5, 0.5, 0.0);
  glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
  glEnable(GL_LIGHT0);

  glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_local);
  glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glEnable(GL_LIGHTING);

  glMaterialfv(GL_FRONT, GL_AMBIENT, bevel_mat_ambient);
  glMaterialfv(GL_FRONT, GL_SHININESS, bevel_mat_shininess);
  glMaterialfv(GL_FRONT, GL_SPECULAR, bevel_mat_specular);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, bevel_mat_diffuse);

  glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  glShadeModel(GL_FLAT);

  leftMouseClick = leftMouseDoubleClick = middleMouseClick = rightMouseClick = leftMouseMaybeDoubleClick = false;
  leftMouseClickTimeMs = 0;

	selElement = 0;
	selElementName = "";
	selViewport = 0;
	memset(viewports, 0, sizeof(viewports));

	memset(objPos, 0, sizeof(objPos));
	memset(objRot, 0, sizeof(objRot));
}

static void Usage() {
  exit(-1);
}

static void visibilityHandler(int v) {
  if (v == GLUT_VISIBLE) {
    windowVisible = true;
  } else {
    windowVisible = false;
  }
  changeState();
}

static void menuHandler(int choice) {
	switch(choice) {
		case 1:
			exit(0);
			break;
	}
}

static bool save() {
	char const * filename;
	char const * filter_patterns[2] = { "*.txt", "*.text" };

	filename = tinyfd_saveFileDialog("select filename to save", "bones.txt", 2, filter_patterns, NULL);

	if (!filename) {
		tinyfd_messageBox("Error", "Save file name is NULL", "ok", "error", 1);
		return false;
	}

	FILE * file_handler;
#ifdef _WIN32
	if (tinyfd_winUtf8)
		file_handler = _wfopen(tinyfd_utf8to16(filename), L"w"); /* the UTF-8 filename is converted to UTF-16 to open the file*/
	else
#endif
	file_handler = fopen(filename, "w");

	if (!file_handler) {
		tinyfd_messageBox("Error", "Can not open this file in write mode", "ok", "error", 1);
		return false;
	}

	fputs("Data: pose\n", file_handler);
	fprintf(file_handler, "Skeleton: %s\n", skeletonFilename.c_str());

    for (auto it = bones.begin(); it != bones.end(); it++) {
        std::string key = it->first;
        Segment* & seg = it->second;
        if (seg) {
            Vector3f axis = seg->get_axis();
            float angle = seg->get_angle();
            float magnitude = seg->get_mag();
            Point3f begin = seg->get_start_point();
            Point3f end = begin + seg->get_end_point();
            fprintf(file_handler, "Bone: \"%s\"; Parent: \"%s\"; Axis: %f, %f, %f; Angle: %f; Magnitude: %f; Begin: %f, %f, %f; End: %f, %f, %f\n",
                seg->name.c_str(),
                seg->parent_name.c_str(),
                axis[0], axis[1], axis[2],
                angle,
                magnitude,
                begin[0], begin[1], begin[2],
                end[0], end[1], end[2]
            );
        } else {
            fprintf(file_handler, "Root: \"%s\"\n", key.c_str());
        }
    }

	fclose(file_handler);

	return true;
}


enum {
	SAVE_BUTTON,
	QUIT_BUTTON,
};

static void gluiControlCallback(int control_id) {
	switch (control_id) {
		case SAVE_BUTTON:
			save();
			break;
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

  gluiSidePanel = GLUI_Master.create_glui_subwindow(mainWindow, GLUI_SUBWINDOW_RIGHT);

  // Quit button
  gluiSidePanel->add_button ("Save", SAVE_BUTTON, gluiControlCallback);
  gluiSidePanel->add_button ("Quit", QUIT_BUTTON, gluiControlCallback);

  // Link window to GLUI
  gluiSidePanel->set_main_gfx_window(mainWindow);

  // Bottom subwindow

  gluiBottomPanel = GLUI_Master.create_glui_subwindow(mainWindow, GLUI_SUBWINDOW_BOTTOM);
  gluiBottomPanel->set_main_gfx_window(mainWindow);

  GLUI_Translation *t = nullptr;
  GLUI_Rotation *r = nullptr;

  // Viewport 0: Up

  objPos[0][0] = objPos[0][1] = 0.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Up XY", GLUI_TRANSLATION_XY, objPos[0]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  objPos[0][2] = 3.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Up Z", GLUI_TRANSLATION_Z, &objPos[0][2]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  memcpy(objRot[0], IDENTITY_MATRIX, sizeof(IDENTITY_MATRIX));
  r = new GLUI_Rotation(gluiBottomPanel, "Up Rot", objRot[0] );
  r->set_spin( 1.0 );

  new GLUI_Column( gluiBottomPanel, false );

  // Viewport 2: Front

  objPos[2][0] = objPos[2][1] = 0.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Front XY", GLUI_TRANSLATION_XY, objPos[2]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  objPos[2][2] = 3.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Front Z", GLUI_TRANSLATION_Z, &objPos[2][2]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  memcpy(objRot[2], IDENTITY_MATRIX, sizeof(IDENTITY_MATRIX));
  r = new GLUI_Rotation(gluiBottomPanel, "Front Rot", objRot[2] );
  r->set_spin( 1.0 );

  new GLUI_Column( gluiBottomPanel, false );

  // Viewport 3: Side

  objPos[3][0] = objPos[3][1] = 0.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Side XY", GLUI_TRANSLATION_XY, objPos[3]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  objPos[3][2] = 3.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Side Z", GLUI_TRANSLATION_Z, &objPos[3][2]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  memcpy(objRot[3], IDENTITY_MATRIX, sizeof(IDENTITY_MATRIX));
  r = new GLUI_Rotation(gluiBottomPanel, "Side Rot", objRot[3] );
  r->set_spin( 1.0 );

  new GLUI_Column( gluiBottomPanel, false );

  // Viewport 1: Perspective

  objPos[1][0] = objPos[1][1] = 0.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Persp XY", GLUI_TRANSLATION_XY, objPos[1]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  objPos[1][2] = 3.0f;
  t = new GLUI_Translation(gluiBottomPanel, "Persp Z", GLUI_TRANSLATION_Z, &objPos[1][2]);
  t->set_speed( .005 );

  new GLUI_Column( gluiBottomPanel, false );

  memcpy(objRot[1], IDENTITY_MATRIX, sizeof(IDENTITY_MATRIX));
  r = new GLUI_Rotation(gluiBottomPanel, "Persp Rot", objRot[1] );
  r->set_spin( 1.0 );

  new GLUI_Column( gluiBottomPanel, false );

  // We register the idle callback with GLUI, *not* with GLUT
  GLUI_Master.set_glutIdleFunc(idleHandler);

  glutMainLoop();
  return EXIT_SUCCESS;
}
