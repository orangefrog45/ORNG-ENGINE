#include "pch/pch.h"
#include "util/ExtraMath.h"
#include "glm/glm/gtc/quaternion.hpp"
#include "glm/glm/gtc/round.hpp"


namespace ORNG {

	glm::mat3 ExtraMath::Init2DScaleTransform(float x, float y) {
		glm::mat3 scale_matrix(
			x, 0.0f, 0.0f,
			0.0f, y, 0.0f,
			0.0f, 0.0f, 1.0f
		);

		return scale_matrix;
	}

	glm::mat3 ExtraMath::Init2DRotateTransform(float rot) {
		float rx = glm::radians(rot);

		glm::mat3 rot_matrix(
			cosf(rot), -sin(rot), 0.0f,
			sin(rot), cos(rot), 0.0f,
			0.0f, 0.0f, 1.0f
		);

		return rot_matrix;
	}

	glm::mat3 ExtraMath::Init2DTranslationTransform(float x, float y) {

		glm::mat3 translation_matrix(
			1.0f, 0.0f, x,
			0.0f, 1.0f, y,
			0.0f, 0.0f, 1.0f
		);

		return translation_matrix;
	}




	glm::mat4 ExtraMath::CalculateLightSpaceMatrix(const glm::mat4& proj, const glm::mat4& view, glm::vec3 light_dir, float z_mult, float shadow_map_size)
	{
		auto corners = ExtraMath::GetFrustumCornersWorldSpace(proj, view);

		/* Calculate directional light light view matrix */
		glm::vec3 center = { 0.f,0.f,0.f };

		for (const auto& corner : corners) {
			center += glm::vec3(corner);
		}

		center /= 8.0f;

		// Calculate bounding sphere radius
		float radius = 0.0f;

		for (int i = 0; i < 8; i++)
		{
			float length = glm::length(glm::vec3(corners[i]) - center);
			radius = glm::max(radius, length);
		}

		center = glm::roundMultiple(center, glm::vec3(0.05f));
		radius = glm::roundMultiple(radius, 0.05f);
		const glm::mat4 light_view = glm::lookAt(center + light_dir, center, glm::vec3(0.f, 1.f, 0.f));

		// Find bounding box that fits the sphere
		glm::vec3 radius_vec(radius, radius, radius);

		glm::vec3 max = radius_vec;
		glm::vec3 min = -radius_vec;

		// Reposition z values to include shadows from geometry just outside of the frustum 

		min.z = min.z < 0 ? min.z * z_mult : min.z / z_mult;
		max.z = max.z > 0 ? max.z * z_mult : max.z / z_mult;


		glm::mat4 light_proj = glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);


		glm::vec4 shadow_origin = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
		shadow_origin = (light_proj * light_view) * shadow_origin;
		shadow_origin = shadow_origin * (shadow_map_size / 2.0f);

		//find fractional component 
		glm::vec4 rounded_origin = glm::round(shadow_origin);
		glm::vec4 round_offset = rounded_origin - shadow_origin;
		round_offset = round_offset * (2.0f / shadow_map_size);

		light_proj[3][0] += round_offset.x;
		light_proj[3][1] += round_offset.y;

		const glm::mat4 dir_light_space_matrix = light_proj * light_view;
		return dir_light_space_matrix;
	}

	std::array<glm::vec4, 8> ExtraMath::GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
	{
		std::array<glm::vec4, 8> corners;
		unsigned int index = 0;

		glm::mat4 inv = glm::inverse(proj * view);

		for (unsigned int x = 0; x < 2; ++x)
		{
			for (unsigned int y = 0; y < 2; ++y)
			{
				for (unsigned int z = 0; z < 2; ++z)
				{
					const glm::vec4 current_corner =
						inv * glm::vec4(
							2.0f * x - 1.0f,
							2.0f * y - 1.0f,
							2.0f * z - 1.0f,
							1.0f);

					corners[index] = (current_corner / current_corner.w);
					index++;
				}
			}
		}

		return corners;
	}

	glm::mat4x4 ExtraMath::Init3DRotateTransform(float rotX, float rotY, float rotZ) {

		glm::quat quat(glm::radians(glm::vec3(rotX, rotY, rotZ)));
		glm::mat4 rotation_matrix = glm::mat4_cast(quat);

		return rotation_matrix;
	}

	glm::mat4x4 ExtraMath::Init3DScaleTransform(float scaleX, float scaleY, float scaleZ) {

		glm::mat4x4 scaleMatrix(
			scaleX, 0.0f, 0.0f, 0.0f,
			0.0f, scaleY, 0.0f, 0.0f,
			0.0f, 0.0f, scaleZ, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);

		return scaleMatrix;

	}

	glm::mat4x4 ExtraMath::Init3DTranslationTransform(float tranX, float tranY, float tranZ) {

		glm::mat4x4 translationMatrix(
			1.f, 0.f, 0.f, 0.f,
			0.0f, 1.0f, 0.0f, 0.f,
			0.0f, 0.0f, 1.0f, 0.f,
			tranX, tranY, tranZ, 1.0f
		);

		return translationMatrix;

	}



	glm::vec3 ExtraMath::ScreenCoordsToRayDir(glm::mat4 proj_matrix, glm::vec2 coords, glm::vec3 cam_pos, glm::vec3 cam_forward, glm::vec3 cam_up, unsigned int window_width, unsigned int window_height) {

		glm::vec2 norm = coords / glm::vec2(window_width, window_height);
		glm::vec4 clipspace = glm::vec4(norm, 1.0, 1.0) * 2.f - 1.f;
		glm::vec4 viewspace = glm::inverse(proj_matrix) * clipspace;
		glm::vec4 worldspace = glm::inverse(glm::lookAt(cam_pos, cam_pos + cam_forward, cam_up)) * viewspace;
		return glm::normalize(glm::vec3(worldspace) - cam_pos);
	}

}