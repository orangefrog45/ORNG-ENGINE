#include "RuntimeLayer.h"

void main() {
	ORNG::Application app;
	ORNG::RuntimeLayer rt;

	ORNG::ApplicationData app_data{};
	app_data.shader_package_file = "shaders.shaderpkg";

	app.layer_stack.PushLayer(&rt);
	app.Init(app_data);
}