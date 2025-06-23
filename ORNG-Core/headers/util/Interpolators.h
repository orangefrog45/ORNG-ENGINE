#pragma once

namespace ORNG {
	class InterpolatorV1 {
		friend class ExtraUI;
		friend class SceneSerializer;
		friend class InterpolatorSerializer;

	public:
		InterpolatorV1(glm::vec2 t_min_max_x, glm::vec2 t_min_max_y, float p1_val, float p2_val) : x_min_max(t_min_max_x), y_min_max(t_min_max_y) {
			AddPoint(t_min_max_x.x, p1_val);
			AddPoint(t_min_max_x.y, p2_val);
		};

		float GetValue(float x);

		void AddPoint(float x, float y);

		void SetPoint(unsigned index, glm::vec2 v);

		void RemovePoint(unsigned index);

		[[nodiscard]] unsigned GetNbPoints() const noexcept { return (unsigned)points.size(); };

		glm::vec2 GetPoint(unsigned index);

		void SortPoints() {
			std::ranges::sort(points, [](const glm::vec2& point_left, const glm::vec2& point_right) {return point_left.x < point_right.x; });
		}

		void ConvertSelfToBytes(std::byte*& p_byte);

		float scale = 1.f;

		static constexpr unsigned GPU_STRUCT_SIZE_BYTES = sizeof(glm::vec4) * 4 + sizeof(unsigned); // 8 points, active_points uint
		static constexpr unsigned GPU_INTERPOLATOR_STRUCT_MAX_POINTS = 8;

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
		InterpolatorV3(glm::vec2 t_min_max_x, glm::vec2 t_min_max_yzw, glm::vec3 p1_val, glm::vec3 p2_val) : x_min_max(t_min_max_x), yzw_min_max(t_min_max_yzw) {
			AddPoint(x_min_max.x, p1_val);
			AddPoint(x_min_max.y, p2_val);
		};

		glm::vec3 GetValue(float x);

		void AddPoint(float x, glm::vec3 v);

		void SetPoint(unsigned index, const glm::vec4& v);

		void RemovePoint(unsigned index);

		[[nodiscard]] unsigned GetNbPoints() const noexcept { return (unsigned)points.size(); };

		glm::vec4 GetPoint(unsigned index);

		void SortPoints() {
			std::ranges::sort(points, [](const glm::vec4& point_left, const glm::vec4& point_right) {return point_left.x < point_right.x; });
		}

		void ConvertSelfToBytes(std::byte*& p_byte);

		float scale = 1.f;

		inline static const unsigned GPU_STRUCT_SIZE_BYTES = sizeof(glm::vec4) * 8 + sizeof(unsigned);
		inline static const unsigned GPU_INTERPOLATOR_STRUCT_MAX_POINTS = 8;
	private:

		const glm::vec2 x_min_max = { 0, 1 };
		const glm::vec2 yzw_min_max = { 0, 1 };
		std::vector<glm::vec4> points;
	};
}