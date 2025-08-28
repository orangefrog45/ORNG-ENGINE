#include "RuntimeLayer.h"

int main() {
	ORNG::Application app;
	ORNG::RuntimeLayer rt;

	ORNG::ApplicationData app_data{};

	app.layer_stack.PushLayer(&rt);
	app.Init(app_data);

	return 0;
}