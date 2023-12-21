#include "../../ORNG-Editor/headers/EditorLayer.h"
#include "../headers/GameLayer.h"

void main() {
	char buffer[ORNG_MAX_FILEPATH_SIZE];
	GetModuleFileName(nullptr, buffer, ORNG_MAX_FILEPATH_SIZE);
	std::string exe_path = buffer;
	exe_path = exe_path.substr(0, exe_path.find_last_of('\\'));

	ORNG::Application application;
	ORNG::GameLayer game{ };
	ORNG::EditorLayer editor{ &game.p_scene, exe_path + "\\projects\\base-project"};
	game.SetEditor(&editor);

	application.layer_stack.PushLayer(static_cast<ORNG::Layer*>(&game));
	application.layer_stack.PushLayer(static_cast<ORNG::Layer*>(&editor));
	application.Init();
}