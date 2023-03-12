#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <math.h>

class ExtraMath
{
public:
	static float ToRadians(float x) {
		return x * ((glm::pi<float>()) / 180.0f);
	}

	static glm::fmat4x4 Init3DRotateTransform(float rotX, float rotY, float rotZ);
	static glm::fmat4x4 Init3DScaleTransform(float scaleX, float scaleY, float scaleZ);
	static glm::fmat4x4 Init3DTranslationTransform(float tranX, float tranY, float tranZ);
	static glm::fmat3 Init2DScaleTransform(float x, float y);
	static glm::fmat3 Init2DRotateTransform(float rot);
	static glm::fmat3 Init2DTranslationTransform(float x, float y);
	static glm::fmat4x4 Init3DCameraTransform(const glm::fvec3& pos, const glm::fvec3& target, const glm::fvec3& up);
	static glm::fmat4 GetCameraTransMatrix(glm::fvec3 pos);
	static glm::fmat4x4 InitPersProjTransform(float FOV, float WINDOW_WIDTH, float WINDOW_HEIGHT, float zNear, float zFar);

};
