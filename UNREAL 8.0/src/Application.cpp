#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glew.h>
#include <glfw/glfw3.h>
#include "Application.h"
#include "util/Log.h"


Application::Application() {
}


void Application::Init() {
	Log::Init();


	//GLFW INIT
	GLFWwindow* window;

	if (!glfwInit())
		exit(1);

	window = glfwCreateWindow(1920, 1080, "UNREAL 8.0", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE); // keys "stick" until they've been polled

	//GLEW INIT
	GLint GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		ORO_CORE_CRITICAL("GLEW INIT FAILED");
		printf("ERROR: %s", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}

	//IMGUI INIT
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.FontDefault = io.Fonts->AddFontFromFileTTF("./res/fonts/PlatNomor-WyVnn.ttf", 18.0f);

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init((char*)glGetString(GL_NUM_SHADING_LANGUAGE_VERSIONS));

	//GL CONFIG
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glLineWidth(3.0);

	//RENDERER INIT
	renderer.Init();


	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		RenderScene();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();



		input_handle.HandleInput(window); // handle mouse-locking, caching key states
		ASSERT(renderer.m_active_camera != nullptr);
		InputHandle::HandleCameraInput(*renderer.m_active_camera, input_handle);

		{ // debug spawn entities
			static int instances = 0;

			if (input_handle.g_down) {

				auto& entity = renderer.scene.CreateMeshComponent("./res/meshes/cube/cube.obj");
				glm::fvec3 place_position = renderer.m_active_camera->GetPos() + (glm::fvec3(renderer.m_active_camera->GetTarget() * 15.0f));
				instances++;
				entity.SetPosition(place_position.x, place_position.y, place_position.z);

				ORO_CORE_TRACE("Positions: {0}", instances);
			}
		}

	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void Application::RenderScene() {

	renderer.RenderWindow();

}
