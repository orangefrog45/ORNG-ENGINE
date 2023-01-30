#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include <iostream>
#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

unsigned int VBO;


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

static void RenderSceneCB() {
	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	//explicity re-bind to ensure correct handle is used
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));

	//index offset of attribute (coordinates first)
	GLCall(glEnableVertexAttribArray(0));

	//specify format of "enabled" attribute
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));

	//render using bound VBO as input
	GLCall(glDrawArrays(GL_TRIANGLES, 0, 3));

	//now it's drawn, clear state
	GLCall(glDisableVertexAttribArray(0));

	//call render
	glutPostRedisplay();

	glutSwapBuffers();
}

static void CreateVertexBuffer() {
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);

	glm::vec3 Vertices[3];
	Vertices[0] = { -1.0f, -1.0f, 0.0f };
	Vertices[1] = { 0.0f, 1.0f, 0.0f };
	Vertices[2] = { 1.0f, -1.0f, 0.0f };


	//create handle
	GLCall(glGenBuffers(1, &VBO));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
	//knows to look in ARRAY_BUFFER for data which is bound
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW));
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(1920, 1080);

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

	GLclampf Red = 1.0f, Green = 0.0f, Blue = 0.0f, Alpha = 0.0f;
	CreateVertexBuffer();

	glutDisplayFunc(RenderSceneCB);

	glutMainLoop();

	return 0;
}
