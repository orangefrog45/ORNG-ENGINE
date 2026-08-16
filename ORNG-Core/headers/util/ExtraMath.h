#pragma once


namespace ORNG {
	class DirectionalLight;

	struct Box2D {
		Box2D(lml::vec2 t_min, lml::vec2 t_max) : min(t_min), max(t_max) {}
		Box2D() = default;
		lml::vec2 min;
		lml::vec2 max;
	};

	class ExtraMath {
	public:

		struct Plane {
			Plane() = default;
			Plane(lml::vec3 t_normal, lml::vec3 p1) : normal(lml::normalize(t_normal)), distance(lml::dot(normal, p1)) {}
			float GetSignedDistanceToPlane(const lml::vec3& point) const { return lml::dot(normal, point) - distance; }
			lml::vec3 normal = { 0.0f, 1.0f, 0.0f };
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


		// Returns a quaternion that rotates v onto w by factor "interp" (0-1)
		static lml::quat MapVectorTransform(const lml::vec3& v, const lml::vec3& w, float interp = 1.f);

		static lml::mat4 CalculateLightSpaceMatrix(const lml::mat4& proj, const lml::mat4& view,
			lml::vec3 light_dir, float z_mult, float shadow_map_size);

		static lml::vec3 AngleAxisRotateAroundPoint(lml::vec3 rotation_center, lml::vec3 point_to_rotate,
			lml::vec3 axis, float angle);

		static std::array<lml::vec4, 8> GetFrustumCornersWorldSpace(const lml::mat4& proj, const lml::mat4& view);
		static lml::mat3 Init3DRotateTransform(float rotX, float rotY, float rotZ);
		static lml::mat3 Init3DScaleTransform(float scaleX, float scaleY, float scaleZ);
		static lml::mat4 Init3DTranslationTransform(float tranX, float tranY, float tranZ);

		static lml::vec3 ScreenCoordsToRayDir(lml::mat4 proj_matrix, lml::vec2 coords, lml::vec3 cam_pos, lml::vec3 cam_forward,
			lml::vec3 cam_up, int window_width, int window_height);
	};
}
