#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "Application.h"




Application application;

static void InitializeGlutCallbacks() {
	glutDisplayFunc([]() {application.RenderSceneCB(); });
	glutKeyboardFunc([](unsigned char key, int x, int y) {application.GetKeyboard().KeyboardCB(key, x, y); });
	glutSpecialFunc([](int key, int x, int y) {application.GetKeyboard().SpecialKeyboardCB(key, x, y); });
	glutSpecialUpFunc([](int key, int x, int y) {application.GetKeyboard().SpecialKeysUp(key, x, y); });
	glutKeyboardUpFunc([](unsigned char key, int x, int y) {application.GetKeyboard().KeysUp(key, x, y); });
	glutPassiveMotionFunc([](int x, int y) {application.PassiveMouseCB(x, y); });
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);

	if (!application.Init()) {
		return 1;
	}
	InitializeGlutCallbacks();

	glutMainLoop();

	return 0;
}
