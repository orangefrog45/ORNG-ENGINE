#pragma once
#include <glm/glm.hpp>
struct ViewData {

	ViewData(const glm::fmat4& camMat, const glm::fmat4& invCamMat, const glm::fmat4& projMat, const glm::fvec3 camera_pos) : cameraMatrix(camMat), inverseCameraMatrix(invCamMat), projectionMatrix(projMat),
		camera_pos(camera_pos) {}

	glm::fmat4 cameraMatrix;
	glm::fmat4 inverseCameraMatrix;
	glm::fmat4 projectionMatrix;
	glm::fvec3 camera_pos;
};
