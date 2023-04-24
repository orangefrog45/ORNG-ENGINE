#include "pch/pch.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>
#include "util/ImGuiLib.h"
#include "../extern/Icons.h"
#include "layers/EditorLayer.h"
#include "rendering/Renderer.h"
#include "rendering/Scene.h"
#include "rendering/MeshInstanceGroup.h"
#include "components/lights/PointLightComponent.h"
#include "components/lights/DirectionalLight.h"
#include "components/lights/SpotLightComponent.h"
#include "components/ScriptComponent.h"
#include "components/SceneEntity.h"
#include "components/MeshComponent.h"
#include "util/Log.h"
#include "Input.h"
#include "rendering/Renderpasses.h"

void EditorLayer::Init() {
	ImGuiLib::Init();

	RenderPasses::InitPasses();
	m_display_quad.Load();
	Renderer::SetActiveScene(*m_active_scene);
	Renderer::SetActiveCamera(&m_editor_camera);
	m_active_scene->Init();
	auto& entity = m_active_scene->CreateEntity("Cube");
	auto& entity2 = m_active_scene->CreateEntity("Cone");
	auto& entity3 = m_active_scene->CreateEntity("Other");

	auto mesh = entity.AddComponent<MeshComponent>("./res/meshes/oranges/orange.obj", Renderer::GetShaderLibrary().GetShader("reflection").GetShaderID());
	auto mesh2 = entity2.AddComponent<MeshComponent>("./res/meshes/light meshes/cone.obj");
	auto mesh3 = entity3.AddComponent<MeshComponent>("./res/meshes/light meshes/cube_light.obj");
	auto script = entity.AddComponent<ScriptComponent>();
	entity.AddComponent<PointLightComponent>();
	entity.AddComponent<SpotLightComponent>();

	script->OnUpdate = [script] {
		auto meshc = script->GetEntity()->GetComponent<MeshComponent>();

		if (RenderPasses::current_entity_id != meshc->GetEntityHandle()) {
			meshc->SetPosition(RenderPasses::current_pos);
		}

	};

	for (int x = -10; x < 10; x++) {
		for (int y = -10; y < 10; y++) {
			for (int z = -10; z < 10; z++) {
				auto& ent = m_active_scene->CreateEntity();
				auto m = ent.AddComponent<MeshComponent>("./res/meshes/light meshes/cube_light.obj");
				auto s = ent.AddComponent<ScriptComponent>();
				m->SetShaderID(Renderer::GetShaderLibrary().GetShader("reflection").GetShaderID());
				s->OnUpdate = [s, x, y, z] {
					glm::vec3 current_rot = s->GetEntity()->GetComponent<MeshComponent>()->GetWorldTransform()->GetRotation();
					s->GetEntity()->GetComponent<MeshComponent>()->SetRotation(current_rot + 0.01f * (x + y + z) * static_cast<float>(Input::GetTimeStep()));
				};
				m->SetPosition(x * 300.f, y * 300.f, z * 300.f);
				m->SetScale(250.f, 250.f, 250.f);
			}
		}
	}


	/*script->OnUpdate = [script] {
		static std::vector<glm::vec3> points = { glm::vec3(0, 100, 0), glm::vec3(0, 0, 100), glm::vec3(100, 0, 0), glm::vec3(-100, 0, 0), glm::vec3(0, -100, 0), glm::vec3(0, 0, -100) };
		auto meshc = script->GetEntity()->GetComponent<MeshComponent>();
		glm::vec3 pos = meshc->GetWorldTransform()->GetPosition();
		static int active_point = 0;

		glm::vec3 point_vector = glm::normalize(points[active_point] - pos);
		float distance = glm::length(points[active_point] - pos);
		if (distance <= 5.f) {
			if (active_point == points.size() - 1) {
				active_point = 0;
			}
			else {
				active_point++;
			}
		}

		meshc->SetPosition(pos + point_vector * 0.1f * static_cast<float>(Input::GetTimeStep()));
	};*/

	m_active_scene->LoadScene(&m_editor_camera);


	mesh->SetScale(10.f, 10.f, 10.0f);

	OAR_CORE_INFO("Editor layer initialized");
}


void EditorLayer::Update() {
	if (!ImGui::GetIO().WantTextInput) {
		m_editor_camera.Update();
	}
	m_active_scene->m_terrain.UpdateTerrainQuadtree();
	m_active_scene->RunUpdateScripts();
}

void EditorLayer::ShowDisplayWindow() {

	glm::mat4 cam_mat = m_editor_camera.GetViewMatrix();
	glm::mat4 proj_mat = m_editor_camera.GetProjectionMatrix();
	Renderer::GetShaderLibrary().SetMatrixUBOs(proj_mat, cam_mat);

	RenderPasses::DoGBufferPass();

	if (Input::IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && !ImGui::GetIO().WantCaptureMouse) {
		RenderPasses::DoPickingPass();
		RenderPasses::gbuffer_positions_sample_flag = true;
	}

	RenderPasses::DoLightingPass();
	RenderPasses::DoDrawToQuadPass(&m_display_quad);

	OAR_CORE_INFO("DRAW CALLS: {0}", Renderer::Get().m_draw_call_amount);
	Renderer::Get().m_draw_call_amount = 0;
}

void EditorLayer::ShowAssetManager() {
	ImGui::SetNextWindowSize(ImVec2(1500, 400));
	ImGui::Begin("Assets");
	ImGui::AlignTextToFramePadding();
	ImGui::BeginTabBar("Selection");
	static std::string path = "./res/meshes";
	static std::string entry_name = "";
	static bool click_flag = false;
	static std::string error_message = "";


	if (ImGui::BeginTabItem("Meshes"))
	{

		if (ImGui::TreeNode("Add mesh"))
		{

			ImGui::TextColored(ImVec4(1, 0, 0, 1), error_message.c_str());

			static std::string file_extension = "";
			ImGuiLib::ShowFileExplorer(path, entry_name, click_flag);

			if (click_flag)
			{
				file_extension = entry_name.find('.') != std::string::npos ? entry_name.substr(entry_name.find_last_of('.')) : "";
				std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::toupper);

				if (file_extension == ".OBJ" || file_extension == ".FBX")
				{
					MeshAsset* asset = m_active_scene->CreateMeshAsset(path + "/" + entry_name);
					asset->LoadMeshData();
					m_active_scene->LoadMeshAssetIntoGPU(asset);
					error_message.clear();
				}
				else
				{
					error_message = file_extension.empty() ? "" : "Invalid file type: " + file_extension; // if file extension is empty it's a directory, so no error
				}
				click_flag = false;
			}

			ImGui::TreePop();
		}
		if (ImGui::BeginTable("Meshes", 4, ImGuiTableFlags_Borders))
		{
			for (auto& data : m_active_scene->m_mesh_assets)
			{
				ImGui::PushID(data);
				ImGui::TableNextColumn();
				ImGui::Text(data->GetFilename().substr(data->GetFilename().find_last_of('/') + 1).c_str());
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 0.5));

				if (ImGui::SmallButton("X"))
				{
					m_active_scene->DeleteMeshAsset(data);
				}

				ImGui::PopStyleColor();
				if (data->is_loaded)
				{
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOADED");
				}
				else
				{
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "NOT LOADED");
				}
				ImGui::PopID();
			}

			ImGui::EndTable();
			ImGui::EndTabItem();
		}
	}

	static Texture2D* selected_texture = nullptr;
	static Texture2DSpec current_spec;

	if (ImGui::BeginTabItem("Textures"))
	{
		if (ImGui::TreeNode("Add texture")) {

			ImGui::TextColored(ImVec4(1, 0, 0, 1), error_message.c_str());

			static std::string file_extension = "";
			ImGuiLib::ShowFileExplorer(path, entry_name, click_flag);

			if (click_flag) /* If clicked create Texture2D asset if file is valid */
			{
				file_extension = entry_name.find('.') != std::string::npos ? entry_name.substr(entry_name.find_last_of('.')) : "";
				std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::toupper);

				if (file_extension == ".PNG" || file_extension == ".JPG")
				{
					m_active_scene->CreateTexture2DAsset(path + "/" + entry_name);
					error_message.clear();
				}
				else
				{
					error_message = file_extension.empty() ? "" : "Invalid file type: " + file_extension; // if file extension is empty it's a directory, so no error
				}
				click_flag = false;
			}
			ImGui::TreePop();
		}

		if (selected_texture) {
			ImGuiLib::ShowTextureEditor(selected_texture, current_spec);
		}

		if (ImGui::TreeNode("Texture viewer"))
		{
			ImGui::GetStyle().CellPadding = ImVec2(11.f, 15.f);

			if (ImGui::BeginChild("##aeifd", ImVec2(1100, 350))) {
				/* Create table for textures */
				if (ImGui::BeginTable("Textures", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_PadOuterX, ImVec2(1100, 300), 250.f));
				{
					/* Push textures into table */
					for (auto& data : m_active_scene->m_texture_2d_assets)
					{
						ImGui::PushID(data);
						ImGui::TableNextColumn();

						ImGui::Text(data->m_filename.substr(data->m_filename.find_last_of('/') + 1).c_str());
						ImGui::SameLine();
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.2, 0.8, 0.5));
						if (ImGui::SmallButton("EDIT"))
						{
							selected_texture = data;
							current_spec = selected_texture->m_spec;
						}
						ImGui::PopStyleColor();


						if (data->m_texture_obj != 0)
						{
							ImGui::TextColored(ImVec4(0, 1, 0, 1), "LOADED");
						}
						else
						{
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "NOT LOADED");
						}
						ImGui::Image(ImTextureID(data->GetTextureRef()), ImVec2(250, 250));
						ImGui::PopID();
					}

					ImGui::GetStyle().CellPadding = ImVec2(4.f, 4.f);
					ImGui::EndTable();
				}
				ImGui::EndChild();
			};
			ImGui::TreePop();
		}
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
	ImGui::End();
}

void EditorLayer::ShowUIWindow() {
	CreateBaseWindow();

	DisplayEntityEditor();
	DisplaySceneEntities();
	//DisplaySceneData();
	//DisplayDebugControls(Renderer::m_framebuffer_library.dir_depth_fb.GetDepthMap());
	DisplayDirectionalLightControls();

	ImGui::End();

	ShowAssetManager();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::CreateBaseWindow() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Tools", nullptr, ImGuiWindowFlags_NoResize);
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

}


void EditorLayer::DisplaySceneEntities() {
	if (ImGui::CollapsingHeader("Scene")) {
		for (auto& entity : m_active_scene->m_entities) {
			if (entity && ImGui::Button(entity->name, ImVec2(200, 25))) {
				RenderPasses::current_entity_id = entity->GetID();
			}
		}
	};

}

void EditorLayer::DisplayEntityEditor() {
	auto entity = m_active_scene->GetEntity(RenderPasses::current_entity_id);
	if (!entity) return;

	auto meshc = entity->GetComponent<MeshComponent>();
	auto plight = entity->GetComponent<PointLightComponent>();
	auto slight = entity->GetComponent<SpotLightComponent>();

	if (ImGui::CollapsingHeader("Entity editor")) {
		std::string ent_text = std::format("Entity '{}'", entity->name);
		ImGui::Text(ent_text.c_str());

		if (meshc) {

			if (ImGui::TreeNode("Mesh")) {
				ImGui::Text("Mesh asset name");
				ImGui::SameLine();
				static std::string input_filename;
				ImGui::InputText("##filename input", &input_filename);

				if (ImGui::Button("Link asset")) {

					for (auto& mesh_asset : m_active_scene->m_mesh_assets) {
						std::string filename = mesh_asset->GetFilename().substr(mesh_asset->GetFilename().find_last_of('/') + 1);
						std::transform(filename.begin(), filename.end(), filename.begin(), ::toupper);
						std::transform(input_filename.begin(), input_filename.end(), input_filename.begin(), ::toupper);

						if (filename == input_filename) {
							m_active_scene->SortMeshIntoInstanceGroup(meshc, mesh_asset);
							meshc = entity->GetComponent<MeshComponent>();
						}

					}
				};

				if (meshc->mp_mesh_asset) {
					Shader& highlight_shader = Renderer::GetShaderLibrary().GetShader("highlight");
					highlight_shader.ActivateProgram();
					highlight_shader.SetUniform("transform", meshc->GetWorldTransform()->GetMatrix());
					Renderer::DrawMesh(meshc->GetMeshData(), false);

					m_selected_mesh_data.scale = meshc->GetWorldTransform()->GetScale();
					m_selected_mesh_data.position = meshc->GetWorldTransform()->GetPosition();
					m_selected_mesh_data.rotation = meshc->GetWorldTransform()->GetRotation();

					ImGuiLib::ShowVec3Editor("Position", m_selected_mesh_data.position);

					ImGuiLib::ShowVec3Editor("Rotation", m_selected_mesh_data.rotation);

					ImGuiLib::ShowVec3Editor("Scale", m_selected_mesh_data.scale);


					entity->GetComponent<MeshComponent>()->SetPosition(m_selected_mesh_data.position);
					entity->GetComponent<MeshComponent>()->SetScale(m_selected_mesh_data.scale);
					entity->GetComponent<MeshComponent>()->SetRotation(m_selected_mesh_data.rotation);

				}
				ImGui::TreePop();
			}

		}


		if (plight) {
			if (ImGui::TreeNode("Pointlight")) {
				ImGui::SameLine();
				if (ImGui::Button("X", ImVec2(25, 25))) {
					entity->DeleteComponent<PointLightComponent>();
				};
				DisplayPointLightControls(plight);
				ImGui::TreePop();
			}
		}
		else {
			ImGui::Text("Pointlight");
			ImGui::SameLine();
			if (ImGui::Button("+", ImVec2(25, 25))) {
				entity->AddComponent<PointLightComponent>();
			}
		}

		if (slight) {
			if (ImGui::TreeNode("Spotlight")) {
				DisplaySpotlightControls(slight);
				ImGui::TreePop();
			}
		}
		else {
			ImGui::Text("Spotlight");
			ImGui::SameLine();
			if (ImGui::Button("+", ImVec2(25, 25))) {
				entity->AddComponent<SpotLightComponent>();
			}
		}
	}
}

void EditorLayer::DisplaySpotlightControls(SpotLightComponent* light) {

	m_spotlight_config_data.aperture = glm::degrees(acosf(light->aperture));
	m_spotlight_config_data.direction = light->m_light_direction_vec;
	m_spotlight_config_data.base.atten_constant = light->attenuation.constant;
	m_spotlight_config_data.base.atten_exp = light->attenuation.exp;
	m_spotlight_config_data.base.atten_linear = light->attenuation.linear;
	m_spotlight_config_data.base.color = light->color;
	m_spotlight_config_data.base.max_distance = light->max_distance;
	m_spotlight_config_data.base.pos = light->transform.GetPosition();

	ImGui::PushItemWidth(200.f);
	ImGui::SliderFloat("constant", &m_spotlight_config_data.base.atten_constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &m_spotlight_config_data.base.atten_linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &m_spotlight_config_data.base.atten_exp, 0.0f, 0.1f);
	ImGui::SliderFloat("max distance", &m_spotlight_config_data.base.max_distance, 0.0f, 200.0f);

	ImGui::Text("Aperture");
	ImGui::SameLine();
	ImGui::SliderFloat("##a", &m_spotlight_config_data.aperture, 0.f, 90.f);

	ImGui::PopItemWidth();

	ImGuiLib::ShowVec3Editor("Direction", m_spotlight_config_data.direction);

	ImGuiLib::ShowColorVec3Editor("Color", m_spotlight_config_data.base.color);
	ImGuiLib::ShowVec3Editor("Position", m_spotlight_config_data.base.pos);

	light->attenuation.constant = m_spotlight_config_data.base.atten_constant;
	light->attenuation.linear = m_spotlight_config_data.base.atten_linear;
	light->attenuation.exp = m_spotlight_config_data.base.atten_exp;
	light->max_distance = m_spotlight_config_data.base.max_distance;
	light->color = m_spotlight_config_data.base.color;
	light->transform.SetPosition(m_spotlight_config_data.base.pos);
	light->SetLightDirection(m_spotlight_config_data.direction.x, m_spotlight_config_data.direction.y, m_spotlight_config_data.direction.z);
	light->SetAperture(m_spotlight_config_data.aperture);
}

void EditorLayer::DisplayDebugControls(unsigned int depth_map_texture) {
	ImGui::Text("DEBUG CONTROLS");
	ImGui::Checkbox("Toggle depth view", &m_debug_config_data.depth_map_view);

	if (m_debug_config_data.depth_map_view == true) {
		ImGui::End();
		ImGui::Begin("DEPTH MAP");
		ImGui::Image((void*)depth_map_texture, ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		ImGui::Begin("Tools");
	}
}

void EditorLayer::DisplayDirectionalLightControls() {

	if (ImGui::CollapsingHeader("Directional light")) {
		ImGui::Text("DIR LIGHT CONTROLS");
		ImGui::PushItemWidth(200.f);
		ImGui::SliderFloat("x-pos", &m_dir_light_data.light_position.x, -1.0f, 1.0f);
		ImGui::SliderFloat("y-pos", &m_dir_light_data.light_position.y, -1.0f, 1.0f);
		ImGui::SliderFloat("z-pos", &m_dir_light_data.light_position.z, -1.0f, 1.0f);
		ImGuiLib::ShowColorVec3Editor("Color", m_dir_light_data.light_color);
		ImGui::PopItemWidth();

		m_active_scene->m_directional_light.color = glm::vec3(m_dir_light_data.light_color.x, m_dir_light_data.light_color.y, m_dir_light_data.light_color.z);
		m_active_scene->m_directional_light.SetLightDirection(m_dir_light_data.light_position);
	}
}


void EditorLayer::DisplayPointLightControls(PointLightComponent* light) {

	m_pointlight_config_data.color = light->color;
	m_pointlight_config_data.atten_exp = light->attenuation.exp;
	m_pointlight_config_data.atten_linear = light->attenuation.linear;
	m_pointlight_config_data.atten_constant = light->attenuation.constant;
	m_pointlight_config_data.max_distance = light->max_distance;

	ImGui::PushItemWidth(200.f);
	ImGui::SliderFloat("constant", &m_pointlight_config_data.atten_constant, 0.0f, 1.0f);
	ImGui::SliderFloat("linear", &m_pointlight_config_data.atten_linear, 0.0f, 1.0f);
	ImGui::SliderFloat("exp", &m_pointlight_config_data.atten_exp, 0.0f, 0.1f);
	ImGui::SliderFloat("max distance", &m_pointlight_config_data.max_distance, 0.0f, 200.0f);
	ImGui::PopItemWidth();
	ImGuiLib::ShowColorVec3Editor("Color", m_pointlight_config_data.color);
	ImGuiLib::ShowVec3Editor("Position", m_pointlight_config_data.pos);

	light->attenuation.constant = m_pointlight_config_data.atten_constant;
	light->attenuation.linear = m_pointlight_config_data.atten_linear;
	light->attenuation.exp = m_pointlight_config_data.atten_exp;
	light->max_distance = m_pointlight_config_data.max_distance;
	light->color = m_pointlight_config_data.color;
	light->transform.SetPosition(m_pointlight_config_data.pos);
}

void EditorLayer::DisplaySceneData() {

	unsigned int total_vertices = 0;
	unsigned int total_lights = 0;

	total_lights += m_active_scene->GetSpotLights().size();
	total_lights += m_active_scene->GetPointLights().size();

	for (const auto& group : m_active_scene->GetGroupMeshEntities()) {
		//total_vertices += group->GetInstances() * group->GetMeshData()->GetIndicesCount();
	}

	m_scene_data.total_vertices = total_vertices;
	m_scene_data.num_lights = total_lights;

	ImGui::Text("SCENE INFO");
	ImGui::Text(std::format("VERTEX COUNT: {}", m_scene_data.total_vertices).c_str());
	ImGui::Text(std::format("LIGHT COUNT: {}", m_scene_data.num_lights).c_str());
};

