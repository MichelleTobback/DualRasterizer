#pragma once

namespace dae
{
	struct Vector2
	{
		float x{};
		float y{};

		Vector2() = default;
		Vector2(float _x, float _y);
		Vector2(const Vector2& from, const Vector2& to);

		float Magnitude() const;
		float SqrMagnitude() const;
		float Normalize();
		Vector2 Normalized() const;

		static float Dot(const Vector2& v1, const Vector2& v2);
		static float Cross(const Vector2& v1, const Vector2& v2);
		static Vector2 Lerp(const Vector2& v1, const Vector2& v2, float t);

		//Member Operators
		Vector2 operator*(float scale) const;
		Vector2 operator/(float scale) const;
		Vector2 operator+(const Vector2& v) const;
		Vector2 operator-(const Vector2& v) const;
		Vector2 operator-() const;
		//Vector2& operator-();
		Vector2& operator+=(const Vector2& v);
		Vector2& operator-=(const Vector2& v);
		Vector2& operator/=(float scale);
		Vector2& operator*=(float scale);
		float& operator[](int index);
		float operator[](int index) const;

		inline friend bool operator==(const Vector2& lhs, const Vector2& rhs)
		{
			constexpr float epsilon{ 0.0001f };
			return (std::abs(rhs.x - lhs.x) > epsilon &&
				std::abs(rhs.y - lhs.y) > epsilon);
		}
		inline friend bool operator!=(const Vector2& lhs, const Vector2& rhs) { return !(lhs == rhs); }

		static const Vector2 UnitX;
		static const Vector2 UnitY;
		static const Vector2 Zero;
	};

	//Global Operators
	inline Vector2 operator*(float scale, const Vector2& v)
	{
		return { v.x * scale, v.y * scale };
	}
}
