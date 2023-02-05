#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include "../OpenGL OGLDEV/headers/Camera.h"
#include "../OpenGL OGLDEV/headers/shaderhandling.h"
#include "../OpenGL OGLDEV/headers/ExtraMath.h"
#include "../OpenGL OGLDEV/headers/WorldTransform.h"
#include "../OpenGL OGLDEV/headers/KeyboardState.h"
#include "../OpenGL OGLDEV/headers/TimeStep.h"



#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

unsigned int VBO;
unsigned int IBO;
int gTransformLocation;
int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;
float FOV = 60.0f;
float zNear = 0.1f;
float zFar = 10.0f;
WorldTransform transform;
TimeStep timeStep;
Camera camera(WINDOW_WIDTH, WINDOW_HEIGHT, &timeStep);
PersProjData persProjData = { FOV, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, zNear, zFar };
KeyboardState keyboardState;
double lastTime = 0;


static void GLClearError() {
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line) {
	while (GLenum error = glGetError()) {
		std::cout << "[OpenGL Error] (" << error << ")" << " " << function << " : " << file << " : " << line << std::endl;
		return false;
	}
	return true;
}

struct Vertex {
	glm::fvec3 pos;
	glm::fvec3 color;

	Vertex() {}


	Vertex(float x, float y, float z) {
		pos = glm::fvec3(x, y, z);

		float red = (float)rand() / (float)RAND_MAX;
		float green = (float)rand() / (float)RAND_MAX;
		float blue = (float)rand() / (float)RAND_MAX;
		color = glm::fvec3(red, green, blue);

	}
};

static void CreateVertexBuffer() {

	Vertex Vertices[8];

	Vertices[0] = Vertex(0.5f, 0.5f, 0.5f);
	Vertices[1] = Vertex(-0.5f, 0.5f, -0.5f);
	Vertices[2] = Vertex(-0.5f, 0.5f, 0.5f);
	Vertices[3] = Vertex(0.5f, -0.5f, -0.5f);
	Vertices[4] = Vertex(-0.5f, -0.5f, -0.5f);
	Vertices[5] = Vertex(0.5f, 0.5f, -0.5f);
	Vertices[6] = Vertex(0.5f, -0.5f, 0.5f);
	Vertices[7] = Vertex(-0.5f, -0.5f, 0.5f);

	//create handle
	GLCall(glGenBuffers(1, &VBO));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
	//knows to look in ARRAY_BUFFER for data which is bound
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW));

}

static void CreateIndexBuffer() {
	unsigned int Indices[] = {
		0, 1, 2,
		1, 3, 4,
		5, 6, 3,
		7, 3, 6,
		2, 4, 7,
		0, 7, 6,
		0, 5, 1,
		1, 5, 3,
		5, 0, 6,
		7, 4, 3,
		2, 1, 4,
		0, 2, 7
	};

	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);
}

static void ReshapeCallback(int w, int h) {
	WINDOW_WIDTH = w;
	WINDOW_HEIGHT = h;
}

static void PassiveMouseCB(int x, int y)
{
	camera.OnMouse(glm::vec2(x, y));
}


static void RenderSceneCB() {
	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	static float angle = 0.0f;
	static float scale = 1.0f;
	timeStep.timeInterval = glutGet(GLUT_ELAPSED_TIME) - timeStep.lastTime;
	timeStep.lastTime = glutGet(GLUT_ELAPSED_TIME);
	std::cout << timeStep.timeInterval << std::endl;
#ifdef _WIN64
	angle += 1.0f;
#else
	angle += 1.0f;
#endif

	//transform.SetRotation(0.0f, 25.0f, 0.0f);
	transform.SetPosition(0.0f, 0.0f, -5.0f);
	//camera.SetPosition(3.0f, 0.0f, -8.0f);
	camera.HandleInput(&keyboardState);

	glm::fmat4x4 worldMatrix = transform.GetMatrix();
	glm::fmat4x4 cameraMatrix = camera.GetMatrix();
	glm::fmat4x4 projectionMatrix = ExtraMath::InitPersProjTransform(persProjData);


	glm::fmat4x4 WVP = worldMatrix * cameraMatrix * projectionMatrix;
	glm::fmat4x4 finalTransform = WVP;



	GLCall(glUniformMatrix4fv(gTransformLocation, 1, GL_TRUE, &finalTransform[0][0]));


	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	//explicity re-bind to ensure correct handle is used
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO));


	//index offset of pos
	GLCall(glEnableVertexAttribArray(0));
	//specify format of "enabled" attribute (pos)
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0));

	//color
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))));

	GLCall(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0));


	//now it's drawn, clear state
	GLCall(glDisableVertexAttribArray(0));
	GLCall(glDisableVertexAttribArray(1));


	//call render
	glutPostRedisplay();

	glutSwapBuffers();

}

static void CallKeyboardCB(unsigned char key, int x, int y) {
	keyboardState.KeyboardCB(key, x, y);
}
static void CallSpecialKeyboardCB(int key, int x, int y) {
	keyboardState.SpecialKeyboardCB(key, x, y);
}
static void CallSpecialKeysUp(int key, int x, int y) {
	keyboardState.SpecialKeysUp(key, x, y);
}
static void CallKeysUp(unsigned char key, int x, int y) {
	keyboardState.KeysUp(key, x, y);
}
static void InitializeGlutCallbacks() {
	glutDisplayFunc(RenderSceneCB);
	glutKeyboardFunc(CallKeyboardCB);
	glutSpecialFunc(CallSpecialKeyboardCB);
	glutSpecialUpFunc(CallSpecialKeysUp);
	glutKeyboardUpFunc(CallKeysUp);
	glutPassiveMotionFunc(PassiveMouseCB);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);

	glutInitWindowPosition(200, 100);

	int win = glutCreateWindow("UNREAL 8.0");

	char game_mode_string[64];
	snprintf(game_mode_string, sizeof(game_mode_string), "%dx%d@32", WINDOW_WIDTH, WINDOW_HEIGHT);
	glutGameModeString(game_mode_string);
	glutEnterGameMode();

	unsigned int res = glewInit();
	if (GLEW_OK != res)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(res));
		return 1;
	}

	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	printf("window id: %d\n", win);

	InitializeGlutCallbacks();

	ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(source.vertexSource, source.fragmentSource);
	GLCall(glUseProgram(shader));

	//stored as index
	GLCall(gTransformLocation = glGetUniformLocation(shader, "gTransform"));
	ASSERT(gTransformLocation != -1);

	//vertices must be ordered clockwise across the screen to be rendered
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	CreateIndexBuffer();
	CreateVertexBuffer();


	//allow resizing of window without breaking graphics
	glutReshapeFunc(ReshapeCallback);

	glutMainLoop();

	GLCall(glDeleteProgram(shader));

	return 0;
}
