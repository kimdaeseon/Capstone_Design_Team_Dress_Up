#include "Renderer.h"
#include "SocketServer.h"
#include "ObjParser.h"
#include "stb_image.h"

#include <vector>
#include <sstream>
#include <fstream>
#include <cmath>

ObjParser objParser = ObjParser();
void draw_center(void)
{
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f); /* R */
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.2f, 0.0f, 0.0f);
	glEnd();
	glRasterPos3f(0.2f, 0.0f, 0.0f);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'x');

	glBegin(GL_LINES);
	glColor3f(0.0f, 1.0f, 0.0f); /* G */
	glVertex3f(0.0f, 0.2f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();
	glRasterPos3f(0.0f, 0.2f, 0.0f);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'y');

	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 1.0f); /* B */
	glVertex3f(0.0f, 0.0f, -0.2f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();
	glRasterPos3f(0.0f, 0.0f, -0.2f);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'z');
}

void idle() {
	static GLuint previousClock = glutGet(GLUT_ELAPSED_TIME);
	static GLuint currentClock = glutGet(GLUT_ELAPSED_TIME);
	static GLfloat deltaT;

	currentClock = glutGet(GLUT_ELAPSED_TIME);
	deltaT = currentClock - previousClock;
	if (deltaT < 1000.0 / 20.0) { return; }
	else { previousClock = currentClock; }

	//char buff[256];
	//sprintf_s(buff, "Frame Rate = %f", 1000.0 / deltaT);
	//frameRate = buff;

	glutPostRedisplay();
}

void close()
{
	glDeleteTextures(1, &dispBindIndex);
	glutLeaveMainLoop();
	CloseHandle(hMutex);
}

void add_quats(float q1[4], float q2[4], float dest[4])
{
	static int count = 0;
	float t1[4], t2[4], t3[4];
	float tf[4];

	vcopy(q1, t1);
	vscale(t1, q2[3]);

	vcopy(q2, t2);
	vscale(t2, q1[3]);

	vcross(q2, q1, t3);
	vadd(t1, t2, tf);
	vadd(t3, tf, tf);
	tf[3] = q1[3] * q2[3] - vdot(q1, q2);

	dest[0] = tf[0];
	dest[1] = tf[1];
	dest[2] = tf[2];
	dest[3] = tf[3];

	if (++count > RENORMCOUNT) {
		count = 0;
		normalize_quat(dest);
	}
}

void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(58, (double)width / height, 0.1, 100);
	glMatrixMode(GL_MODELVIEW);
}

void motion(int x, int y)
{
	GLfloat spin_quat[4];
	float gain;
	gain = 2.0; /* trackball gain */

	if (drag_state == GLUT_DOWN)
	{
		if (button_state == GLUT_LEFT_BUTTON)
		{
			trackball(spin_quat,
				(gain * rot_x - 500) / 500,
				(500 - gain * rot_y) / 500,
				(gain * x - 500) / 500,
				(500 - gain * y) / 500);
			add_quats(spin_quat, quat, quat);
		}
		else if (button_state == GLUT_RIGHT_BUTTON)
		{
			t[0] -= (((float)trans_x - x) / 500);
			t[1] += (((float)trans_y - y) / 500);
		}
		else if (button_state == GLUT_MIDDLE_BUTTON)
			t[2] -= (((float)trans_z - y) / 500 * 4);
		else if (button_state == 3 || button_state == 4) // scroll
		{

		}
		//glutPostRedisplay();
	}

	rot_x = x;
	rot_y = y;

	trans_x = x;
	trans_y = y;
	trans_z = y;
}

void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		if (button == GLUT_LEFT_BUTTON)
		{
			rot_x = x;
			rot_y = y;

			//t[0] = t[0] + 1;


		}
		else if (button == GLUT_RIGHT_BUTTON)
		{
			trans_x = x;
			trans_y = y;
		}
		else if (button == GLUT_MIDDLE_BUTTON)
		{
			//trcon = trcon + 1;
			trans_z = y;
		}
		else if (button == 3 || button == 4)
		{
			const float sign = (static_cast<float>(button) - 3.5f) * 2.0f;
			t[2] -= sign * 500 * 0.00015f;
		}
	}

	drag_state = state;
	button_state = button;
}

void vzero(float* v)
{
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 0.0f;
}

void vset(float* v, float x, float y, float z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

void vsub(const float* src1, const float* src2, float* dst)
{
	dst[0] = src1[0] - src2[0];
	dst[1] = src1[1] - src2[1];
	dst[2] = src1[2] - src2[2];
}

void vcopy(const float* v1, float* v2)
{
	register int i;
	for (i = 0; i < 3; i++)
		v2[i] = v1[i];
}

void vcross(const float* v1, const float* v2, float* cross)
{
	float temp[3];

	temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
	temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
	temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
	vcopy(temp, cross);
}

float vlength(const float* v)
{
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void vscale(float* v, float div)
{
	v[0] *= div;
	v[1] *= div;
	v[2] *= div;
}

void vnormal(float* v)
{
	vscale(v, 1.0f / vlength(v));
}

float vdot(const float* v1, const float* v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void vadd(const float* src1, const float* src2, float* dst)
{
	dst[0] = src1[0] + src2[0];
	dst[1] = src1[1] + src2[1];
	dst[2] = src1[2] + src2[2];
}

void trackball(float q[4], float p1x, float p1y, float p2x, float p2y)
{
	float a[3]; /* Axis of rotation */
	float phi;  /* how much to rotate about axis */
	float p1[3], p2[3], d[3];
	float t;

	if (p1x == p2x && p1y == p2y) {
		/* Zero rotation */
		vzero(q);
		q[3] = 1.0;
		return;
	}

	/*
	 * First, figure out z-coordinates for projection of P1 and P2 to
	 * deformed sphere
	 */
	vset(p1, p1x, p1y, tb_project_to_sphere(TRACKBALLSIZE, p1x, p1y));
	vset(p2, p2x, p2y, tb_project_to_sphere(TRACKBALLSIZE, p2x, p2y));

	/*
	 *  Now, we want the cross product of P1 and P2
	 */
	vcross(p2, p1, a);

	/*
	 *  Figure out how much to rotate around that axis.
	 */
	vsub(p1, p2, d);
	t = vlength(d) / (2.0f * TRACKBALLSIZE);

	/*
	 * Avoid problems with out-of-control values...
	 */
	if (t > 1.0) t = 1.0;
	if (t < -1.0) t = -1.0;
	phi = 2.0f * asin(t);

	axis_to_quat(a, phi, q);
}

void axis_to_quat(float a[3], float phi, float q[4])
{
	vnormal(a);
	vcopy(a, q);
	vscale(q, sin(phi / 2.0f));
	q[3] = cos(phi / 2.0f);
}

float tb_project_to_sphere(float r, float x, float y)
{
	float d, t, z;

	d = sqrt(x * x + y * y);
	if (d < r * 0.70710678118654752440f) {    /* Inside sphere */
		z = sqrt(r * r - d * d);
	}
	else {           /* On hyperbola */
		t = r / 1.41421356237309504880f;
		z = t * t / d;
	}
	return z;
}

void normalize_quat(float q[4])
{
	int i;
	float mag;

	mag = (q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	for (i = 0; i < 4; i++) q[i] /= mag;
}

void build_rotmatrix(float m[4][4], float q[4])
{
	m[0][0] = 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
	m[0][1] = 2.0f * (q[0] * q[1] - q[2] * q[3]);
	m[0][2] = 2.0f * (q[2] * q[0] + q[1] * q[3]);
	m[0][3] = 0.0f;

	m[1][0] = 2.0f * (q[0] * q[1] + q[2] * q[3]);
	m[1][1] = 1.0f - 2.0f * (q[2] * q[2] + q[0] * q[0]);
	m[1][2] = 2.0f * (q[1] * q[2] - q[0] * q[3]);
	m[1][3] = 0.0f;

	m[2][0] = 2.0f * (q[2] * q[0] - q[1] * q[3]);
	m[2][1] = 2.0f * (q[1] * q[2] + q[0] * q[3]);
	m[2][2] = 1.0f - 2.0f * (q[1] * q[1] + q[0] * q[0]);
	m[2][3] = 0.0f;

	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

void InitializeWindow(int argc, char* argv[])
{
	// initialize glut settings
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowSize(1000 / 2, 1000 / 2);

	glutInitWindowPosition(0, 0);

	dispWindowIndex = glutCreateWindow("3D Model");

	trackball(quat, 90.0, 0.0, 0.0, 0.0);

	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special);
	glutMotionFunc(motion);
	glutMouseFunc(mouse);
	glutCloseFunc(close);
	//GLuint image = load   ("./my_texture.bmp");

	//glBindTexture(1,)

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// bind textures
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	reshape(1000, 1000);

	/*glGenTextures(1, &dispBindIndex);
	glBindTexture(GL_TEXTURE_2D, dispBindIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
}


float squareDistance(Vertex vertex1, Vertex vertex2) {
	float differX = vertex1.X - vertex2.X;
	float differY = vertex1.Y - vertex2.Y;
	float differZ = vertex1.Z - vertex2.Z;
	return differX * differX + differY * differY + differZ * differZ;
}

bool isTop(Vertex vertex, vector<Vertex> skeleton) {
	Vertex leftWrist = skeleton[7];
	Vertex rightWrist = skeleton[8];
	Vertex neck = skeleton[6];
	Vertex waist = skeleton[5];

	float distanceOfWrist = squareDistance(leftWrist, rightWrist);
	float distanceOfWaistAndNeck = squareDistance(neck, waist);

	float distanceOfLeftWristAndVertex = squareDistance(leftWrist, vertex);
	float distanceOfRightWristAndVertex = squareDistance(rightWrist, vertex);

	float distanceOfNeckAndVertex = squareDistance(neck, vertex);
	float distanceOfWaistAndVertex = squareDistance(waist, vertex);

	if (distanceOfLeftWristAndVertex > distanceOfRightWristAndVertex) {
		if (distanceOfRightWristAndVertex + distanceOfWrist < distanceOfLeftWristAndVertex) return false;
	}
	else {
		if (distanceOfLeftWristAndVertex + distanceOfWrist < distanceOfRightWristAndVertex) return false;
	}


	if (distanceOfNeckAndVertex > distanceOfWaistAndVertex) {
		if (distanceOfWaistAndVertex + distanceOfWaistAndNeck < distanceOfNeckAndVertex) return false;
	}
	else {
		if (distanceOfNeckAndVertex + distanceOfWaistAndNeck < distanceOfWaistAndVertex) return false;
	}

	return true;
}

/*
가운데 골반 점
왼쪽 골반 점
오른쪽 골반 점
왼쪽 발목점
오른쪽 발목점

허리점
목점
왼쪽 손목
오른쪽 손목
*/

bool isBottom(Vertex vertex, vector<Vertex> skeleton) {
	Vertex middlePelvis = skeleton[0];
	Vertex leftPelvis = skeleton[2];
	Vertex rightPelvis = skeleton[1];

	Vertex leftAnkle = skeleton[3];
	Vertex rightAnkle = skeleton[4];

	float weight = 0.6;

	float rangeOfPelvis = abs(leftPelvis.X - rightPelvis.X);

	float leftLimit = leftPelvis.X - rangeOfPelvis * weight;
	float rightLimit = rightPelvis.X + rangeOfPelvis * weight;

	float topLimit = (leftPelvis.Y > rightPelvis.Y) ? leftPelvis.Y : rightPelvis.Y;
	float bottomLimit = (leftAnkle.Y < rightAnkle.Y) ? leftAnkle.Y : rightAnkle.Y;

	if (vertex.X >= leftLimit && vertex.X <= rightLimit) {
		if (vertex.Y >= bottomLimit && vertex.Y <= topLimit) {
			return true;
		}
	}

	return false;
}

Vertex crossDot(Vertex v1, Vertex v2) {
	float v11 = v1.X;
	float v12 = v1.Y;
	float v13 = v1.Z;

	float v21 = v2.X;
	float v22 = v2.Y;
	float v23 = v2.Z;

	Vertex result;
	result.X = v12 * v23 - v13 * v22;
	result.Y = v13 * v21 - v11 * v23;
	result.Z = v11 * v22 - v12 * v21;

	return result;
}

Vertex calculateNormal(Vertex v1, Vertex v2, Vertex v3) {
	Vertex va, vb;

	va.X = v2.X - v1.X;
	va.Y = v2.Y - v1.Y;
	va.Z = v2.Z - v1.Z;

	vb.X = v3.X - v1.X;
	vb.Y = v3.Y - v1.Y;
	vb.Z = v3.Z - v1.Z;

	return crossDot(va, vb);
}

GLfloat* getAmbient(Vertex color) {
	GLfloat result[4] = { color.X, color.Y, color.Z, 1 };
	return result;
}
GLfloat* getDiffuse(Vertex color) {
	GLfloat result[4] = { color.X, color.Y, color.Z, 1 };
	return result;
}
GLfloat* getSpecular(Vertex color) {
	GLfloat result[4] = { color.X, color.Y, color.Z, 1 };
	return result;
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1, 0.1, 200);
	glTranslatef(t[0], t[1], t[2] - 1.0f);
	glScalef(1, 1, 1);
	GLfloat m[4][4], m1[4][4];
	build_rotmatrix(m, quat);
	gluLookAt(0, 2.0, 2.0, 0, 0, 0, 0, 1.0, 0);

	GLfloat r, g, b;
	glMultMatrixf(&m[0][0]);


	// test #1
	glEnable(GL_LIGHTING);

	glEnable(GL_LIGHT0);
	GLfloat diffuse0[4] = { 0.9, 0.9, 0.9, 100.0 };
	GLfloat ambient0[4] = { 0.5, 0.5, 0.5, 100.0 };
	GLfloat specular0[4] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light0_pos[4] = { -2.0, 2.0, 2.0, 1.0 };
	//GLfloat light0_pos[4] = { 0.3, 0.3, 0.5, 1.0 };
	//GLfloat spot_dir[3] = { -2.0f, 0.0f, -1.0f };

	glEnable(GL_LIGHT1);
	GLfloat diffuse1[4] = { 1.0, 1.0, 1.0, 10.0 };
	GLfloat ambient1[4] = { 0.5, 0.5, 0.5, 10.0 };
	GLfloat specular1[4] = { 1.0, 1.0, 1.0, 10.0 };
	GLfloat light1_pos[4] = { -2.0, 2.0, -2.0, 10.0 };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular0);
	//glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_dir);
	//glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 50.0);
	//glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 1.0);

	glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, specular1);



	// test #2

	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.2);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.1);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.05);


	// test #3

	glShadeModel(GL_SMOOTH);
	//glShadeModel(GL_FLAT);

	// test #4
	//빨간색 플라스틱과 유사한 재질을 다음과 같이 정의
	GLfloat mat_ambient[4] = { 1, 1, 1, 100 };
	GLfloat mat_diffuse[4] = { 1, 1, 1, 100.0f };
	GLfloat mat_specular[4] = { 1, 1, 1, 100.0f };
	GLfloat mat_shininess = 256.0;
	//
	//// 폴리곤의 앞면의 재질을 설정 
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

	// 텍스처 로드 및 생성
	int width, height, nrChannels;
	unsigned char* data = stbi_load("2.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


	glEnable(GL_TEXTURE_2D);

	vector<Vertex> skeleton = objParser.skeleton;
	vector<Vertex> realVertex = objParser.realVertex;
	vector<Vertex> realColor = objParser.realColor;
	vector<Vertex> realTexture = objParser.realTexture;

	// 
	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glFrontFace(GL_CCW);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_TRIANGLES);
	
	for (register int j = 0; j < realVertex.size(); j = j + 3) {
		bool isTopFace = isTop(realVertex[j], skeleton) && isTop(realVertex[j + 1], skeleton) && isTop(realVertex[j + 2], skeleton);
		bool isBottomFace = isBottom(realVertex[j], skeleton) && isBottom(realVertex[j + 1], skeleton) && isBottom(realVertex[j + 2], skeleton);
		if (isTopFace && !isBottomFace) {
			Vertex normal = calculateNormal(realVertex[j], realVertex[j + 1], realVertex[j + 2]);

			glColor3f(1, 1, 1);
			glTexCoord2f(realTexture[j].X, realTexture[j].Y);
			glVertex3f(realVertex[j].X, realVertex[j].Y, realVertex[j].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);

			glColor3f(1, 1, 1);
			glTexCoord2f(realTexture[j + 1].X, realTexture[j + 1].Y);
			glVertex3f(realVertex[j + 1].X, realVertex[j + 1].Y, realVertex[j + 1].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);

			glColor3f(1, 1, 1);
			glTexCoord2f(realTexture[j + 2].X, realTexture[j + 2].Y);
			glVertex3f(realVertex[j + 2].X, realVertex[j + 2].Y, realVertex[j + 2].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);
		}
	}
	glEnd();

	// 텍스처 로드 및 생성
	data = stbi_load("4.jpeg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


	glEnable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLES);

	for (register int j = 0; j < realVertex.size(); j = j + 3) {
		bool isTopFace = isTop(realVertex[j], skeleton) && isTop(realVertex[j + 1], skeleton) && isTop(realVertex[j + 2], skeleton);
		bool isBottomFace = isBottom(realVertex[j], skeleton) && isBottom(realVertex[j + 1], skeleton) && isBottom(realVertex[j + 2], skeleton);
		if (!isTopFace && isBottomFace) {
			Vertex normal = calculateNormal(realVertex[j], realVertex[j + 1], realVertex[j + 2]);
			glColor3f(1, 1, 1);
			glTexCoord2f(realTexture[j].X, realTexture[j].Y);
			glVertex3f(realVertex[j].X, realVertex[j].Y, realVertex[j].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);

			glColor3f(1, 1, 1);
			glTexCoord2f(realTexture[j + 1].X, realTexture[j + 1].Y);
			glVertex3f(realVertex[j + 1].X, realVertex[j + 1].Y, realVertex[j + 1].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);

			glColor3f(1, 1, 1);
			glTexCoord2f(realTexture[j + 2].X, realTexture[j + 2].Y);
			glVertex3f(realVertex[j + 2].X, realVertex[j + 2].Y, realVertex[j + 2].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);
		}
	}
	glEnd();

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLES);

	GLfloat otherAmbient[4] = { 0.1, 0.1, 0, 1 };
	for (register int j = 0; j < realVertex.size(); j = j + 3) {
		bool isTopFace = isTop(realVertex[j], skeleton) && isTop(realVertex[j + 1], skeleton) && isTop(realVertex[j + 2], skeleton);
		bool isBottomFace = isBottom(realVertex[j], skeleton) && isBottom(realVertex[j + 1], skeleton) && isBottom(realVertex[j + 2], skeleton);
		if (!isTopFace && !isBottomFace) {
			Vertex normal = calculateNormal(realVertex[j], realVertex[j + 1], realVertex[j + 2]);

			glColor3f(realColor[j].X, realColor[j].Y, realColor[j].Z);
			glTexCoord2f(realTexture[j].X, realTexture[j].Y);
			glVertex3f(realVertex[j].X, realVertex[j].Y, realVertex[j].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);

			glColor3f(realColor[j + 1].X, realColor[j + 1].Y, realColor[j + 1].Z);
			glTexCoord2f(realTexture[j + 1].X, realTexture[j + 1].Y);
			glVertex3f(realVertex[j + 1].X, realVertex[j + 1].Y, realVertex[j + 1].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);

			glColor3f(realColor[j + 2].X, realColor[j + 2].Y, realColor[j + 2].Z);
			glTexCoord2f(realTexture[j + 2].X, realTexture[j + 2].Y);
			glVertex3f(realVertex[j + 2].X, realVertex[j + 2].Y, realVertex[j + 2].Z);
			glNormal3f(normal.X, normal.Y, normal.Z);
		}
	}
	glEnd();

	glutSwapBuffers();
}

vector<string> split(string str, char Delimiter) {
	istringstream iss(str);             // istringstream에 str을 담는다.
	string buffer;                      // 구분자를 기준으로 절삭된 문자열이 담겨지는 버퍼

	vector<string> result;

	// istringstream은 istream을 상속받으므로 getline을 사용할 수 있다.
	while (getline(iss, buffer, Delimiter)) {
		result.push_back(buffer);               // 절삭된 문자열을 vector에 저장
	}

	return result;
}



int main(int argc, char* argv[])
{
	try {
		SocketServer sock = SocketServer();
		vector<string> lines;
		string msg;
		sock.connection();
		for (;;) {
			msg = sock.recvData();
			objParser.parse(msg.c_str());
		}
		sock.close();
	}
	catch (string exception) {

		objParser.calculateFace();

		InitializeWindow(argc, argv);

		display();

		glutMainLoop();

		return 1;
	}
}
