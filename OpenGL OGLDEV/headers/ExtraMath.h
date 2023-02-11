#pragma once
#include <glm/glm.hpp>
#include <math.h>

struct PersProjData {

	PersProjData();
	PersProjData(float FOV, float WINDOW_WIDTH, float WINDOW_HEIGHT, float zNear, float zFar);

	float FOV;
	float WINDOW_WIDTH;
	float WINDOW_HEIGHT;
	float zNear;
	float zFar;
};

class ExtraMath
{
public:
	static const double pi;

	static float ToRadians(float x) {
		return x * (pi / 180.0f);
	}

	static glm::fmat4x4 Init3DRotateTransform(float rotX, float rotY, float rotZ);
	static glm::fmat4x4 Init3DScaleTransform(float scaleX, float scaleY, float scaleZ);
	static glm::fmat4x4 Init3DTranslationTransform(float tranX, float tranY, float tranZ);
	static glm::fmat4x4 Init3DCameraTransform(const glm::fvec3& pos, const glm::fvec3& target, const glm::fvec3& up);
	static glm::fmat4 GetCameraTransMatrix(glm::fvec3 pos);
	static glm::fmat4x4 InitPersProjTransform(PersProjData& p);

};
