#include "pch/pch.h"

#include "util/ExtraMath.h"

using namespace ORNG;

// Point should already be located on sphere with radius 'radius' centered at 'rotation_center' for accurate results
lml::vec3 ExtraMath::AngleAxisRotateAroundPoint(lml::vec3 rotation_center, lml::vec3 point_to_rotate, lml::vec3 axis, float angle) {
	return rotation_center + lml::angleAxis(angle, axis) * (point_to_rotate - rotation_center);
}

lml::quat ExtraMath::MapVectorTransform(const lml::vec3& v, const lml::vec3& w, float interp) {
	lml::vec3 u = lml::normalize(v);
	lml::vec3 t = lml::normalize(w);

	lml::vec3 a = lml::cross(u, t);

	float theta = acos(lml::dot(u, t));

	lml::quat q = lml::angleAxis(theta * interp, lml::normalize(a));

	return q;
}

lml::mat4 ExtraMath::CalculateLightSpaceMatrix(const lml::mat4& proj, const lml::mat4& view, lml::vec3 light_dir, float z_mult, float shadow_map_size) {
	auto corners = ExtraMath::GetFrustumCornersWorldSpace(proj, view);

	/* Calculate directional light light view matrix */
	lml::vec3 center = { 0.f,0.f,0.f };

	for (const auto& corner : corners) {
		center += lml::vec3(corner);
	}

	center /= 8.0f;

	// Calculate bounding sphere radius
	float radius = 0.0f;

	for (size_t i = 0; i < 8; i++) {
		float length = lml::length(lml::vec3(corners[i]) - center);
		radius = lml::max(radius, length);
	}

	center = lml::roundMultiple(center, lml::vec3(0.05f));
	radius = lml::roundMultiple(radius, 0.05f);
	const lml::mat4 light_view = lml::lookAt(center + light_dir, center, lml::vec3(0.f, 1.f, 0.f));

	// Find bounding box that fits the sphere
	lml::vec3 radius_vec(radius, radius, radius);

	lml::vec3 max = radius_vec;
	lml::vec3 min = -radius_vec;

	// Reposition z values to include shadows from geometry just outside of the frustum

	min.z = min.z < 0 ? min.z * z_mult : min.z / z_mult;
	max.z = max.z > 0 ? max.z * z_mult : max.z / z_mult;


	lml::mat4 light_proj = lml::ortho(min.x, max.x, min.y, max.y, min.z, max.z);


	lml::vec4 shadow_origin = lml::vec4(0.05f, 0.05f, 0.05f, 1.0f);
	shadow_origin = (light_proj * light_view) * shadow_origin;
	shadow_origin = shadow_origin * (shadow_map_size / 2.0f);

	//find fractional component
	lml::vec4 rounded_origin = lml::round(shadow_origin);
	lml::vec4 round_offset = rounded_origin - shadow_origin;
	round_offset = round_offset * (2.0f / shadow_map_size);

	light_proj[3][0] += round_offset.x;
	light_proj[3][1] += round_offset.y;

	const lml::mat4 dir_light_space_matrix = light_proj * light_view;
	return dir_light_space_matrix;
}

std::array<lml::vec4, 8> ExtraMath::GetFrustumCornersWorldSpace(const lml::mat4& proj, const lml::mat4& view)
{
	std::array<lml::vec4, 8> corners;
	unsigned int index = 0;

	lml::mat4 inv = lml::inverse(proj * view);

	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const lml::vec4 current_corner =
					inv * lml::vec4(
						2.0f * static_cast<float>(x) - 1.0f,
						2.0f * static_cast<float>(y) - 1.0f,
						2.0f * static_cast<float>(z) - 1.0f,
						1.0f);

				corners[index] = (current_corner / current_corner.w);
				index++;
			}
		}
	}

	return corners;
}

lml::mat3 ExtraMath::Init3DRotateTransform(float rotX, float rotY, float rotZ) {
	lml::quat quat(lml::radians(lml::vec3(rotX, rotY, rotZ)));
	return lml::mat3_cast(quat);
}

lml::mat3 ExtraMath::Init3DScaleTransform(float scaleX, float scaleY, float scaleZ) {
	return lml::mat3{
		scaleX, 0.0f, 0.0f,
		0.0f, scaleY, 0.0f,
		0.0f, 0.0f, scaleZ,
	};
}

lml::mat4 ExtraMath::Init3DTranslationTransform(float tranX, float tranY, float tranZ) {
	lml::mat4 translationMatrix(
		1.f, 0.f, 0.f, 0.f,
		0.0f, 1.0f, 0.0f, 0.f,
		0.0f, 0.0f, 1.0f, 0.f,
		tranX, tranY, tranZ, 1.0f
	);

	return translationMatrix;
}

lml::vec3 ExtraMath::ScreenCoordsToRayDir(lml::mat4 proj_matrix, lml::vec2 coords, lml::vec3 cam_pos, lml::vec3 cam_forward,
	lml::vec3 cam_up, int window_width, int window_height) {
	lml::vec2 norm = coords / lml::vec2(window_width, window_height);
	lml::vec4 clipspace = lml::vec4(lml::vec3(norm.x, norm.y, 1.0)) * 2.f - 1.f;
	lml::vec4 viewspace = lml::inverse(proj_matrix) * clipspace;
	lml::vec4 worldspace = lml::inverse(lml::lookAt(cam_pos, cam_pos + cam_forward, cam_up)) * viewspace;
	return lml::normalize(lml::vec3(worldspace) / worldspace.w - cam_pos);
}
