#define _CRT_SECURE_NO_WARNINGS //--- ���α׷� �� �տ� ������ ��

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "opengl.h"
#include <cmath>
#include <random>
#include <string>

using namespace std;

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<double> XYdis(-1, 1);
uniform_real_distribution<float> xz_dis(-10.f, 10.f);
uniform_real_distribution<double> dis(0.0, 1.0);
std::uniform_real_distribution<> dist(0.0, 1.0);

struct Transform
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale = glm::vec3(1.0, 1.0, 1.0);

	glm::mat4 GetTransform()
	{
		glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 S = glm::scale(glm::mat4(1.0), scale);
		//glm::mat4 RX = glm::rotate(glm::mat4(1.0f), (float)glm::radians(rotation.x), glm::vec3(1.0, 0.0, 0.0));
		glm::mat4 RY = glm::rotate(glm::mat4(1.0f), (float)glm::radians(rotation.y), glm::vec3(0.0, 1.0, 0.0));
		//glm::mat4 RZ = glm::rotate(glm::mat4(1.0f), (float)glm::radians(rotation.z), glm::vec3(0.0, 0.0, 1.0));
		//return T * RY * S;
		return S * RY * T;
	}
};

int ammo = 30;
int maxAmmo = 30;
float bulletSpeed = 0.1f;
int score = 0;
void* font = GLUT_BITMAP_HELVETICA_18;

struct OBJECT {
	GLuint vao, vbo[3];
	Transform worldmatrix;
	Transform modelmatrix;
	OBJECT* parent{ nullptr };

	glm::vec3* vertex;
	glm::vec3* face;
	glm::vec3* vertexdata;
	glm::vec3* normaldata;
	glm::vec3* colordata;

	int v_count = 0;
	int f_count = 0;
	int vertex_count = f_count * 3;

	void ReadObj(string fileName)
	{
		ifstream in{ fileName };

		string s;

		while (in >> s)
		{
			if (s == "v") v_count++;
			else if (s == "f") ++f_count;
		}
		in.close();
		in.open(fileName);

		vertex_count = f_count * 3;

		vertex = new glm::vec3[v_count];
		face = new glm::vec3[f_count];
		vertexdata = new glm::vec3[vertex_count];
		normaldata = new glm::vec3[vertex_count];
		colordata = new glm::vec3[vertex_count];

		int v_incount = 0;
		int f_incount = 0;
		while (in >> s)
		{
			if (s == "v") {
				in >> vertex[v_incount].x >> vertex[v_incount].y >> vertex[v_incount].z;
				v_incount++;
			}
			else if (s == "f") {
				in >> face[f_incount].x >> face[f_incount].y >> face[f_incount].z;
				vertexdata[f_incount * 3 + 0] = vertex[static_cast<int>(face[f_incount].x - 1)];
				vertexdata[f_incount * 3 + 1] = vertex[static_cast<int>(face[f_incount].y - 1)];
				vertexdata[f_incount * 3 + 2] = vertex[static_cast<int>(face[f_incount].z - 1)];
				f_incount++;
			}
		}

		for (int i = 0; i < f_count; i++)
		{
			glm::vec3 normal = glm::cross(vertexdata[i * 3 + 1] - vertexdata[i * 3 + 0], vertexdata[i * 3 + 2] - vertexdata[i * 3 + 0]);
			//glm::vec3 normal = glm::vec3(0.0, 1.0, 0.0);
			normaldata[i * 3 + 0] = normal;
			normaldata[i * 3 + 1] = normal;
			normaldata[i * 3 + 2] = normal;
		}
	}

	glm::mat4 GetTransform()
	{
		if (parent)
			return parent->GetTransform() * worldmatrix.GetTransform();
		return worldmatrix.GetTransform();
	}

	glm::mat4 GetmodelTransform()
	{
		return modelmatrix.GetTransform();
	}

	void Move(glm::vec3 dir, float distance)
	{
		worldmatrix.position.x += distance * dir.x;
		worldmatrix.position.y += distance * dir.y;
		worldmatrix.position.z += distance * dir.z;
	}
};

struct CUBE :OBJECT
{
	void Init()
	{
		for (int i = 0; i < vertex_count; i++)
		{
			colordata[i].r = 0.3f;
			colordata[i].g = 0.3f;
			colordata[i].b = 0.3f;
		}
		for (int i = 0; i < vertex_count; i++)
		{
			vertexdata[i] -= glm::vec3(0.5, 0.5, 0.5);
		}

		glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
		glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
		glGenBuffers(3, vbo); //--- 3���� VBO�� �����ϰ� �Ҵ��ϱ�

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
	}

	void draw(int shaderID)
	{
		unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertex_count);
	}

	void resize()
	{

	}

	void update()
	{
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}
};
CUBE cube;
CUBE minicube;

struct MONSTER : CUBE {
	int hp;

	float x, y, z;
	float dx, dy, dz;
	bool active;

	glm::vec3 RandomColor() {
		return glm::vec3(dist(gen), dist(gen), dist(gen));
	}

	void virtual Init()
	{
		glm::vec3 color = RandomColor();

		for (int i = 0; i < vertex_count; i++)
		{
			if (i % 6 == 0) {
				color = RandomColor();
			}
			colordata[i] = color;
		}


		for (int i = 0; i < vertex_count; i++)
		{
			vertexdata[i] -= glm::vec3(0.5, 0.5, 0.5);
		}

		glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
		glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
		glGenBuffers(3, vbo); //--- 3���� VBO�� �����ϰ� �Ҵ��ϱ�

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
	}

	void initial_status() {
		hp = 10;

		x = xz_dis(gen);
		z = xz_dis(gen);
		worldmatrix.position = glm::vec3(x, -30.0f, z); // ���� �ʱ� ��ġ
		y = 0;

		active = true;
	}

	void hit() {
		hp--;
		if (hp <= 0) {
			active = false;
		}
	}

	bool is_die() {
		return !active;
	}

	void move(glm::vec3 m) {
		x += m.x;
		y += m.y;
		z += m.z;
	}
};

constexpr int MONSTER_COUNT = 10;
MONSTER monster[MONSTER_COUNT];

struct PLAYER : OBJECT {
	int hp;

	float x, y, z;
	bool b_w, b_a, b_s, b_d;
	float speed;

	GLuint ebo;
	int n_count = 0;
	glm::vec3* normal;

	void virtual ReadObj(string fileName) {
		ifstream in{ fileName };

		string s;

		while (in >> s)
		{
			if (s == "v") v_count++;
			else if (s == "vn") n_count++;
			else if (s == "f") f_count++;
		}
		in.close();
		in.open(fileName);

		vertex_count = f_count * 3;

		vertex = new glm::vec3[v_count];
		normal = new glm::vec3[n_count];
		vertexdata = new glm::vec3[vertex_count];
		normaldata = new glm::vec3[vertex_count];
		colordata = new glm::vec3[vertex_count];

		int v_incount = 0;
		int f_incount = 0;
		int n_incount = 0;
		int nv_incount = 0;

		int tmp;
		char slash;

		while (in >> s)
		{
			if (s == "v") {
				in >> vertex[v_incount].x >> vertex[v_incount].y >> vertex[v_incount].z;
				v_incount++;
			}
			else if (s == "vn") {
				in >> normal[nv_incount].x >> normal[nv_incount].y >> normal[nv_incount].z;
				nv_incount++;
			}
			else if (s == "f") {
				glm::vec3 vert;
				glm::vec3 nor;

				in >> vert.x >> slash >> tmp >> slash >> nor.x;
				in >> vert.y >> slash >> tmp >> slash >> nor.y;
				in >> vert.z >> slash >> tmp >> slash >> nor.z;

				vertexdata[f_incount * 3 + 0] = vertex[static_cast<int>(vert.x - 1)];
				vertexdata[f_incount * 3 + 1] = vertex[static_cast<int>(vert.y - 1)];
				vertexdata[f_incount * 3 + 2] = vertex[static_cast<int>(vert.z - 1)];
				f_incount++;

				normaldata[n_incount * 3 + 0] = normal[static_cast<int>(nor.x - 1)];
				normaldata[n_incount * 3 + 1] = normal[static_cast<int>(nor.y - 1)];
				normaldata[n_incount * 3 + 2] = normal[static_cast<int>(nor.z - 1)];
				n_incount++;
			}
		}
		return;
	}

	void Init()
	{
		//x = -0.5f;
		//y = 0.0f;
		//z = -0.5f;

		speed = 0.01f;
		b_w = false;
		b_a = false;
		b_s = false;
		b_d = false;

		hp = 10;
		for (int i = 0; i < vertex_count; i++)
		{
			// double random_color = dis(gen);

			colordata[i].x = 0.2f;
			colordata[i].y = 0.2f;
			colordata[i].z = 0.6f;
		}
		for (int i = 0; i < vertex_count; i++)
		{
			vertexdata[i] -= glm::vec3(0.5, 0.5, 0.5);
		}

		glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
		glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
		glGenBuffers(3, vbo); //--- 3���� VBO�� �����ϰ� �Ҵ��ϱ�

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
	}

	void draw(int shaderID)
	{
		unsigned int modelLocation = glGetUniformLocation(shaderID, "model");
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(GetTransform() * GetmodelTransform()));
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertex_count);
	}

	void resize()
	{

	}

	void update()
	{
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);
	}

	void get_damage() {
		hp--;
	}

	void transform(glm::vec3 m) {
		x += m.x;
		y += m.y;
		z += m.z;

		Move(m, 1);
	}
};

PLAYER player;

struct Bullet : CUBE {
	float x, y, z;
	float dx, dy, dz;
	bool active;

	void intial_status(float px, float py, float pz) {
		active = true;
		x = px;
		y = py;
		z = pz;
		worldmatrix.position = glm::vec3(px, -30.f + py, pz);
		worldmatrix.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	void virtual Init()
	{
		double rcr = dis(gen);
		double rcg = dis(gen);
		double rcb = dis(gen);

		for (int i = 0; i < vertex_count; i++)
		{
			colordata[i].x = rcr;
			colordata[i].y = rcg;
			colordata[i].z = rcb;
		}
		for (int i = 0; i < vertex_count; i++)
		{
			vertexdata[i] -= glm::vec3(0.5, 0.5, 0.5);
		}

		glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
		glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
		glGenBuffers(3, vbo); //--- 3���� VBO�� �����ϰ� �Ҵ��ϱ�

		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), vertexdata, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), colordata, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(glm::vec3), normaldata, GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);
	}
};

unsigned int bullet_count;

constexpr int HIT_DISTANCE = 1;
constexpr int MAX_BULLET = 30;
Bullet bullets[MAX_BULLET] = {};

GLfloat lineShape[10][2][3] = {};	//--- ���� ��ġ ��

glm::vec3 colors[12][3] = {};

GLfloat XYZShape[3][2][3] = {
	{{-1.0,0.0,0.0},{1.0,0.0,0.0}},
	{{0.0,-1.0,0.0},{0.0,1.0,0.0}},
	{{0.0,0.0,-1.0},{0.0,0.0,1.0}} };

GLfloat XYZcolors[6][3] = { //--- �� ����
	{ 1.0, 0.0, 0.0 },	   	{ 1.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0 },	   	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 },	   	{ 0.0, 0.0, 1.0 }
};

// cameraPos�� �÷��̾ �����̴� ��ŭ �������ֱ�
glm::vec3 cameraPos = glm::vec3(0.0f, 30.0f, 0.0f); //--- ī�޶� ��ġ
glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 0.0f); //--- ī�޶� �ٶ󺸴� ����
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); //--- ī�޶� ���� ����

glm::mat4 model = glm::mat4(1.0f);
glm::mat4 projection = glm::mat4(1.0f);

glm::mat4 view = glm::lookAt(
	cameraPos,                // ī�޶� ��ġ
	cameraDirection,             // ī�޶� �ٶ󺸴� ����
	cameraUp                  // ī�޶��� ���� ����
);

// ���� �� �κ� �ڵ�
void fireBullet();
void updateBullets();
float Cal_Distance(glm::vec3 p1, glm::vec3 p2);
void checkCollisions();
void renderText(float x, float y, const char* text);
void reload();
//

GLuint vao, vbo[3];
GLuint TriPosVbo, TriColorVbo;

GLchar* vertexSource, * fragmentSource; //--- �ҽ��ڵ� ���� ����
GLuint vertexShader, fragmentShader; //--- ���̴� ��ü
GLuint shaderProgramID; //--- ���̴� ���α׷�

int windowWidth = 800;
int windowHeight = 800;

float openGLX, openGLY;
int movingRectangle = -1;

float ox = 0, oy = 0;
float x_angle = 0;
float y_angle = 0;
float z_angle = 0;
float pre_x_angle = 0;
float pre_y_angle = 0;
float wheel_scale = 0.15;
bool right_button = 0;
float fovy = 45;
float near_1 = 0.1;
float far_1 = 200.0;
float persfect_z = -2.0;

bool start = true;

bool NSelection = true;	//true : cube, false : pyramid
bool YSelection = false;
int RSelection = 0;
int RSelectionCnt = 0;
int CCnt = 0;
float CX, CY, CZ;

void make_shaderProgram();
void make_vertexShaders();
void make_fragmentShaders();

GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
void KeyboardUpFunction(unsigned char key, int x, int y);
void InitBuffer();
char* filetobuf(const char*);
GLvoid Mouse(int button, int state, int x, int y);
GLvoid WindowToOpenGL(int mouseX, int mouseY, float& x, float& y);
GLvoid Motion(int x, int y);
GLvoid TimerFunction(int value);
GLvoid SpecialKeys(int key, int x, int y);
GLvoid mouseWheel(int button, int dir, int x, int y);

void main(int argc, char** argv) //--- ������ ����ϰ� �ݹ��Լ� ����
{
	//--- ������ �����ϱ�
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Example1");

	//--- GLEW �ʱ�ȭ�ϱ�
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Unable to initialize GLEW" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		std::cout << "GLEW Initialized\n";
	}
	cube.ReadObj("map.obj");
	minicube.ReadObj("map.obj");
	//player.ReadObj("cube.obj");
	for (int i = 0; i < MONSTER_COUNT; ++i)
	{
		monster[i].ReadObj("cube.obj");
	}
	player.ReadObj("gun1.obj");

	for (int i = 0; i < MAX_BULLET; ++i) {
		bullets[i].ReadObj("bullet.obj");
	}

	std::cout << "Objects initialized.\n";
	CX = 1.0, CY = 1.0, CZ = 1.0;

	//--- ���̴� �о�ͼ� ���̴� ���α׷� �����
	make_shaderProgram(); //--- ���̴� ���α׷� �����
	InitBuffer();
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE); //--- ���� ������ �ʿ��� ������ �ϸ� �ȴ�.
	//glDisable(GL_DEPTH_TEST | GL_CULL_FACE);	//����

	glutTimerFunc(10, TimerFunction, 0);
	glutTimerFunc(10, TimerFunction, 1);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUpFunction);
	glutSpecialFunc(SpecialKeys); // ����Ű �ݹ� �Լ� ���
	glutMouseFunc(Mouse);
	glutMotionFunc(Motion);
	glutMouseWheelFunc(mouseWheel);

	glutMainLoop();
}

GLvoid drawScene()
{
	glUseProgram(shaderProgramID);
	glClearColor(0.0, 0.0, 0.0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //--- ���� ���۸� Ŭ�����Ѵ�.

	glBindVertexArray(vao);

	// ���� �ٲٱ�
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), XYZcolors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	unsigned int cameraLocation = glGetUniformLocation(shaderProgramID, "camera_position");
	glUniform3f(cameraLocation, cameraPos.x, cameraPos.y, cameraPos.z);

	projection = glm::mat4(1.0f);
	projection = glm::scale(projection, glm::vec3(wheel_scale, wheel_scale, wheel_scale));
	projection = glm::rotate(projection, (float)glm::radians(x_angle), glm::vec3(1.0, 0.0, 0.0));
	projection = glm::rotate(projection, (float)glm::radians(y_angle), glm::vec3(0.0, 1.0, 0.0));

	int viewLocation = glGetUniformLocation(shaderProgramID, "view");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(projection));

	glm::mat4 perspect = glm::mat4(1.0f);
	perspect = glm::perspective(glm::radians(fovy), (float)windowWidth / (float)windowHeight, near_1, far_1);
	perspect = glm::translate(perspect, glm::vec3(0.0, 0.0, persfect_z));
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projection");
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(perspect));

	glm::mat4 lightmatrix = cube.GetTransform();
	glm::vec3 lightposition = glm::vec3(0, 10, 0);

	unsigned int lightPosLocation = glGetUniformLocation(shaderProgramID, "lightPos");
	glUniform3f(lightPosLocation, lightposition.x, lightposition.y, lightposition.z);
	unsigned int lightColorLocation = glGetUniformLocation(shaderProgramID, "lightColor");
	glUniform3f(lightColorLocation, CX, CY, CZ);
	unsigned int objColorLocation = glGetUniformLocation(shaderProgramID, "objectColor");
	glUniform3f(objColorLocation, 1.0, 0.5, 0.3);

	// monster�� Ȱ�� ������ ���� �׸���
	for (int i = 0; i < MONSTER_COUNT; ++i) {
		if (monster[i].active)
		{
			monster[i].draw(shaderProgramID);
		}
	}

	// ��
	cube.draw(shaderProgramID);

	for (int i = 0; i < MAX_BULLET; ++i) {
		if (bullets[i].active) {
			bullets[i].draw(shaderProgramID);
		}
	}
	player.draw(shaderProgramID);

	glDisable(GL_DEPTH_TEST);
	glColor3f(1.0f, 1.0f, 1.0f);
	char hud[128];
	sprintf_s(hud, "Ammo: %d / %d", ammo, maxAmmo);
	renderText(-0.9f, 0.9f, hud);
	sprintf_s(hud, "Score: %d", score);
	renderText(-0.9f, 0.8f, hud);
	glEnable(GL_DEPTH_TEST);

	glutSwapBuffers();
}
//--- �ٽñ׸��� �ݹ� �Լ�
GLvoid Reshape(int w, int h)
{
	glViewport(0, 0, w, h);
}

void InitBuffer()
{
	glGenVertexArrays(1, &vao); //--- VAO �� �����ϰ� �Ҵ��ϱ�
	glBindVertexArray(vao); //--- VAO�� ���ε��ϱ�
	glGenBuffers(2, vbo); //--- 2���� VBO�� �����ϰ� �Ҵ��ϱ�

	// �ʱ�ȭ
	cube.Init();
	for (int i = 0; i < MONSTER_COUNT; ++i) {
		monster[i].Init();
		monster[i].active = true;
	}
	player.Init();

	for (int i = 0; i < MAX_BULLET; ++i) {
		bullets[i].Init();
		bullets[i].active = false;
		bullets[i].worldmatrix.position = glm::vec3(0.0f, -30.f, 0.0f);
		//bullets[i].modelmatrix.scale = glm::vec3(0.5f, 0.5f, 0.5f);
	}

	// �÷��̾�� ���� �ʱ� ��ġ ����
	player.worldmatrix.position = glm::vec3(0.0f, -30.f, 0.0f);

	for (int i = 0; i < MONSTER_COUNT; ++i) {
		monster[i].initial_status();
	}
	for (int i = 0; i < MONSTER_COUNT; ++i) {
		monster[i].parent = &cube; // ������ �θ� ���� (�ʿ� ��)
	}
}

void make_shaderProgram()
{
	make_vertexShaders(); //--- ���ؽ� ���̴� �����
	make_fragmentShaders(); //--- �����׸�Ʈ ���̴� �����
	//-- shader Program
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);
	//--- ���̴� �����ϱ�
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	//--- Shader Program ����ϱ�
	glUseProgram(shaderProgramID);
}

void make_vertexShaders()
{
	vertexSource = filetobuf("vertex3.glsl");
	//--- ���ؽ� ���̴� ��ü �����
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//--- ���̴� �ڵ带 ���̴� ��ü�� �ֱ�
	glShaderSource(vertexShader, 1, (const GLchar**)&vertexSource, 0);
	//--- ���ؽ� ���̴� �������ϱ�
	glCompileShader(vertexShader);
	//--- �������� ����� ���� ���� ���: ���� üũ
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cout << "ERROR: vertex shader ������ ����\n" << errorLog << std::endl;
		return;
	}
}

void make_fragmentShaders()
{
	fragmentSource = filetobuf("fragment3.glsl");
	//--- �����׸�Ʈ ���̴� ��ü �����
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//--- ���̴� �ڵ带 ���̴� ��ü�� �ֱ�
	glShaderSource(fragmentShader, 1, (const GLchar**)&fragmentSource, 0);
	//--- �����׸�Ʈ ���̴� ������
	glCompileShader(fragmentShader);
	//--- �������� ����� ���� ���� ���: ������ ���� üũ
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cout << "ERROR: fragment shader ������ ����\n" << errorLog << std::endl;
		return;
	}
}

char* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb"); // Open file for reading 
	if (!fptr) // Return NULL on failure 
		return NULL;
	fseek(fptr, 0, SEEK_END); // Seek to the end of the file 
	length = ftell(fptr); // Find out how many bytes into the file we are 
	buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator 
	fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file 
	fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer 
	fclose(fptr); // Close the file 
	buf[length] = 0; // Null terminator 
	return buf; // Return the buffer 
}

bool CheckCollision(const PLAYER& player, const MONSTER& monster)
{
	float distance = glm::length(player.worldmatrix.position - monster.worldmatrix.position);
	float collisionDistance = 0.5f; // �浹 �Ÿ� ���� (�÷��̾�� ������ �߽� �Ÿ�)

	return distance < collisionDistance; // �� ��ü�� �浹������ true ��ȯ
}

void MoveMonsterTowardsPlayer(float speed)
{
	for (int i = 0; i < MONSTER_COUNT; ++i) {

		if (!monster[i].active) continue; // ���Ͱ� ��Ȱ��ȭ�Ǹ� �������� ����

		glm::vec3 direction = player.worldmatrix.position - monster[i].worldmatrix.position;

		if (glm::length(direction) > 0.01f) // �ʹ� ������ ������ ����
		{
			direction = glm::normalize(direction);

			monster[i].worldmatrix.position += direction * speed;
			monster[i].move(direction * speed);
		}

		// �浹 ����
		if (CheckCollision(player, monster[i]))
		{
			monster[i].active = false; // �浹�ϸ� monster ��Ȱ��ȭ
			player.get_damage(); // player�� ������ �ִ� �Լ�
			glutTimerFunc(1000, TimerFunction, 6); // monster ��Ȱ Ÿ�̸� ����
			cout << "Player HP : " << player.hp << '\n';
		}
	}
}

void KeyboardUpFunction(unsigned char key, int x, int y) { // Ű���带 �������� ȣ���ϴ� �Լ� 
	if (key == 'w') {
		player.b_w = false;
	}
	else if (key == 'a') {
		player.b_a = false;
	}
	else if (key == 's') {
		player.b_s = false;
	}
	else if (key == 'd') {
		player.b_d = false;
	}

	glutPostRedisplay();
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	// w a s d�� �Է������� �÷��̾� ��ġ�� ����ǵ��� �ϱ� + ī�޶� ��ġ��
	if (key == 'r' || key == 'R') {
		reload();
	}
	else if (key == 'w') {
		player.b_w = true;
		glutTimerFunc(10, TimerFunction, 2);
		// TODO : ī�޶� ��ġ�� ���� ���� �������
	}
	else if (key == 'a') {
		player.b_a = true;
		glutTimerFunc(10, TimerFunction, 3);
	}
	else if (key == 's') {
		player.b_s = true;
		glutTimerFunc(10, TimerFunction, 4);
	}
	else if (key == 'd') {
		player.b_d = true;
		glutTimerFunc(10, TimerFunction, 5);
	}

	glutPostRedisplay(); //--- ������ �ٲ� ������ ��� �ݹ� �Լ��� ȣ���Ͽ� ȭ���� refresh �Ѵ�
}

GLvoid SpecialKeys(int key, int x, int y)
{
	glutPostRedisplay(); // ȭ�� ����
}

int movingMouse = -1;
float beforeX, beforeY;

GLvoid Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		ox = x;
		oy = y;
		right_button = true;
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		fireBullet();
	}
	else {
		ox = 0;
		oy = 0;
		pre_x_angle = x_angle;
		pre_y_angle = y_angle;

		player.worldmatrix.rotation.y = -y_angle;

		right_button = false;
	}
}

GLvoid Motion(int x, int y)
{
	if (right_button) {
		y_angle = x - ox;
		x_angle = y - oy;
		x_angle += pre_x_angle;
		y_angle += pre_y_angle;

		y_angle /= 2;
		x_angle /= 2;

		player.worldmatrix.rotation.y = -y_angle;
	}
	glutPostRedisplay();
}

GLvoid mouseWheel(int button, int dir, int x, int y)
{
	if (dir > 0)
	{
		wheel_scale += dir * 0.1;
	}
	else if (dir < 0)
	{
		wheel_scale += dir * 0.1;
		if (wheel_scale < 0.1)
		{
			wheel_scale = 0.1;
		}
	}
	glutPostRedisplay();
}

GLvoid WindowToOpenGL(int mouseX, int mouseY, float& x, float& y)
{
	x = (2.0f * mouseX) / windowWidth - 1.0f;
	y = 1.0f - (2.0f * mouseY) / windowHeight;
}

GLvoid TimerFunction(int value)
{
	switch (value)
	{
	case 0: // update ����
		updateBullets();
		checkCollisions();
		glutTimerFunc(10, TimerFunction, 0);
		break;
	case 1: // monster �����̴� ����
		// ���Ͱ� �÷��̾ ���� �̵�
		MoveMonsterTowardsPlayer(0.005f); // �ӵ� 0.05
		glutTimerFunc(10, TimerFunction, 1);
		break;

	case 2: // wŰ �������� �����̴� ����
		// 0, 0, -speed
		player.transform(glm::vec3(-player.speed * sin(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * cos(player.worldmatrix.rotation.y / 180 * 3.14)));
		// 0, 0, +speed
		cameraPos += glm::vec3(-player.speed * -sin(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * -cos(player.worldmatrix.rotation.y / 180 * 3.14));
		if (player.b_w)
			glutTimerFunc(10, TimerFunction, 2);
		break;

	case 3: // aŰ �������� �����̴� ����
		// -speed, 0, 0
		player.transform(glm::vec3(-player.speed * cos(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * -sin(player.worldmatrix.rotation.y / 180 * 3.14)));
		// +speed, 0, 0
		cameraPos += glm::vec3(-player.speed * -cos(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * sin(player.worldmatrix.rotation.y / 180 * 3.14));
		if (player.b_a)
			glutTimerFunc(10, TimerFunction, 3);
		break;

	case 4: // sŰ �������� �����̴� ����
		// 0, 0, +speed
		player.transform(glm::vec3(-player.speed * -sin(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * -cos(player.worldmatrix.rotation.y / 180 * 3.14)));
		// 0, 0, -speed
		cameraPos += glm::vec3(-player.speed * sin(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * cos(player.worldmatrix.rotation.y / 180 * 3.14));
		if (player.b_s)
			glutTimerFunc(10, TimerFunction, 4);
		break;

	case 5:	// dŰ �������� �����̴� ����
		// +speed, 0, 0
		player.transform(glm::vec3(-player.speed * -cos(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * sin(player.worldmatrix.rotation.y / 180 * 3.14)));
		// -speed, 0, 0
		cameraPos += glm::vec3(-player.speed * cos(player.worldmatrix.rotation.y / 180 * 3.14), 0, -player.speed * -sin(player.worldmatrix.rotation.y / 180 * 3.14));
		if (player.b_d)
			glutTimerFunc(10, TimerFunction, 5);
		break;
	case 6: // monster ������ ����
		for (int i = 0; i < MONSTER_COUNT; ++i) {
			if (monster[i].is_die()) {
				monster[i].initial_status();
			}
		}
		break;
	}
	glutPostRedisplay();
}

// ��
void fireBullet() {
	if (ammo > 0) {
		for (int i = 0; i < MAX_BULLET; ++i) {
			if (bullets[i].active == false) {
				bullets[i].active = true;
				bullets[i].intial_status(player.x, player.y, player.z);
				bullets[i].dx = -sin(player.worldmatrix.rotation.y / 180 * 3.14);
				bullets[i].dy = 0.0f;
				bullets[i].dz = -cos(player.worldmatrix.rotation.y / 180 * 3.14);
				ammo--;
				break;
			}
		}
	}
	else {
		std::cout << "Out of ammo! Press 'R' to reload." << std::endl;
	}
}

void reload() {
	if (ammo < maxAmmo) {
		ammo = maxAmmo;
		std::cout << "Reloaded! Ammo: " << ammo << std::endl;
	}
}

void updateBullets() {
	for (auto& bullet : bullets) {
		if (bullet.active) {

			bullet.x += bullet.dx * bulletSpeed;
			bullet.y += bullet.dy * bulletSpeed;
			bullet.z += bullet.dz * bulletSpeed;

			bullet.Move(glm::vec3(bullet.dx, bullet.dy, bullet.dz), bulletSpeed);

			if (std::abs(bullet.z) > 20.0f) {
				bullet.active = false;
				std::cout << "bullet out bound" << std::endl;
			}
		}
	}
}

float Cal_Distance(glm::vec3 p1, glm::vec3 p2) {
	return std::sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2));
}

void checkCollisions() {
	for (auto& bullet : bullets) {
		if (!bullet.active) continue;

		for (auto& mons : monster) {
			if (mons.is_die()) continue;

			if (Cal_Distance(bullet.worldmatrix.position, mons.worldmatrix.position) <= HIT_DISTANCE) {
				bullet.active = false;
				mons.hit();

				if (mons.is_die()) {
					glutTimerFunc(1000, TimerFunction, 6);
					score++;
					std::cout << "Hit! Score: " << score << std::endl;
				}
			}
		}
	}
}

void renderText(float x, float y, const char* text) {
	glRasterPos2f(x, y);
	for (const char* c = text; *c != '\0'; c++) {
		glutBitmapCharacter(font, *c);
	}
}



//update() : �ƿ� �����͸� �ٲٰ� ������ ����.

// player
