#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include "EditorLayer.h"
#include "Renderer.h"
#include "Scene.h"
#include "MeshInstanceGroup.h"
#include "Log.h"

void EditorLayer::Init()
{
	m_display_quad.Load();
	m_active_scene->Init();
	// auto& orange = m_active_scene->CreateMeshComponent("./res/meshes/house-draft/source/house.fbx");
	auto &cube = m_active_scene->CreateMeshComponent("./res/meshes/cube/cube.obj");
	mp_renderer->m_active_camera = &m_active_scene->m_active_camera;
	// orange.SetScale(5.f, 5.f, 5.f);
	// orange.SetPosition(0.0f, 190.0f, 0.0f);
	auto &light = m_active_scene->CreatePointLight();
	light.SetPosition(0.f, 100.0f, -100.0f);
	light.SetColor(1, 0, 0);

	m_active_scene->LoadScene();
	// orange.GetMeshData()->m_materials[0].diffuse_color = glm::vec3(1);
	OAR_CORE_INFO("Editor layer initialized");
}

void EditorLayer::ShowDisplayWindow()
{
	m_active_scene->m_terrain.UpdateTerrainQuadtree();
	mp_renderer->RenderScene(*m_active_scene, m_display_quad);
}

void EditorLayer::ShowUIWindow()
{
	CreateBaseWindow();
	DisplaySceneData();
	DisplayPointLightControls();
	DisplayDebugControls(mp_renderer->m_framebuffer_library.salt_dir_depth_fb.GetDepthMap());
	DisplayDirectionalLightControls();
	DisplayTerrainConfigControls();

	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::CreateBaseWindow()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	{
		ImGui::Begin("Tools");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
}

void EditorLayer::DisplayTerrainConfigControls()
{
	static TerrainConfigData saved_data;

	ImGui::Text("TERRAIN CONTROLS");
	ImGui::InputInt("Seed", &m_terrain_config_data.seed);
	ImGui::SliderInt("Resolution", &m_terrain_config_data.resolution, 1, 10);
	ImGui::SliderFloat("Sampling density", &m_terrain_config_data.sampling_density, 1, 200);
	ImGui::SliderFloat("Height scale", &m_terrain_config_data.height_scale, -100, 100);

	if (!(saved_data == m_terrain_config_data))
	{
		// m_active_scene->m_terrain.UpdateTerrain(m_terrain_config_data.seed, 10000, 10000, glm::vec3(0, 0, 0), m_terrain_config_data.resolution, m_terrain_config_data.height_scale, m_terrain_config_data.sampling_density);
		saved_data = m_terrain_config_data;
	}
}

void EditorLayer::DisplayDebugControls(unsigned int depth_map_texture)
{
	ImGui::Text("DEBUG CONTROLS");
	ImGui::Checkbox("Toggle depth view", &m_debug_config_data.depth_map_view);

	if (m_debug_config_data.depth_map_view == true)
	{
		ImGui::End();
		ImGui::Begin("DEPTH MAP");
		ImGui::Image((void *)depth_map_texture, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		ImGui::Begin("Tools");
	}
}

void EditorLayer::DisplayDirectionalLightControls()
{

	ImGui::Text("DIR LIGHT CONTROLS");
	ImGui::SliderFloat("x-pos", &m_dir_light_data.light_position.x, -1.0f, 1.0f);
	ImGui::SliderFloat("y-pos", &m_dir_light_data.light_position.y, -1.0f, 1.0f);
	ImGui::SliderFloat("z-pos", &m_dir_light_data.light_position.z, -1.0f, 1.0f);
	ImGui::SliderFloat("r", &m_dir_light_data.light_color.x, 0.0f, 1.0f);
	ImGui::SliderFloat("g", &m_dir_light_data.light_color.y, 0.0f, 1.0f);
	ImGui::SliderFloat("b", &m_dir_light_data.light_color.z, 0.0f, 1.0f);

	m_active_scene->m_directional_light.SetColor(m_dir_light_data.light_color.x, m_dir_light_data.light_color.y, m_dir_light_data.light_color.z);
	m_active_scene->m_directional_light.SetLightDirection(m_dir_light_data.light_position);
}

void EditorLayer::DisplayPointLightControls()
{
	ImGui::Text("POINTLIGHT CONTROLS");
	ImGui::SliderFloat("constant", &m_light_config_data.atten_constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &m_light_config_data.atten_linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &m_light_config_data.atten_exp, 0.0f, 0.1f);
	ImGui::SliderFloat("max distance", &m_light_config_data.max_distance, 0.0f, 200.0f);
	ImGui::Text(std::format("Pointlights: {}", m_active_scene->m_point_lights.size()).c_str());
	ImGui::Checkbox("Toggle Pointlights", &m_light_config_data.lights_enabled);

	for (auto &light : m_active_scene->m_point_lights)
	{
		light->SetAttenuation(m_light_config_data.atten_constant, m_light_config_data.atten_linear, m_light_config_data.atten_exp);
		light->SetMaxDistance(m_light_config_data.max_distance);
	}
}

void EditorLayer::DisplaySceneData()
{

	unsigned int total_vertices = 0;
	unsigned int total_lights = 0;

	total_lights += m_active_scene->GetSpotLights().size();
	total_lights += m_active_scene->GetPointLights().size();

	for (const auto &group : m_active_scene->GetGroupMeshEntities())
	{
		total_vertices += group->GetInstances() * group->GetMeshData()->GetIndicesCount();
	}

	m_scene_data.total_vertices = total_vertices;
	m_scene_data.num_lights = total_lights;

	ImGui::Text("SCENE INFO");
	ImGui::Text(std::format("VERTEX COUNT: {}", m_scene_data.total_vertices).c_str());
	ImGui::Text(std::format("LIGHT COUNT: {}", m_scene_data.num_lights).c_str());
};
