#pragma once

namespace ORNG {
	class InterpolatorV1 {
		friend class ExtraUI;
		friend class SceneSerializer;
		friend struct InterpolatorSerializer;

	public:
		InterpolatorV1(lml::vec2 t_min_max_x, lml::vec2 t_min_max_y, float p1_val, float p2_val) : x_min_max(t_min_max_x), y_min_max(t_min_max_y) {
			AddPoint(t_min_max_x.x, p1_val);
			AddPoint(t_min_max_x.y, p2_val);
		}

		float GetValue(float x);

		void AddPoint(float x, float y);

		void SetPoint(size_t index, lml::vec2 v);

		void RemovePoint(size_t index);

		[[nodiscard]] unsigned GetNbPoints() const noexcept { return static_cast<unsigned>(points.size()); }

		lml::vec2 GetPoint(size_t index);

		void SortPoints() {
			std::ranges::sort(points, [](const lml::vec2& point_left, const lml::vec2& point_right) {return point_left.x < point_right.x; });
		}

		void ConvertSelfToBytes(std::byte*& p_byte);

		float scale = 1.f;

		static constexpr unsigned GPU_STRUCT_SIZE_BYTES = sizeof(lml::vec4) * 4 + sizeof(unsigned); // 8 points, active_points uint
		static constexpr unsigned GPU_INTERPOLATOR_STRUCT_MAX_POINTS = 8;

	private:
		std::vector<lml::vec2> points;

		const lml::vec2 x_min_max = { 0, 1 };
		const lml::vec2 y_min_max = { 0, 1 };
	};

	class InterpolatorV3 {
		friend class ExtraUI;
		friend class SceneSerializer;
		friend struct InterpolatorSerializer;
	public:
		InterpolatorV3(lml::vec2 t_min_max_x, lml::vec2 t_min_max_yzw, lml::vec3 p1_val, lml::vec3 p2_val) : x_min_max(t_min_max_x), yzw_min_max(t_min_max_yzw) {
			AddPoint(x_min_max.x, p1_val);
			AddPoint(x_min_max.y, p2_val);
		}

		lml::vec3 GetValue(float x);

		void AddPoint(float x, lml::vec3 v);

		void SetPoint(size_t index, const lml::vec4& v);

		void RemovePoint(size_t index);

		[[nodiscard]] unsigned GetNbPoints() const noexcept { return static_cast<unsigned>(points.size()); }

		lml::vec4 GetPoint(size_t index);

		void SortPoints() {
			std::ranges::sort(points, [](const lml::vec4& point_left, const lml::vec4& point_right) {return point_left.x < point_right.x; });
		}

		void ConvertSelfToBytes(std::byte*& p_byte);

		float scale = 1.f;

		inline static const size_t GPU_STRUCT_SIZE_BYTES = sizeof(lml::vec4) * 8 + sizeof(unsigned);
		inline static const size_t GPU_INTERPOLATOR_STRUCT_MAX_POINTS = 8;
	private:

		const lml::vec2 x_min_max = { 0, 1 };
		const lml::vec2 yzw_min_max = { 0, 1 };
		std::vector<lml::vec4> points;
	};
}
