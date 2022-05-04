#include "Renderer.h"
#include "SocketServer.h"
#include <vector>
#include <sstream>

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

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// bind textures
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	reshape(1000, 1000);

	glGenTextures(1, &dispBindIndex);
	glBindTexture(GL_TEXTURE_2D, dispBindIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void display()
{
	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// set matrix
	glMatrixMode(GL_PROJECTION);
	//glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	//glOrtho(-2, 2, -2, 2, -2, 2);      // labtest 1
	//glOrtho(-2, 2, -2, 2, 0, 2);      // labtest 1
	//glOrtho(0, 2, -2, 2, -2, 2);      // labtest 1
	gluPerspective(20, 1, 1, 80);		// labtest 2
	//gluPerspective(60, 1, 1, 20);		// labtest 2

	//glFrustum(-1, 1, -1, 1, 1, 20);     // labtest 2-1
	//glFrustum(-0.1, 0.1, -0.1, 0.1, 1, 20);    // labtest 2-1
	//glTranslatef(t[0], t[1], t[2] - 1.0f);

	glTranslatef(t[0], t[1], t[2] - 3.0f);    // labtest 3
	//glScalef(3, 3, 3);			// labtest 3

	glScalef(1, 2, 3);			// labtest 4
	//glScalef(1, 1, 1);			   // labtest 4
	//glTranslatef(1,0,0);


	//glRotatef(35.26, 1.0, 0.0, 0.0);	// labtest 5
	//glRotatef(45.0, 0.0, 1.0, 0.0);   // labtest 5

	//gluLookAt(0, 0, 3, 0, 0, 0, 0, 1, 0);    // labtest 6
	//gluLookAt(0, 0, 3, 0, 0, 0, 0, -1, 0);     // labtest 6
	//gluLookAt(0, 0, 3, 0, 0, 100, 0, 1, 0);    // labtest 6

	GLfloat m[4][4], m1[4][4];
	build_rotmatrix(m, quat);

	m1[0][0] = 1.0f;
	m1[0][1] = 0.0f;
	m1[0][2] = -0.3f;
	m1[0][3] = 0.0f;

	m1[1][0] = 0.0f;;
	m1[1][1] = 1.0f;
	m1[1][2] = 0.3f;
	m1[1][3] = 0.0f;

	m1[2][0] = 0.0f;
	m1[2][1] = 0.0f;
	m1[2][2] = 1.0f;
	m1[2][3] = 0.0f;

	m1[3][0] = 0.0f;
	m1[3][1] = 0.0f;
	m1[3][2] = 0.0f;
	m1[3][3] = 1.0f;

	glMultMatrixf(&m[0][0]);
	//glMatrixMode(GL_MODELVIEW);
	//glMatrixMode(GL_PROJECTION);
	// Draw Model Points ///////////
	//GLfloat r, g, b;

	/*
	// Draw Input depth ///////////
	float dispgap = 0;
	glPointSize(3);

	glBegin(GL_POINTS);

	//glColor3f(0, 0, 1);
	//glVertex3f(0.0, 0.0, 0.0);

	float xx, yy, zz;
	float xx1, yy1, zz1;
	*/
	/*
	for (register int j = -250; j < 250; j=j+10)
	{
		for (register int i = -250; i < 250; i=i+10)
		{
			for (register int z = -250; z < 250; z = z + 10)
			{
				r = (i+250) / 250.0;
				g = (j+250) / 250.0;
				b = (z+250) / 250.0;
				glColor3f(r, g, b);
				//glColor3f(0.4, 0.4, 0.4);
				xx = j / 500.0;
				yy = i / 500.0;
				zz = z / 500.0;
				xx1 = m1[0][0] * xx + m1[0][1] * yy + m1[0][2] * zz;
				yy1 = m1[1][0] * xx + m1[1][1] * yy + m1[1][2] * zz;
				zz1 = m1[2][0] * xx + m1[2][1] * yy + m1[2][2] * zz;
				//glVertex3f(xx1,yy1,zz1);   // labtest 7 with ortho
											// labtest 8 see perspective again in pipeline viewpoint
				glVertex3f(xx, yy, zz);
			}
		}
	}

	glEnd();
	*/


	//set: gluPerspective(10, 1, 1, 200);		// labtest 9

	glPointSize(7);

	glBegin(GL_POINTS);

	for (register int j = 0; j < skeleton.size(); j++) {
		glColor3f(1.0, 0, 0);
		glVertex3f(skeleton[j].X, skeleton[j].Y, skeleton[j].Z);
	}
	for (register int j = 0; j < vertex.size(); j = j + 1)
	{
		glColor3f(0.5, 0.5, 1);
		//glVertex3f(realVertex[j].X, realVertex[j].Y, realVertex[j].Z);
		glVertex3f(vertex[j].X, vertex[j].Y, vertex[j].Z);
	}
	for (register int j = 0; j < realNormal.size(); j = j + 1)
	{
		glNormal3f(realNormal[j].X, realNormal[j].Y, realNormal[j].Z);
	}
	glEnd();



	/*
	glPointSize(7);
	glBegin(GL_TRIANGLES);

	glColor3f(0, 0, 1);
	glVertex3f(-1, -1, 1);
	glColor3f(0, 1, 0);
	glVertex3f(1, -1, 1);
	glColor3f(1, 0, 0);
	glVertex3f(-1, 1, 1);

	glColor3f(0, 0, 1);
	glVertex3f(1, 1, 1);
	glColor3f(0, 1, 0);
	glVertex3f(1, -1, 1);
	glColor3f(1, 0, 0);
	glVertex3f(-1, 1, 1);

	glEnd();
	*/
	glutSwapBuffers();

}

void parsing(const char* line) {
	float x, y, z;
	char lineHeader[128];
	sscanf(line, "%s %f %f %f", lineHeader, &x, &y, &z);
	if (strcmp(lineHeader, "v") == 0) {
		Vertex tempVertex;
		tempVertex.X = x / scale;
		tempVertex.Y = -y / scale;
		tempVertex.Z = z / scale;
		vertex.push_back(tempVertex);
	}
	else if (strcmp(lineHeader, "b") == 0) {
		Vertex tempVertex;
		tempVertex.X = x / scale;
		tempVertex.Y = -y / scale;
		tempVertex.Z = z / scale;
		skeleton.push_back(tempVertex);
	}
	else if (strcmp(lineHeader, "vt") == 0) {
		Vertex tempVertex;
		tempVertex.X = x / scale;
		tempVertex.Y = y / scale;
		texture.push_back(tempVertex);
	}
	else if (strcmp(lineHeader, "vn") == 0) {
		Vertex tempVertex;
		tempVertex.X = x / scale;
		tempVertex.Y = y / scale;
		tempVertex.Z = z / scale;
		normal.push_back(tempVertex);
	}
	else if (strcmp(lineHeader, "f") == 0) {
		std::string vertex1, vertex2, vertex3;
		unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
		int matches = sscanf(line, "%d %d %d %d\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2], &vertexIndex[3]);
		if (matches != 4) {
			printf("File can't be read by our simple parser : ( Try exporting with other options\n");
			return;
		}
		vertexIndices.push_back(vertexIndex[0]);
		vertexIndices.push_back(vertexIndex[1]);
		vertexIndices.push_back(vertexIndex[2]);
		vertexIndices.push_back(vertexIndex[3]);
		//uvIndices.push_back(uvIndex[0]);
		//uvIndices.push_back(uvIndex[1]);
		//uvIndices.push_back(uvIndex[2]);
		//normalIndices.push_back(normalIndex[0]);
		//normalIndices.push_back(normalIndex[1]);
		//normalIndices.push_back(normalIndex[2]);
		//normalIndices.push_back(normalIndex[3]);
	}
}

void calculateFace() {
	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		Vertex tempVertex = vertex[vertexIndex - 1];
		realVertex.push_back(tempVertex);

		//unsigned int normalIndex = normalIndices[i];
		//Vertex tempNormal = normal[normalIndex - 1];
		//realNormal.push_back(tempNormal);

		//unsigned int uvIndex = uvIndices[i];
		//Vertex tempUv = vertex[uvIndex - 1];
		//realVertex.push_back(tempUv);
	}
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
			lines = split(msg, '\n');
			for (int i = 0; i < lines.size(); i++) {
				parsing(lines[i].c_str());
			}
		}
		sock.close();
	}
	catch (string exception) {
		calculateFace();

		InitializeWindow(argc, argv);

		display();

		glutMainLoop();

		return 1;
	}


	vector<int> vertexIndices;
	vector<int> uvIndices;
	vector<int> normalIndices;

	/*
	FILE* file;
	file = fopen("junyong10.obj", "r");

	float x, y, z;

	while (1) {
		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader
		if (strcmp(lineHeader, "v") == 0) {
			fscanf(file, "%f %f %f\n", &x, &y, &z);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = -y / scale;
			tempVertex.Z = z / scale;
			vertex.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "b") == 0) {
			fscanf(file, "%f %f %f\n", &x, &y, &z);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = -y / scale;
			tempVertex.Z = z / scale;
			skeleton.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			fscanf(file, "%f %f\n", &x, &y);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = y / scale;
			texture.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			fscanf(file, "%f %f %f\n", &x, &y, &z);
			Vertex tempVertex;
			tempVertex.X = x / scale;
			tempVertex.Y = y / scale;
			tempVertex.Z = z / scale;
			normal.push_back(tempVertex);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
			//int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			//int matches = fscanf(file, "%d//%d %d//%d %d//%d %d//%d\n", &vertexIndex[0], &normalIndex[0], &vertexIndex[1], &normalIndex[1], &vertexIndex[2], &normalIndex[2], &vertexIndex[3], &normalIndex[3]);
			int matches = fscanf(file, "%d %d %d %d\n", &vertexIndex[0], &vertexIndex[1], &vertexIndex[2], &vertexIndex[3]);
			if (matches != 4) {
				printf("File can't be read by our simple parser : ( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			vertexIndices.push_back(vertexIndex[3]);
			//uvIndices.push_back(uvIndex[0]);
			//uvIndices.push_back(uvIndex[1]);
			//uvIndices.push_back(uvIndex[2]);
			//normalIndices.push_back(normalIndex[0]);
			//normalIndices.push_back(normalIndex[1]);
			//normalIndices.push_back(normalIndex[2]);
			//normalIndices.push_back(normalIndex[3]);
		}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); i++) {
		unsigned int vertexIndex = vertexIndices[i];
		Vertex tempVertex = vertex[vertexIndex - 1];
		realVertex.push_back(tempVertex);

		//unsigned int normalIndex = normalIndices[i];
		//Vertex tempNormal = normal[normalIndex - 1];
		//realNormal.push_back(tempNormal);

		//unsigned int uvIndex = uvIndices[i];
		//Vertex tempUv = vertex[uvIndex - 1];
		//realVertex.push_back(tempUv);
	}

	fclose(file);
			*/

	InitializeWindow(argc, argv);

	display();

	glutMainLoop();
	return 0;

}
