#include <GLErrorHandling.h>
#include "Renderer.h"
#include "TimeStep.h"
#include "Log.h"
#include "Scene.h"
#include "Quad.h"
#include "MeshComponent.h"
#include "MeshInstanceGroup.h"

void Renderer::Init()
{
	TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

	m_resources.Init();
	m_framebuffer_library.Init();
	m_shader_library.Init();

	OAR_CORE_INFO("Renderer initialized in {0}ms", time.GetTimeInterval());
}

void Renderer::RenderSkybox(Skybox &skybox)
{
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	m_shader_library.skybox_shader.ActivateProgram();
	RendererResources::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.m_cubemap_texture.m_texture_obj, RendererResources::TextureUnits::COLOR);

	glBindVertexArray(skybox.m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
}

void Renderer::DrawQuad(Quad &quad)
{
	GLCall(glBindVertexArray(quad.m_vao));
	GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
	GLCall(glBindVertexArray(0));
}

void Renderer::RenderScene(Scene &scene, Quad &quad)
{

	/* Update UBO's immediately to stop out-of-sync draws */
	glm::fmat4 cam_mat = scene.m_active_camera.GetViewMatrix();
	glm::fmat4 proj_mat = scene.m_active_camera.GetProjectionMatrix();
	m_shader_library.SetMatrixUBOs(proj_mat, cam_mat);

	// SHADOW MAP PASS
	DrawShadowMap(scene);

	// GBUFFER PASS
	glClearColor(10000, 10000, 10000, 10000);
	m_framebuffer_library.salt_deferred_fb.BindForDraw();
	m_shader_library.g_buffer_shader.ActivateProgram();
	glClearColor(0, 0, 0, 0);
	DrawLightingGroups(scene, m_shader_library.g_buffer_shader);
	DrawReflectGroups(scene, m_shader_library.g_buffer_shader);
	DrawTerrain(scene.m_terrain);

	// MAIN RENDER PASS
	glViewport(0, 0, RendererResources::GetWindowWidth(), RendererResources::GetWindowHeight());

	m_shader_library.reflection_shader.ActivateProgram();
	m_framebuffer_library.main_view_framebuffer.Bind();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RendererResources::BindTexture(GL_TEXTURE_2D, m_framebuffer_library.salt_dir_depth_fb.GetDepthMap(), RendererResources::TextureUnits::DIR_SHADOW_MAP);
	RendererResources::BindTexture(GL_TEXTURE_2D_ARRAY, m_framebuffer_library.salt_spotlight_depth_fb.GetDepthMap(), RendererResources::TextureUnits::SPOT_SHADOW_MAP);

	RenderLightMeshVisuals(scene);
	RenderLightingEntities(scene);
	RenderReflectShaderEntities(scene);

	RenderSkybox(scene.m_skybox);
	RenderGrid(scene);

	m_framebuffer_library.UnbindAllFramebuffers();

	// RENDER TO QUAD
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	m_shader_library.basic_sampler_shader.ActivateProgram();
	RendererResources::BindTexture(GL_TEXTURE_2D, m_framebuffer_library.main_view_framebuffer.GetTexture(), RendererResources::TextureUnits::COLOR);
	RendererResources::BindTexture(GL_TEXTURE_2D, m_framebuffer_library.salt_deferred_fb.GetGeometryTexture(), RendererResources::TextureUnits::WORLD_POSITIONS);
	RendererResources::BindTexture(GL_TEXTURE_2D, m_framebuffer_library.main_view_framebuffer.GetDepthTexture(), RendererResources::TextureUnits::DIR_SHADOW_MAP);
	m_shader_library.basic_sampler_shader.SetCameraPos(scene.m_active_camera.GetPos());
	m_shader_library.basic_sampler_shader.SetCameraDir(scene.m_active_camera.GetTarget());

	m_shader_library.basic_sampler_shader.SetTransform(quad.GetTransform().GetMatrix());
	DrawQuad(quad);

	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawTerrain(const Terrain &terrain)
{

	std::vector<TerrainQuadtree *> node_array;
	int total_chunks = 0;
	int drawn_chunks = 0;
	int total_verts = 0;
	int drawn_verts = 0;
	terrain.m_quadtree->QueryChunks(node_array, m_active_camera->GetPos(), terrain.m_width);
	for (auto &node : node_array)
	{
		total_chunks++;
		total_verts += node->m_chunk->m_terrain_data.positions.size();
		if (node->m_chunk->m_terrain_data.bounding_box.IsOnFrustum(m_active_camera->m_view_frustum))
		{
			drawn_verts += node->m_chunk->m_terrain_data.positions.size();
			drawn_chunks++;
			glBindVertexArray(node->m_chunk->m_vao);

			GLCall(glDrawElements(GL_QUADS,
								  node->m_chunk->m_terrain_data.indices.size(),
								  GL_UNSIGNED_INT,
								  nullptr));

			glBindVertexArray(0);
		}
	}
	OAR_CORE_TRACE("CHUNKS CULLED : {0}/{1}, VERTS CULLED : {2}/{3}", total_chunks - drawn_chunks, total_chunks, total_verts - drawn_verts, total_verts);
}

void Renderer::DrawShadowMap(Scene &scene)
{
	// BIND FOR DRAW
	m_shader_library.depth_shader.ActivateProgram();

	// DIRECTIONAL LIGHT
	m_framebuffer_library.salt_dir_depth_fb.BindForDraw();
	glClear(GL_DEPTH_BUFFER_BIT);

	m_shader_library.depth_shader.SetPVMatrix(scene.GetDirectionalLight().GetTransformMatrix());
	DrawLightingGroups(scene, m_shader_library.depth_shader);
	DrawReflectGroups(scene, m_shader_library.depth_shader);
	// DrawTerrain(scene.m_terrain);

	// SPOT LIGHTS
	m_framebuffer_library.salt_spotlight_depth_fb.BindForDraw();
	auto &lights = scene.GetSpotLights();

	for (unsigned int i = 0; i < lights.size(); i++)
	{

		m_framebuffer_library.salt_spotlight_depth_fb.SetDepthTexLayer(i); // index 0 = directional light depth map
		m_shader_library.depth_shader.SetPVMatrix(lights[i]->GetTransformMatrix());
		glClear(GL_DEPTH_BUFFER_BIT);

		DrawLightingGroups(scene, m_shader_library.depth_shader);
		DrawReflectGroups(scene, m_shader_library.depth_shader);
		// DrawTerrain(scene.m_terrain);
	}

	// POINT LIGHTS
	/*m_shader_library.cube_map_shadow_shader.ActivateProgram();
	m_framebuffer_library.pointlight_depth_fb.BindForDraw();
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	glClearColor(0.2f, 0.5f, FLT_MAX, FLT_MAX);
	auto& point_lights = scene.GetPointLights();
	for (int i = 0; i < point_lights.size(); i++) {
		m_shader_library.cube_map_shadow_shader.SetLightPos(point_lights[i].GetWorldTransform().GetPosition());
		for (int y = 0; y < 6; y++) {
			m_framebuffer_library.pointlight_depth_fb.BindForDraw();
			m_framebuffer_library.pointlight_depth_fb.SetCubemapFace(i, y);
			GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

			m_shader_library.cube_map_shadow_shader.SetPVMatrix(point_lights[i].GetLightTransforms()[y]);
			DrawLightingGroups(m_shader_library.cube_map_shadow_shader);
			DrawReflectGroups(m_shader_library.cube_map_shadow_shader);
			glReadPixels(0, 0, 1024, 1024, GL_RED, GL_FLOAT, pixels);

			PrintUtils::PrintDebug(std::to_string(pixels[20000]));
		}
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);*/

	m_framebuffer_library.UnbindAllFramebuffers();
}

void Renderer::RenderLightMeshVisuals(Scene &scene)
{
	m_shader_library.flat_color_shader.ActivateProgram();
	for (const auto &light : scene.GetPointLights())
	{
		glm::fvec3 light_color = light->GetColor();
		m_shader_library.flat_color_shader.SetColor(light_color.x, light_color.y, light_color.z);
		DrawMeshWithShader(scene, light->GetMeshVisual()->GetMeshData(), 1, m_shader_library.flat_color_shader);
	}

	for (const auto &light : scene.GetSpotLights())
	{
		glm::fvec3 light_color = light->GetColor();
		m_shader_library.flat_color_shader.SetColor(light_color.x, light_color.y, light_color.z);
		DrawMeshWithShader(scene, light->GetMeshVisual()->GetMeshData(), 1, m_shader_library.flat_color_shader);
	}
}

void Renderer::RenderReflectShaderEntities(Scene &scene)
{
	m_shader_library.reflection_shader.ActivateProgram();
	m_shader_library.reflection_shader.SetCameraPos(scene.m_active_camera.GetPos());
	RendererResources::BindTexture(GL_TEXTURE_CUBE_MAP, scene.m_skybox.GetCubeMapTexture().GetTextureRef(), RendererResources::TextureUnits::COLOR);
	DrawReflectGroups(scene, m_shader_library.reflection_shader);
}

void Renderer::RenderLightingEntities(Scene &scene)
{

	m_shader_library.lighting_shader.ActivateProgram();

	m_shader_library.lighting_shader.SetPointLights(scene.GetPointLights());
	m_shader_library.lighting_shader.SetSpotLights(scene.GetSpotLights());
	m_shader_library.lighting_shader.SetAmbientLight(scene.GetAmbientLighting());
	m_shader_library.lighting_shader.SetDirectionLight(scene.GetDirectionalLight());
	m_shader_library.lighting_shader.SetViewPos(scene.m_active_camera.GetPos());
	m_shader_library.lighting_shader.SetLightSpaceMatrix(scene.GetDirectionalLight().GetTransformMatrix());

	DrawLightingGroups(scene, m_shader_library.lighting_shader);

	m_shader_library.lighting_shader.SetMaterial(scene.m_terrain.m_terrain_top_mat);
	RendererResources::BindTexture(GL_TEXTURE_2D_ARRAY, scene.m_terrain.m_texture_array.GetTextureRef(), RendererResources::TextureUnits::TERRAIN_DIFFUSE);
	m_shader_library.lighting_shader.SetTerrainMode(true);
	m_shader_library.lighting_shader.SetNormalMapActive(true);
	DrawTerrain(scene.GetTerrain());
	m_shader_library.lighting_shader.SetTerrainMode(false);
}

void Renderer::RenderGrid(Scene &scene)
{
	m_shader_library.grid_shader.ActivateProgram();
	m_shader_library.grid_shader.SetCameraPos(scene.m_active_camera.GetPos());
	scene.grid_mesh.CheckBoundary(scene.m_active_camera.GetPos());
	scene.grid_mesh.Draw();
}

template <typename T>
void Renderer::DrawLightingGroups(Scene &scene, T &shader)
{
	for (auto group : scene.GetGroupMeshEntities())
	{
		if (group->GetMeshData()->GetLoadStatus() == true && (group->GetShaderType() == MeshData::MeshShaderMode::LIGHTING))
		{
			MeshData *mesh_data = group->GetMeshData();

			if (group->GetMeshData()->GetIsShared() == true)
				group->UpdateMeshTransformBuffers();

			DrawMeshWithShader(scene, mesh_data, group->GetInstances(), shader);
		}
	}
}

template <typename T>
void Renderer::DrawReflectGroups(Scene &scene, T &shader)
{
	for (auto group : scene.GetGroupMeshEntities())
	{
		if (group->GetMeshData()->GetLoadStatus() == true && (group->GetShaderType() == MeshData::MeshShaderMode::REFLECT))
		{
			MeshData *mesh_data = group->GetMeshData();

			if (group->GetMeshData()->GetIsShared() == true)
				group->UpdateMeshTransformBuffers();

			DrawMeshWithShader(scene, mesh_data, group->GetInstances(), shader);
		}
	}
};

template <typename T>
void Renderer::DrawMeshWithShader(Scene &scene, MeshData *mesh_data, unsigned int t_instances, T &shader)
{
	GLCall(glBindVertexArray(mesh_data->m_VAO));

	for (unsigned int i = 0; i < mesh_data->m_meshes.size(); i++)
	{

		unsigned int materialIndex = mesh_data->m_meshes[i].materialIndex;

		shader.SetMaterial(mesh_data->m_materials[materialIndex]);

		GLCall(glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
												 mesh_data->m_meshes[i].numIndices,
												 GL_UNSIGNED_INT,
												 (void *)(sizeof(unsigned int) * mesh_data->m_meshes[i].baseIndex),
												 t_instances,
												 mesh_data->m_meshes[i].baseVertex));
	}
	GLCall(glBindVertexArray(0));
}
