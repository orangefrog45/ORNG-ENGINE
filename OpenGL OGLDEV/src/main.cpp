#include <stdio.h>
#include <glew.h>
#include <freeglut.h>
#include "MainFramework.h"



MainFramework mainFramework;


static void InitializeGlutCallbacks() {
	glutDisplayFunc([]() {mainFramework.RenderSceneCB(); });
	glutKeyboardFunc([](unsigned char key, int x, int y) {mainFramework.GetKeyboard().KeyboardCB(key, x, y); });
	glutSpecialFunc([](int key, int x, int y) {mainFramework.GetKeyboard().SpecialKeyboardCB(key, x, y); });
	glutSpecialUpFunc([](int key, int x, int y) {mainFramework.GetKeyboard().SpecialKeysUp(key, x, y); });
	glutKeyboardUpFunc([](unsigned char key, int x, int y) {mainFramework.GetKeyboard().KeysUp(key, x, y); });
	glutPassiveMotionFunc([](int x, int y) {mainFramework.PassiveMouseCB(x, y); });
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glewInit();
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(mainFramework.GetWindowWidth(), mainFramework.GetWindowHeight());

	glutInitWindowPosition(200, 100);

	int win = glutCreateWindow("UNREAL 8.0");

	char game_mode_string[64];
	snprintf(game_mode_string, sizeof(game_mode_string), "%dx%d.10@32", mainFramework.GetWindowWidth(), mainFramework.GetWindowHeight());
	glutGameModeString(game_mode_string);
	//glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);
	//glutEnterGameMode();

	unsigned int res = glewInit();
	if (GLEW_OK != res)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(res));
		return 1;
	}

	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	printf("window id: %d\n", win);



	if (!mainFramework.Init()) {
		return 1;
	}
	InitializeGlutCallbacks();

	glutMainLoop();

	return 0;
}
