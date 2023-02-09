#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stb/stb_image.h>
#include "Camera.h"
#include "ShaderHandling.h"
#include "ExtraMath.h"
#include "WorldTransform.h"
#include "Texture.h"
#include "MainFramework.h"
#include "KeyboardState.h"
#include "TimeStep.h"


MainFramework mainFramework;


static void CallKeyboardCB(unsigned char key, int x, int y) {
	mainFramework.CallKeyboardCB(key, x, y);
}
static void CallSpecialKeyboardCB(int key, int x, int y) {
	mainFramework.CallSpecialKeyboardCB(key, x, y);
}
static void CallSpecialKeysUp(int key, int x, int y) {
	mainFramework.CallSpecialKeysUp(key, x, y);
}
static void CallKeysUp(unsigned char key, int x, int y) {
	mainFramework.CallKeysUp(key, x, y);
}
static void CallRenderSceneCB() {
	mainFramework.RenderSceneCB();
}
static void CallPassiveMouseCB(int x, int y) {
	mainFramework.PassiveMouseCB(x, y);
}
static void InitializeGlutCallbacks() {
	glutDisplayFunc(CallRenderSceneCB);
	glutKeyboardFunc(CallKeyboardCB);
	glutSpecialFunc(CallSpecialKeyboardCB);
	glutSpecialUpFunc(CallSpecialKeysUp);
	glutKeyboardUpFunc(CallKeysUp);
	glutPassiveMotionFunc(CallPassiveMouseCB);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(mainFramework.WINDOW_WIDTH, mainFramework.WINDOW_HEIGHT);

	glutInitWindowPosition(200, 100);

	int win = glutCreateWindow("UNREAL 8.0");

	char game_mode_string[64];
	snprintf(game_mode_string, sizeof(game_mode_string), "%dx%d@32", mainFramework.WINDOW_WIDTH, mainFramework.WINDOW_HEIGHT);
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

	if (!mainFramework.Init()) {
		return 1;
	}

	glutMainLoop();

	return 0;
}
