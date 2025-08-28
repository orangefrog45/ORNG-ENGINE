#include "pch/pch.h"
#include "util/Interpolators.h"
#include "util/util.h"

namespace ORNG {

	glm::vec3 InterpolatorV3::GetValue(float x) {
		if (points.empty())
			return { 0, 0, 0 };

		if (x < points[0].x) {
			return { points[0].y, points[0].z, points[0].w };
		}

		for (size_t i = 0; i < points.size(); i++) {
			if (points[i].x > x) {
				unsigned i_0 = static_cast<unsigned>(glm::max(static_cast<int>(i) - 1, 0));

				glm::vec3 p1 = { points[i_0].y, points[i_0].z, points[i_0].w };
				glm::vec3 p2 = { points[i].y, points[i].z, points[i].w };
				return glm::mix(
					p1,
					p2,
					(x - points[i_0].x) / (points[i].x - points[i_0].x)
				);
			}
		}

		return { points[points.size() - 1].y, points[points.size() - 1].z, points[points.size() - 1].w };
	}


	void InterpolatorV3::AddPoint(float x, glm::vec3 v) {
		if (points.size() >= GPU_INTERPOLATOR_STRUCT_MAX_POINTS)
			return;

		if (x < x_min_max.x || x > x_min_max.y) {
			ORNG_CORE_ERROR("InterpolatorV3 error adding point, x out of range, using bound value instead");
		}

		for (int i = 0; i < 3; i++) {
			if (v[i] < yzw_min_max.x || v[i] > yzw_min_max.y) {
				ORNG_CORE_ERROR("InterpolatorV3 error adding point, value out of range, using bound value instead");
			}
		}

		points.push_back({ glm::clamp(x, x_min_max.x, x_min_max.y), glm::clamp(v, glm::vec3(yzw_min_max.x), glm::vec3(yzw_min_max.y)) });
		SortPoints();
	}

	void InterpolatorV3::RemovePoint(size_t index) {
		if (points.size() == 2) // Must have minimum of 2 points to interpolate between
			return;

		points.erase(points.begin() + static_cast<int>(index));
	}

	void InterpolatorV3::SetPoint(size_t index, const glm::vec4& v) {
		ASSERT(points.size() > index);

		if (v.x < x_min_max.x || v.x > x_min_max.y) {
			ORNG_CORE_ERROR("InterpolatorV3 error setting point, x out of range, using bound value instead");
		}

		for (int i = 1; i < 4; i++) {
			if (v[i] < yzw_min_max.x || v[i] > yzw_min_max.y) {
				ORNG_CORE_ERROR("InterpolatorV3 error setting point, value out of range, using bound value instead");
			}
		}

		points[index] = { glm::clamp(v.x, x_min_max.x, x_min_max.y), glm::clamp({v.y, v.z, v.w}, glm::vec3(yzw_min_max.x), glm::vec3(yzw_min_max.y)) };
	}

	void InterpolatorV1::SetPoint(size_t index, glm::vec2 v) {
		ASSERT(points.size() > index);

		if (v.x < x_min_max.x || v.x > x_min_max.y) {
			ORNG_CORE_ERROR("InterpolatorV1 error setting point, x out of range, using bound value instead");
		}

		if (v.y < y_min_max.x || v.y > y_min_max.y) {
			ORNG_CORE_ERROR("InterpolatorV1 error setting point, value out of range, using bound value instead");
		}

		points[index] = { glm::clamp(v.x, x_min_max.x, x_min_max.y), glm::clamp(v.y, y_min_max.x, y_min_max.y) };
	}


	glm::vec4 InterpolatorV3::GetPoint(size_t index) {
		ASSERT(points.size() > index);
		return points[index];
	}

	void InterpolatorV1::ConvertSelfToBytes(std::byte*& p_byte) {
		constexpr float s = std::numeric_limits<float>::lowest();

		for (size_t i = 0; i < points.size(); i++) {
			ConvertToBytes(p_byte, points[i]);
		}

		for (size_t i = points.size(); i < GPU_INTERPOLATOR_STRUCT_MAX_POINTS; i++) {
			ConvertToBytes(p_byte, s); //padding
			ConvertToBytes(p_byte, s); //padding
		}

		ConvertToBytes(p_byte, static_cast<unsigned>(points.size()), scale);
	}

	void InterpolatorV3::ConvertSelfToBytes(std::byte*& p_byte) {
		constexpr float s = std::numeric_limits<float>::lowest();

		for (size_t i = 0; i < points.size(); i++) {
			ConvertToBytes(p_byte, points[i]);
		}

		for (size_t i = points.size(); i < GPU_INTERPOLATOR_STRUCT_MAX_POINTS; i++) {
			ConvertToBytes(p_byte, s, s, s, s); // padding
		}

		ConvertToBytes(p_byte, static_cast<unsigned>(points.size()), scale);
	}


	float InterpolatorV1::GetValue(float x) {
		if (points.empty())
			return 0.f;

		if (x < points[0].x) {
			return points[0].y;
		}

		for (size_t i = 0; i < points.size(); i++) {
			if (points[i].x > x) {
				unsigned i_0 = static_cast<unsigned>(glm::max(static_cast<int>(i) - 1, 0));

				return glm::mix(
					points[i_0].y,
					points[i].y,
					(x - points[i_0].x) / (points[i].x - points[i_0].x)
				);
			}
		}

		return points[points.size() - 1].y;
	}

	void InterpolatorV1::AddPoint(float x, float v) {
		if (points.size() >= GPU_INTERPOLATOR_STRUCT_MAX_POINTS)
			return;

		if (x < x_min_max.x || x > x_min_max.y) {
			ORNG_CORE_ERROR("InterpolatorV1 error adding point, x out of range, using bound value instead");
		}

		for (int i = 0; i < 3; i++) {
			if (v < y_min_max.x || v > y_min_max.y) {
				ORNG_CORE_ERROR("InterpolatorV1 error adding point, value out of range, using bound value instead");
			}
		}

		points.push_back({ glm::clamp(x, x_min_max.x, x_min_max.y), glm::clamp(v, y_min_max.x, y_min_max.y) });
		SortPoints();
	}

	glm::vec2 InterpolatorV1::GetPoint(size_t index) {
		ASSERT(points.size() > index);
		return points[index];
	}

	void InterpolatorV1::RemovePoint(size_t index) {
		if (points.size() == 2) // Must have minimum of 2 points to interpolate between
			return;

		points.erase(points.begin() + static_cast<int>(index));
	}
}
