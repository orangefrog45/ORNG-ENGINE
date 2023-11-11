#include "RuntimeLayer.h"

void main() {
	ORNG::Application app;
	ORNG::RuntimeLayer rt;

	app.layer_stack.PushLayer(&rt);
	app.Init();
}