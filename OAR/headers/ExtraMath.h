#pragma once
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

	struct Constants {
		inline static double pi = atan(1) * 4;
	};

	static float ToRadians(float x) {
		return x * ((Constants::pi) / 180.0f);
	}

	static glm::mat4x4 Init3DRotateTransform(float rotX, float rotY, float rotZ);
	static glm::mat4x4 Init3DScaleTransform(float scaleX, float scaleY, float scaleZ);
	static glm::mat4x4 Init3DTranslationTransform(float tranX, float tranY, float tranZ);
	static glm::mat3 Init2DScaleTransform(float x, float y);
	static glm::mat3 Init2DRotateTransform(float rot);
	static glm::mat3 Init2DTranslationTransform(float x, float y);
	static glm::mat4x4 Init3DCameraTransform(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up);
	static glm::mat4 GetCameraTransMatrix(glm::vec3 pos);
	static glm::mat4x4 InitPersProjTransform(float FOV, float WINDOW_WIDTH, float WINDOW_HEIGHT, float zNear, float zFar);

};
