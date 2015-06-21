﻿#include "stdafx.h"
#include "drawline.h"
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <cmath>
#include <gl/glut.h>
#include <ctime>
#include <Windows.h>
#include <string>
#include <cstdio>
#include <fstream>
#define  MAX_NUM_OBJECT 10
#define  MAX_ASC_MODEL_VERTEX 1980	//Adjust following by teapot.asc
#define  MAX_ASC_MODEL_FACE   3760	//Adjust following by teapot.asc
#define  MAX_NUM_LIGHT 4
#define  MAX_BUFFER_SIZE 600
const float INF = -1;
using namespace std;

const int X_AXIS = 0,
Y_AXIS = 1,
Z_AXIS = 2;

const int RED = 0,
BLUE = 1,
WHITE = 2;

struct Light{
	int id;
	float ip,x,y,z;
	Light(){}
	Light(int id_, float ip_, float x_, float y_, float z_)
	{
		id = id_;
		ip = ip_;
		x = x_;
		y = y_;
		z = z_;
	}
};

struct ASCModel
{
	float r,g,b,kd,ks;
	int n;
	int   num_vertex;
	int   num_face;
	float vertex[MAX_ASC_MODEL_VERTEX][3];
	int   face[MAX_ASC_MODEL_FACE][5];
	ASCModel(){}
	ASCModel(float r_, float g_, float b_, float kd_, float ks_, int n_)
	{
		r = r_;
		g = g_;
		b = b_;
		kd = kd_;
		ks = ks_;
		n = n_;
	}
};

struct vec4
{
	float data[4];
	vec4(){ data[0] = data[1] = data[2] = data[3] = 0; };
	// vec4(float x, float y, float z, float w) TODO:need to fix the overload problem
	vec4(float x, float y, float z, float w = 1)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	}
	vec4(const vec4& a)
	{
		for (int i = 0; i<4; i++)
			data[i] = a.data[i];
		// cout << "point_data_copy_constructor" <<endl;
	}
	float length()
	{
		return sqrt(pow(data[0], 2) + pow(data[1], 2) + pow(data[2], 2));
	}
	vec4 operator+(const vec4& v)
	{
		vec4 result;
		result.data[0] = this->data[0] + v.data[0];
		result.data[1] = this->data[1] + v.data[1];
		result.data[2] = this->data[2] + v.data[2];
		return result;
	}
	vec4 operator-(const vec4& v)
	{
		vec4 result;
		result.data[0] = this->data[0] - v.data[0];
		result.data[1] = this->data[1] - v.data[1];
		result.data[2] = this->data[2] - v.data[2];
		return result;
	}
};

struct matrix
{
	float data[4][4];
	//Constructor
	matrix(){
		for (int i = 0; i<4; i++)
			for (int j = 0; j<4; j++)
				data[i][j] = 0;
	}
	//Copy constructor
	matrix(const matrix& a)
	{
		for (int i = 0; i<4; i++)
			for (int j = 0; j<4; j++)
				data[i][j] = a.data[i][j];
	}
	matrix(const vec4& v1, const vec4& v2, const vec4& v3, const vec4& v4)
	{
		for (int i = 0; i<4; i++)
		{
			data[0][i] = v1.data[i];
			data[1][i] = v2.data[i];
			data[2][i] = v3.data[i];
			data[3][i] = v4.data[i];
		}
	}
};

struct point_data
{
	float x, y, z;
	point_data* next;
	point_data(){ next = NULL; }
	point_data(float x_, float y_){ x = x_; y = y_; next = NULL; }
	point_data(const point_data& n){ x = n.x; y = n.y; next = NULL; /*cout << "point_data_copy_constructor" <<endl;*/ }
	point_data(vec4 a){ x = a.data[0]; y = a.data[1]; z = a.data[2]; next = NULL; }
};


//functions declaration
void displayFunc(void);
void ReadInput(bool& IsExit);
void scale(float sx, float sy, float sz);
void rotate(float degreeX, float degreeY, float degreeZ);
void translate(float tx, float ty, float tz);
void reset();
void clearData();
void clearScreen();
void observer(float px, float py, float pz, float cx, float cy, float cz, float tilt, float znear, float zfar, float hfov);
void viewport(float vl, float vr, float vb, float vt);
void display();

//Matrix operation
void print_matrix(const matrix& a);
void identity_matrix(matrix& a);
matrix matrix_mul(const matrix& a, const matrix& b);
matrix matrix_translation(float x, float y, float z);
matrix matrix_scaling(float sx, float sy, float sz);
matrix matrix_rotation(float degree, int X_Y_Z_AXIS);

//Vector opeartion
vec4 mat_vec_mul(const matrix& A, const vec4& x);
void print_vec4(const vec4& a);
float dot_product(const vec4& a, const vec4& b);
vec4 cross_product(const vec4& a, const vec4& b);
vec4 normalize(vec4 v);
vec4 norm_vec(vec4 a, vec4 b, vec4 c);

//variables decalration
int height, width;
int num_object = 0;
int num_light = 0;
float zbuffer[MAX_BUFFER_SIZE][MAX_BUFFER_SIZE];
float cbuffer[MAX_BUFFER_SIZE][MAX_BUFFER_SIZE][3];
bool first_endpoint = true;
float AR;
//Eye position
float ex,ey,ez;
point_data* cur_point;
matrix model_matrix;
matrix WVM, EM, PM, GRM, eyetilt;
ASCModel cube[MAX_NUM_OBJECT];
Light light[MAX_NUM_LIGHT];
vec4 view_vector;
//Ambient light coefficient
float ka;
//background color:R,G,B
float bgr,bgg,bgb; 
//Obeject
void create_object();

//Others
float deg2rad(float degree);
float ratio(float a, float b);
float delta(float a, float b);
void readModel(string filename, float r, float g, float b, float kd, float ks, int n);
ifstream fin;

void main(int ac, char** av)
{
	int winSizeX, winSizeY;
	string name;
	// initial();
	fin.open(av[1], std::ifstream::in);
	if (ac == 3) {
		winSizeX = atoi(av[1]);
		winSizeY = atoi(av[2]);
		//		cout<<"Done";
	}
	else if(ac == 2)
	{
		if(fin.fail())
		{
			cout << "Read in failed!" << endl;
			exit(0);
		}
		else
		{
			fin >> winSizeX >> winSizeY;
		}
	}
	else { // default window size
		winSizeX = 800;
		winSizeY = 600;
	}
	cout << av[1] << endl;

	width = winSizeX;
	height = winSizeY;
	cout << "Window size: " << width << " * " << height << endl;
	identity_matrix(model_matrix);
	viewport(0,width,0,height);

	// initialize OpenGL utility toolkit (glut)
	glutInit(&ac, av);

	// single disply and RGB color mapping
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB); // set display mode
	glutInitWindowSize(winSizeX, winSizeY);      // set window size
	glutInitWindowPosition(0, 0);                // set window position on screen
	glutCreateWindow("Lab3 Window");       // set window title
	// set up the mouse and keyboard callback functions
	//glutKeyboardFunc(myKeyboard); // register the keyboard action function

	// displayFunc is called whenever there is a need to redisplay the window,
	// e.g., when the window is exposed from under another window or when the window is de-iconified
	glutDisplayFunc(displayFunc); // register the redraw function

	// set background color
	glClearColor(0.0, 0.0, 0.0, 0.0);     // set the background to black
	glClear(GL_COLOR_BUFFER_BIT); // clear the buffer

	// misc setup
	glMatrixMode(GL_PROJECTION);  // setup coordinate system
	glLoadIdentity();
	gluOrtho2D(0, winSizeX, 0, winSizeY);
	glShadeModel(GL_FLAT);
	glFlush();
	glutMainLoop();
}


void scale(float sx, float sy, float sz)
{
	model_matrix = matrix_mul(matrix_scaling(sx, sy, sz), model_matrix);
	printf("Current matrix:\n");
	print_matrix(model_matrix);
}

void rotate(float degreeX, float degreeY, float degreeZ)
{
	model_matrix = matrix_mul(matrix_rotation(degreeX, X_AXIS), model_matrix);
	model_matrix = matrix_mul(matrix_rotation(degreeY, Y_AXIS), model_matrix);
	model_matrix = matrix_mul(matrix_rotation(degreeZ, Z_AXIS), model_matrix);
	// print_matrix(R);
	printf("Current matrix:\n");
	print_matrix(model_matrix);
}

void translate(float tx, float ty, float tz)
{
	model_matrix = matrix_mul(matrix_translation(tx, ty, tz), model_matrix);
	printf("Current matrix:\n");
	print_matrix(model_matrix);
}

//reset to identity matrix
void reset()
{
	identity_matrix(model_matrix);
	printf("Current matrix:\n");
	print_matrix(model_matrix);
}


void clearData()
{
	num_object = 0;
}

void clearScreen()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
}



// Display function
void displayFunc(void){

	bool IsExit;
	IsExit = false;
	// clear the entire window to the background color
	glClear(GL_COLOR_BUFFER_BIT);
	while (!IsExit)
	{
		glClearColor(0.0, 0.0, 0.0, 0.0);
		//	redraw();
		// draw the contents!!! Iterate your object's data structure!
		// flush the queue to actually paint the dots on the opengl window
		glFlush();
		ReadInput(IsExit);
	}
	fin.close();
	// exit(0);
}

//Read the command and operate it
void ReadInput(bool& IsExit)
{

	float sx, sy, sz, degreeX, degreeY, degreeZ,
		tx, ty, tz,
		// vl, vr, vb, vt,
		px, py, pz, cx, cy, cz, tilt, znear, zfar, hfov,
		r,g,b,kd,ks,
		ip, x, y, z;
	int id;
	int n;
	string command, comment, filename;
	fin >> command;
	cout << "== Command: " << command;
	if (command == "scale")
	{
		fin >> sx;
		fin >> sy;
		fin >> sz;
		cout << " : " << fixed << setprecision(2) << sx << " " << sy << " " << sz << endl;
		scale(sx, sy, sz);
	}
	else if (command == "object")
	{
		fin >> filename;
		cout << "[ " << filename << " ]" << endl;
		fin >> r >> g >> b >> kd >> ks >> n;
		readModel(filename,r,g,b,kd,ks,n);
		cout << "Read object sucessfully!" << endl;
		cout << "object color: " << r << " " << g << " " << " " << b << endl;
		create_object();
		cout << "Created ojbect!" << endl;
		num_object++;
	}
	// else if (command == "viewport")
	// {
	// 	fin >> vl >> vr >> vb >> vt;
	// 	cout << "( " << vl << "," << vr << "," << vb << "," << vt << " )" << endl;
	// 	viewport(vl, vr, vb, vt);
	// }
	else if (command == "observer")
	{
		fin >> px >> py >> pz >> cx >> cy >> cz >> tilt >> znear >> zfar >> hfov;
		cout << "Eye position: (";
		cout << px << " " << py << " " << pz << " )" << endl;
		cout << "Center of interest: ("
			<< cx << " " << cy << " " << cz << " )" << endl;
		// cout << "Tilt angle:" << tilt << endl
			// << "Near plane:" << znear << "\nFar Plane" << zfar << "\nHalf field of view(angle)" << hfov << endl;
		observer(px, py, pz, cx, cy, cz, tilt, znear, zfar, hfov);
	}
	else if (command == "display")
	{
		cout << endl << "draw sreen!" << endl;
		display();
		glFlush();
	}
	else if (command == "rotate")
	{
		fin >> degreeX >> degreeY >> degreeZ;
		cout << " : " << fixed << setprecision(2) << degreeX << " " << degreeY << " " << degreeZ << " " << endl;
		rotate(degreeX, degreeY, degreeZ);
	}
	else if (command == "translate")
	{
		fin >> tx;
		fin >> ty;
		fin >> tz;
		cout << " : " << fixed << setprecision(2) << tx << " " << ty << " " << tz << endl;
		translate(tx, ty, tz);
	}
	else if (command == "reset")
	{
		cout << endl;
		reset();
	}
	else if (command == "clearData")
	{
		cout << endl;
		clearData();
		cout << "Data is clear" << endl;
	}
	else if (command == "clearScreen")
	{
		cout << "Screen is cleared" << endl;
		cout << endl;
		clearScreen();
	}
	else if (command == "end")
	{
		IsExit = true;
		cout << endl;
		// exit(0);
	}
	else if (command == "#")
	{
		getline(fin, comment);
		cout << comment << endl;
		cout << endl;
	}
	else if(command == "ambient")
	{
		fin >> ka;
		cout << "Ambient ka:" << fixed << setprecision(2) << ka << endl << endl;
	}
	else if(command == "background")
	{
		fin >> bgr >> bgg >> bgb;
		cout << "Background color: " << bgr << " " << bgg << " " << bgb << endl;
	}
	else if(command == "light")
	{
		fin >> id >> ip >> x >> y >> z;
		light[num_light] = Light(id,ip,x,y,z);
		num_light++;
		cout << endl << "Light [" << id << "]: intensity:" << ip << endl 
		<<"position: (" << x << ", " << y << ", " << z << ", " << ")" << endl;   
	}
}

//Matrix A multiply matrix B
matrix matrix_mul(const matrix& a, const matrix& b)
{
	matrix c;
	for (int i = 0; i<4; i++)
		for (int j = 0; j<4; j++)
			for (int k = 0; k<4; k++)
				c.data[i][j] += a.data[i][k] * b.data[k][j];
	return c;
}

//Print the current matrix
void print_matrix(const matrix& a)
{
	for (int i = 0; i<4; i++)
	{
		printf("[ ");
		for (int j = 0; j < 4; j++)
			printf("%8.2f ", a.data[i][j]);
		printf(" ]\n");

	}
	printf("\n");
}

//Identify a matrix
void identity_matrix(matrix& a)
{
	a = matrix();
	for (int i = 0; i<4; i++)
		a.data[i][i] = 1;
}

//Covert degree to radius
float deg2rad(float degree)
{
	return degree / 180 * acos(-1);		//acos(-1) = PI
}

//Matrix and vector multiplication
vec4 mat_vec_mul(const matrix& A, const vec4& x)
{
	vec4 b;
	for (int i = 0; i<4; i++)
		for (int j = 0; j<4; j++)
			b.data[i] += A.data[i][j] * x.data[j];
	return b;
}

//Print the vector
void print_vec4(const vec4& a)
{
	printf("[ ");
	for (int i = 0; i<4; i++)
		printf("%lf ", a.data[i]);
	printf("]\n");
}

//Translate
matrix matrix_translation(float x, float y, float z)
{
	matrix T;
	identity_matrix(T);
	T.data[0][3] = x;
	T.data[1][3] = y;
	T.data[2][3] = z;
	return T;
}

//Scale 
matrix matrix_scaling(float sx, float sy, float sz)
{
	matrix S;
	identity_matrix(S);
	S.data[0][0] *= sx;
	S.data[1][1] *= sy;
	S.data[2][2] *= sz;
	return S;
}

//Rotate degree around the X_Y_Z_AXIS
matrix matrix_rotation(float degree, int X_Y_Z_AXIS)
{
	matrix rotation_matrix;
	identity_matrix(rotation_matrix);
	float sinv = sin(deg2rad(degree)),
		cosv = cos(deg2rad(degree));
	if (X_Y_Z_AXIS == X_AXIS)
	{
		rotation_matrix.data[1][1] = cosv;
		rotation_matrix.data[1][2] = -sinv;
		rotation_matrix.data[2][1] = sinv;
		rotation_matrix.data[2][2] = cosv;
	}
	else if (X_Y_Z_AXIS == Y_AXIS)
	{
		rotation_matrix.data[0][0] = cosv;
		rotation_matrix.data[0][2] = sinv;
		rotation_matrix.data[2][0] = -sinv;
		rotation_matrix.data[2][2] = cosv;
	}
	else if (X_Y_Z_AXIS == Z_AXIS)
	{
		rotation_matrix.data[0][0] = cosv;
		rotation_matrix.data[0][1] = -sinv;
		rotation_matrix.data[1][0] = sinv;
		rotation_matrix.data[1][1] = cosv;
	}
	return rotation_matrix;
}

float ratio(float a, float b)
{
	return a / b;
}

float delta(float a, float b)
{
	return abs(a - b);
}

//Read the model datas
void readModel(string filename, float r, float g, float b, float kd, float ks, int n)
{
	ifstream modelin(filename);
	cube[num_object] = ASCModel(r,g,b,kd,ks,n);
	// get number of vertex/face first
	modelin >> cube[num_object].num_vertex >> cube[num_object].num_face;

	// read vertex one by one
	for (int i = 0; i<cube[num_object].num_vertex; ++i) {
		modelin >> cube[num_object].vertex[i][0] >> cube[num_object].vertex[i][1] >> cube[num_object].vertex[i][2];
	}
	// read face one by one
	for (int i = 0; i<cube[num_object].num_face; ++i)
	{
		modelin >> cube[num_object].face[i][0];
		for (int j = 1; j <= cube[num_object].face[i][0]; j++)
			modelin >> cube[num_object].face[i][j];
	}
}

//Create the object based on the model matrix
void create_object()
{
	for (int i = 0; i<cube[num_object].num_vertex; ++i)
	{
		float& x = cube[num_object].vertex[i][0],
			&y = cube[num_object].vertex[i][1],
			&z = cube[num_object].vertex[i][2];
		vec4 v = mat_vec_mul(model_matrix, vec4(x, y, z));
		x = v.data[0];
		y = v.data[1];
		z = v.data[2];
	}
}

//Dot product of two vectors
float dot_product(const vec4& a, const vec4& b)
{
	return a.data[0] * b.data[0] +
		a.data[1] * b.data[1] +
		a.data[2] * b.data[2];
}

//Cross product of two vectors
vec4 cross_product(const vec4& a, const vec4& b)
{
	vec4 c;
	float xa = a.data[0],
		xb = b.data[0],
		ya = a.data[1],
		yb = b.data[1],
		za = a.data[2],
		zb = b.data[2];
	c.data[0] = ya * zb - yb * za;
	c.data[1] = -xa * zb + xb * za;
	c.data[2] = xa * yb - xb * ya;
	return c;
}


//Normalize the vector
vec4 normalize(vec4 v)
{
	float len = v.length();
	v.data[0] /= len;
	v.data[1] /= len;
	v.data[2] /= len;
	return v;
}

void observer(float px, float py, float pz, float cx, float cy, float cz, float tilt, float znear, float zfar, float hfov)
{
	//EM
	ex = px; ey = py; ez = pz;
	//Translation of eye poistion: move to origin point
	matrix Teye = matrix_translation(-px, -py, -pz);
	printf("Eye translation matrix:\n");
	print_matrix(Teye);

	//Eye tilt matrix
	eyetilt = matrix_rotation(-tilt, Z_AXIS);	//tilt here should be opposite
	printf("Eye tilt matrix:\n");
	print_matrix(eyetilt);

	//GRM
	vec4 view_vector(cx - px, cy - py, cz - pz, 0);
	vec4 v3 = normalize(view_vector);
	vec4 v1 = cross_product(vec4(0, 1, 0, 0), v3);
	v1 = normalize(v1);
	vec4 v2 = cross_product(v3, v1);
	v2 = normalize(v2);
	GRM = matrix(v1, v2, v3, vec4(0, 0, 0, 1));
	printf("GRM:\n");
	print_matrix(GRM);

	//Mirror Matrix
	matrix mirror;
	identity_matrix(mirror);
	mirror.data[0][0] = -1;
	EM = matrix_mul(eyetilt, matrix_mul(mirror, matrix_mul(GRM, Teye)));

	//PM
	identity_matrix(PM);
	PM.data[1][1] = AR;
	PM.data[2][2] = tan(deg2rad(hfov)) * zfar / (zfar - znear);
	PM.data[2][3] = zfar * znear / (znear - zfar) * tan(deg2rad(hfov));
	PM.data[3][2] = tan(deg2rad(hfov));
	PM.data[3][3] = 0;
}

void viewport(float vl, float vr, float vb, float vt)
{
	//AR
	AR = ratio(delta(vl, vr), delta(vb, vt));
	PM.data[1][1] = AR;						//Reassignment for AR value for preventing  
	print_matrix(PM);						//the misorder of command(observer and viewport)

	//WVM
	identity_matrix(WVM);
	matrix T1, S2, T3;
	T1 = matrix_translation(1, 1, 0);
	S2 = matrix_scaling(ratio(delta(vl, vr), delta(-1, 1)),
		ratio(delta(vb, vt), delta(-1, 1)),
		1);
	T3 = matrix_translation(vl, vb, 0);
	WVM = matrix_mul(T3, matrix_mul(S2, T1));
	// cout << "WVM" << endl;
	// print_matrix(WVM);
}

void display()
{
	//Final matrix
	matrix final;
	final = matrix_mul(WVM, matrix_mul(PM, EM));
	//initial z-buffer and color-buffer
	for(int i=0; i<MAX_BUFFER_SIZE; i++){
		for(int j=0; j<MAX_BUFFER_SIZE; j++){
			zbuffer[i][j] = INF;
			cbuffer[i][j][0] = bgr;
			cbuffer[i][j][1] = bgg;
			cbuffer[i][j][2] = bgb;
		}
	}

	
	for(int i=0; i<MAX_BUFFER_SIZE; i++)
		for(int j=0; j<MAX_BUFFER_SIZE; j++){
			drawDot(i,j,
				cbuffer[i][j][0],
				cbuffer[i][j][1],
				cbuffer[i][j][2]);
		}

	for (int i = 0; i<num_object; i++)	//For every object
	{
		ASCModel c = cube[i];
		int face_num = c.num_face;
		float r = cube[i].r,
			  g = cube[i].g,
			  b = cube[i].b,
			  kd = cube[i].kd,
			  ks = cube[i].ks;
		int n = cube[i].n;
		for (int j = 0; j<face_num; j++)	// For every face of the object
		{
			vec4 pt[4];						// World space points
			vec4 fpt[4];					// Final points
			int num_vec = c.face[j][0];		// number of vectors
/*			Calculate the first point
			int vec_num = c.face[j][1];		// the order number of the selected vector

			vec4 previous(c.vertex[vec_num - 1][0],
				c.vertex[vec_num - 1][1],
				c.vertex[vec_num - 1][2]);
			vec4 fp;
			previous = mat_vec_mul(final, previous);
			fp = previous;
*/
			for (int k = 1; k <= num_vec; k++)	// Calculating points of the face
			{
				int vec_num = c.face[j][k];
				pt[k-1] = vec4(c.vertex[vec_num - 1][0],
					c.vertex[vec_num - 1][1],
					c.vertex[vec_num - 1][2]);
				fpt[k-1] = mat_vec_mul(final, pt[k-1]);
/*				Connect the line of two point(current point and previous one)
				drawLine(previous.data[0] / previous.data[3],
					cur.data[0] / cur.data[3],
					previous.data[1] / previous.data[3],
					cur.data[1] / cur.data[3]);
				//If it's last one, connect back to the first point
				if (k == c.face[j][0])
				{
					previous = fp;
					drawLine(previous.data[0] / previous.data[3],
						cur.data[0] / cur.data[3],
						previous.data[1] / previous.data[3],
						cur.data[1] / cur.data[3]);
				}
				//Update previous
				previous = cur;
*/
			}
			vec4 norm = norm_vec(pt[0],pt[1],pt[2]);	//pay attention to the order of these points.
			float ox = pt[0].data[0],
				  oy = pt[0].data[1],
				  oz = pt[0].data[2];
			float ipsum1=0,ipsum2=0;
			for(int k=0; k<num_light; k++)
			{
				float lx = light[k].x,
					  ly = light[k].y,
					  lz = light[k].z,
					  ip = light[k].ip;
				vec4 l(lx-ox,ly-oy,lz-oz);	l = normalize(l);
				vec4 v(ex-ox,ey-oy,ez-oz);	v = normalize(v);
				float tx,ty,tz,tmp;
				tmp = 2 * dot_product(norm,l),
				tx = tmp * norm.data[0],
				ty = tmp * norm.data[1],
				tz = tmp * norm.data[2];
				vec4 r = vec4(tx,ty,tz) - l;
				ipsum1 += ip*dot_product(norm, l);
				ipsum2 += ip*pow(dot_product(r,v), n);
			}
			float Ir = ka * r + ipsum1 * kd * r + ipsum2 * ks,
				  Ig = ka * g + ipsum1 * kd * g + ipsum2 * ks,
				  Ib = ka * b + ipsum1 * kd * b + ipsum2 * ks;

		}
	}
}

vec4 norm_vec(vec4 a, vec4 b, vec4 c)
{
	vec4 v1 = vec4(b.data[0]-a.data[0],
				   b.data[1]-a.data[1],
				   b.data[2]-a.data[2]);
	vec4 v2 = vec4(c.data[0]-b.data[0],
				   c.data[1]-b.data[1],
				   c.data[2]-b.data[2]);
	vec4 result = cross_product(v1,v2);
	normalize(result);
	return result;
}