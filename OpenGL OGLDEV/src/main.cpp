#include <glew.h>
#include <glfw/glfw3.h>
#include "Application.h"




static Application application;

/*static void InitializeGlutCallbacks() {
	glutDisplayFunc([]() {application.RenderSceneCB(); });
	glutKeyboardFunc([](unsigned char key, int x, int y) {application.GetKeyboard()->KeyboardCB(key, x, y); });
	glutSpecialFunc([](int key, int x, int y) {application.GetKeyboard()->SpecialKeyboardCB(key, x, y); });
	glutSpecialUpFunc([](int key, int x, int y) {application.GetKeyboard()->SpecialKeysUp(key, x, y); });
	glutKeyboardUpFunc([](unsigned char key, int x, int y) {application.GetKeyboard()->KeysUp(key, x, y); });
	glutMotionFunc([](int x, int y) {application.PassiveMouseCB(x, y); });
	glutPassiveMotionFunc([](int x, int y) {application.PassiveMouseCB(x, y); });

}*/


int main() {

	application.Init();

	return 0;
}
