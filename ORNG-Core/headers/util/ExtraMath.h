#pragma once


namespace ORNG {
	class DirectionalLight;

	struct Box2D {
		Box2D(glm::vec2 t_min, glm::vec2 t_max) : min(t_min), max(t_max) {};
		Box2D() = default;
		glm::vec2 min;
		glm::vec2 max;
	};

	class ExtraMath
	{
	public:

		struct Plane {
			Plane() = default;
			Plane(glm::vec3 t_normal, glm::vec3 p1) : normal(glm::normalize(t_normal)), distance(glm::dot(normal, p1)) {};
			float GetSignedDistanceToPlane(const glm::vec3& point) const { return glm::dot(normal, point) - distance; }
			glm::vec3 normal = { 0.0f, 1.0f, 0.0f };
			float distance = 0.0f;
		};

		struct Frustum {
			Plane top_plane;
			Plane bottom_plane;
			Plane right_plane;
			Plane left_plane;
			Plane far_plane;
			Plane near_plane;
		};


		static glm::mat4 CalculateLightSpaceMatrix(const glm::mat4& proj, const glm::mat4& view, glm::vec3 light_dir, float z_mult, float shadow_map_size);

		static glm::vec3 AngleAxisRotateAroundPoint(glm::vec3 rotation_center, glm::vec3 point_to_rotate, glm::vec3 axis, float angle);

		static std::array<glm::vec4, 8> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
		static glm::mat3 Init3DRotateTransform(float rotX, float rotY, float rotZ);
		static glm::mat3 Init3DScaleTransform(float scaleX, float scaleY, float scaleZ);
		static glm::mat4x4 Init3DTranslationTransform(float tranX, float tranY, float tranZ);
		static glm::mat3 Init2DScaleTransform(float x, float y);
		static glm::mat3 Init2DRotateTransform(float rot);
		static glm::mat3 Init2DTranslationTransform(float x, float y);

		static glm::vec3 ScreenCoordsToRayDir(glm::mat4 proj_matrix, glm::vec2 coords, glm::vec3 cam_pos, glm::vec3 cam_forward, glm::vec3 cam_up, unsigned int window_width, unsigned int window_height);
	};
}