#pragma once

namespace ORNG {
	class InterpolatorV1 {
		friend class ExtraUI;
		friend class SceneSerializer;
		friend class InterpolatorSerializer;

	public:
		InterpolatorV1(glm::vec2 t_min_max_x, glm::vec2 t_min_max_y) : x_min_max(t_min_max_x), y_min_max(t_min_max_y) {
			AddPoint(x_min_max.x, y_min_max.x);
			AddPoint(x_min_max.y, y_min_max.y);
		};

		float GetValue(float x);

		void AddPoint(float x, float y);

		void SetPoint(unsigned index, glm::vec2 v);

		void RemovePoint(unsigned index);

		glm::vec2 GetPoint(unsigned index);

		void SortPoints() {
			std::ranges::sort(points, [](const glm::vec2& point_left, const glm::vec2& point_right) {return point_left.x < point_right.x; });
		}

		void ConvertSelfToBytes(std::byte*& p_byte);


		inline static const unsigned GPU_STRUCT_SIZE_BYTES = sizeof(glm::vec4) * 4 + sizeof(unsigned); // 8 points, active_points uint
		inline static const unsigned GPU_INTERPOLATOR_STRUCT_MAX_POINTS = 8;
	private:
		std::vector<glm::vec2> points;

		const glm::vec2 x_min_max = { 0, 1 };
		const glm::vec2 y_min_max = { 0, 1 };
	};

	class InterpolatorV3 {
		friend class ExtraUI;
		friend class SceneSerializer;
		friend class InterpolatorSerializer;
	public:
		InterpolatorV3(glm::vec2 t_min_max_x, glm::vec2 t_min_max_yzw) : x_min_max(t_min_max_x), yzw_min_max(t_min_max_yzw) {
			AddPoint(x_min_max.x, glm::vec3(yzw_min_max.x));
			AddPoint(x_min_max.y, glm::vec3(yzw_min_max.y));
		};

		glm::vec3 GetValue(float x);

		void AddPoint(float x, glm::vec3 v);

		void SetPoint(unsigned index, const glm::vec4& v);

		void RemovePoint(unsigned index);

		glm::vec4 GetPoint(unsigned index);

		void SortPoints() {
			std::ranges::sort(points, [](const glm::vec4& point_left, const glm::vec4& point_right) {return point_left.x < point_right.x; });
		}

		void ConvertSelfToBytes(std::byte*& p_byte);

		inline static const unsigned GPU_STRUCT_SIZE_BYTES = sizeof(glm::vec4) * 8 + sizeof(unsigned);
		inline static const unsigned GPU_INTERPOLATOR_STRUCT_MAX_POINTS = 8;

	private:
		const glm::vec2 x_min_max = { 0, 1 };
		const glm::vec2 yzw_min_max = { 0, 1 };
		std::vector<glm::vec4> points;
	};
}