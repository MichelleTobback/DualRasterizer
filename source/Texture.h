#pragma once
#include <SDL_surface.h>
#include <string>
#include "ColorRGB.h"

namespace dae
{
	struct Vector2;

	//=======================//
	// software
	//=======================//

	class TextureSoftware
	{
	public:
		TextureSoftware(SDL_Surface* pSurface);
		~TextureSoftware();

		TextureSoftware(TextureSoftware&& other) noexcept
			: m_pSurface(std::move(other.m_pSurface))
			, m_pSurfacePixels{ std::move(other.m_pSurfacePixels) }
		{
		}
		TextureSoftware& operator=(TextureSoftware&& other)
		{
			m_pSurface = std::move(other.m_pSurface);
			m_pSurfacePixels = std::move(other.m_pSurfacePixels);
			return *this;
		}

		static TextureSoftware* LoadFromFile(const std::string& path);
		ColorRGB Sample(const Vector2& uv) const;

	private:

		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };
	};

	//=======================//
	// hardware
	//=======================//

	class TextureDX11 final
	{
	public:
		TextureDX11(SDL_Surface* pSurface, ID3D11Device* pDevice);
		~TextureDX11();

		TextureDX11(TextureDX11&& other) 
			: m_pResource(std::move(other.m_pResource))
			, m_pSRV{ std::move(other.m_pSRV) }
		{
		}
		TextureDX11& operator=(TextureDX11&& other)
		{
			m_pResource = std::move(other.m_pResource);
			m_pSRV = std::move(other.m_pSRV);
			return *this;
		}

		static TextureDX11* LoadFromFile(const std::string& path, ID3D11Device* pDevice);

		inline ID3D11ShaderResourceView* GetSRV() const { return m_pSRV; }

	private:

		void Init(SDL_Surface* pSurface, ID3D11Device* pDevice);

		ID3D11Texture2D* m_pResource{ nullptr };
		ID3D11ShaderResourceView* m_pSRV{ nullptr };
	};
}

