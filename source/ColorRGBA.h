#pragma once
#include "MathHelpers.h"

namespace dae
{
	struct ColorRGBA
	{
		ColorRGBA() = default;
		ColorRGBA(float r, float g, float b, float a)
			: r{r}, g{g}, b{b}, a{a}{}
		ColorRGBA(const ColorRGB& color)
			: r{ color.r }, g{ color.g }, b{ color.b }, a{1.f} {}

		float r{};
		float g{};
		float b{};
		float a{};

		ColorRGB Rgb()
		{
			return { r, g, b };
		}

		void MaxToOne()
		{
			const float maxValue = std::max(r, std::max(g, std::max(b, a)));
			if (maxValue > 1.f)
				*this /= maxValue;
		}

		static ColorRGBA Lerp(const ColorRGBA& c1, const ColorRGBA& c2, float factor)
		{
			return { Lerpf(c1.r, c2.r, factor), Lerpf(c1.g, c2.g, factor), Lerpf(c1.b, c2.b, factor), Lerpf(c1.a, c2.a, factor) };
		}

#pragma region ColorRGBA (Member) Operators
		const ColorRGBA& operator+=(const ColorRGBA& c)
		{
			r += c.r;
			g += c.g;
			b += c.b;
			a += c.a;

			return *this;
		}

		ColorRGBA operator+(const ColorRGBA& c) const
		{
			return { r + c.r, g + c.g, b + c.b, a + c.a };
		}

		const ColorRGBA& operator-=(const ColorRGBA& c)
		{
			r -= c.r;
			g -= c.g;
			b -= c.b;
			a -= c.a;

			return *this;
		}

		ColorRGBA operator-(const ColorRGBA& c) const
		{
			return { r - c.r, g - c.g, b - c.b, a - c.a };
		}

		const ColorRGBA& operator*=(const ColorRGBA& c)
		{
			r *= c.r;
			g *= c.g;
			b *= c.b;
			a *= c.a;

			return *this;
		}

		ColorRGBA operator*(const ColorRGBA& c) const
		{
			return { r * c.r, g * c.g, b * c.b, a * c.a };
		}

		const ColorRGBA& operator/=(const ColorRGBA& c)
		{
			r /= c.r;
			g /= c.g;
			b /= c.b;
			a /= c.a;

			return *this;
		}

		const ColorRGBA& operator*=(float s)
		{
			r *= s;
			g *= s;
			b *= s;
			a *= s;

			return *this;
		}

		ColorRGBA operator*(float s) const
		{
			return { r * s, g * s,b * s, a * s };
		}

		const ColorRGBA& operator/=(float s)
		{
			r /= s;
			g /= s;
			b /= s;
			a /= s;

			return *this;
		}

		ColorRGBA operator/(float s) const
		{
			return { r / s, g / s,b / s, a / s };
		}
#pragma endregion
	};

	//ColorRGB (Global) Operators
	inline ColorRGBA operator*(float s, const ColorRGBA& c)
	{
		return c * s;
	}

	namespace colors
	{
		static ColorRGBA A_Red{ 1,0,0,1 };
		static ColorRGBA A_Blue{ 0,0,1,1 };
		static ColorRGBA A_Green{ 0,1,0,1 };
		static ColorRGBA A_Yellow{ 1,1,0,1 };
		static ColorRGBA A_Cyan{ 0,1,1,1 };
		static ColorRGBA A_Magenta{ 1,0,1,1 };
		static ColorRGBA A_White{ 1,1,1,1 };
		static ColorRGBA A_Black{ 0,0,0,1 };
		static ColorRGBA A_Gray{ 0.5f,0.5f,0.5f,1 };
	}
}