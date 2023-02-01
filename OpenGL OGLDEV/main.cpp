#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include "../Include/shaderhandling.h"


#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

const double pi = atan(1) * 4;
unsigned int VBO;
unsigned int IBO;
int gTransformLocation;


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



static void RenderSceneCB() {
	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	static float angleInRadians = 0.0f;
	static float scale = 1.0f;

#ifdef _WIN64
	angleInRadians += 0.01;
#else
	angleInRadians += 0.01f;
#endif

	//PERPRO * (TRA * (ROT * (SCA * POS)))
	glm::fmat4x4 translationMatrix (
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 3.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	glm::fmat4x4 scaleMatrix(
		scale, 0.0f,  0.0f,  0.0f,
		0.0f,  scale, 0.0f,  0.0f,
		0.0f,  0.0f,  scale, 0.0f,
		0.0f,  0.0f,  0.0f,  1.0f
	);

	float FOV = 90.0f;
	float tanHalfFOV = tanf((FOV / 2.0f) * (pi / 180.0f));
	float f = 1 / tanHalfFOV;

	float ar = 1.0;

	float nearZ = 1.0f;
	float farZ = 10.0f;

	float zRange = nearZ - farZ;

	float A = (-farZ - nearZ) / zRange;
	float B = 2.0f * farZ * nearZ / zRange;


	glm::fmat4x4 projectionMatrix(
		f, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	);


	glm::fmat4x4 rotationMatrix(
		cosf(angleInRadians),  0.0f, -sinf(angleInRadians), 0.0f,
		0.0f,				   1.0f, 0.0f,				    0.0f,
		sinf(angleInRadians),  0.0f, cosf(angleInRadians),  0.0f,
		0.0f,				   0.0f, 0.0f,				    1.0f
	);

	glm::fmat4x4 finalTransform = (rotationMatrix * translationMatrix) * projectionMatrix;


	GLCall(glUniformMatrix4fv(gTransformLocation, 1, GL_TRUE, &finalTransform[0][0]));


	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	//explicity re-bind to ensure correct handle is used
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO));


	//index offset of pos
	GLCall(glEnableVertexAttribArray(0));
	//specify format of "enabled" attribute (pos)
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), 0));

	//color
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float))));

	GLCall(glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0));


	//now it's drawn, clear state
	GLCall(glDisableVertexAttribArray(0));
	GLCall(glDisableVertexAttribArray(1));


	//call render
	glutPostRedisplay();

	glutSwapBuffers();
}

int main(int argc, char** argv) {
	srand(time(nullptr));
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(1000, 1000);

	glutInitWindowPosition(200, 100);

	int win = glutCreateWindow("UNREAL 8.0");

	unsigned int res = glewInit();
	if (GLEW_OK != res)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(res));
		return 1;
	}

	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	printf("window id: %d\n", win);

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

	CreateVertexBuffer();
	CreateIndexBuffer();

	glutDisplayFunc(RenderSceneCB);

	glutMainLoop();

	return 0;
}
