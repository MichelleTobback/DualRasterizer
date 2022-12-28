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
		~TextureSoftware();

		static TextureSoftware* LoadFromFile(const std::string& path);
		ColorRGB Sample(const Vector2& uv) const;

	private:
		TextureSoftware(SDL_Surface* pSurface);

		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };
	};

	//=======================//
	// hardware
	//=======================//

	class TextureDX11 final
	{
	public:
		~TextureDX11();

		static TextureDX11* LoadFromFile(const std::string& path, ID3D11Device* pDevice);

		inline ID3D11ShaderResourceView* GetSRV() const { return m_pSRV; }

	private:
		TextureDX11(SDL_Surface* pSurface, ID3D11Device* pDevice);

		void Init(SDL_Surface* pSurface, ID3D11Device* pDevice);

		ID3D11Texture2D* m_pResource{ nullptr };
		ID3D11ShaderResourceView* m_pSRV{ nullptr };
	};
}

