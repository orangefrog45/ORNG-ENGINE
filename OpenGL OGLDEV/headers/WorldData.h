#pragma once
#include <glm/glm.hpp>
struct WorldData {
	WorldData(const glm::fmat4& camMat, const glm::fmat4& invCamMat, const glm::fmat4& projMat) : cameraMatrix(camMat), inverseCameraMatrix(invCamMat), projectionMatrix(projMat) {}
	glm::fmat4 cameraMatrix;
	glm::fmat4 inverseCameraMatrix;
	glm::fmat4 projectionMatrix;
};
