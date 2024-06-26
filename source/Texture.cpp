#include "pch.h"
#include "Texture.h"

namespace dae
{
	//=======================//
	// software
	//=======================//

	TextureSoftware::TextureSoftware(SDL_Surface* pSurface)
		: m_pSurface{ pSurface }
		, m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{

	}

	TextureSoftware::~TextureSoftware()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	TextureSoftware* TextureSoftware::LoadFromFile(const std::string& path)
	{
		return new TextureSoftware(IMG_Load(path.c_str()));
	}

	ColorRGB TextureSoftware::Sample(const Vector2& uv) const
	{
		Uint8 r{}, g{}, b{};

		Vector2 tilesUv{ uv };

		//tiling
		//clamp to edge
		if (tilesUv.x < 0.f)
			tilesUv.x = 0.f;
		else if (tilesUv.x > 1.f)
			tilesUv.x = 1.f;
		if (tilesUv.y < 0.f)
			tilesUv.y = 0.f;
		else if (tilesUv.y > 1.f)
			tilesUv.y = 1.f;

		SDL_assert(m_pSurface && "m_pSurface is nullptr!");

		//sample
		Uint32 u{ Uint32(tilesUv.x * m_pSurface->w) };
		Uint32 v{ Uint32(tilesUv.y * m_pSurface->h) };
		Uint32 pixel{ m_pSurfacePixels[u + (v * m_pSurface->w)] };

		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		constexpr const float colorDivider{ 1.f / 255.f };
		return { r * colorDivider, g * colorDivider, b * colorDivider };
	}

	ColorRGBA TextureSoftware::SampleRGBA(const Vector2& uv) const
	{
		Uint8 r{}, g{}, b{}, a{};
		Vector2 tilesUv{ uv };
		//tiling
		//clamp to edge
		if (tilesUv.x < 0.f)
			tilesUv.x = 0.f;
		else if (tilesUv.x > 1.f)
			tilesUv.x = 1.f;
		if (tilesUv.y < 0.f)
			tilesUv.y = 0.f;
		else if (tilesUv.y > 1.f)
			tilesUv.y = 1.f;

		//sample

		Uint32 u{ Uint32(tilesUv.x * m_pSurface->w) };
		Uint32 v{ Uint32(tilesUv.y * m_pSurface->h) };
		Uint32 pixel{ m_pSurfacePixels[u + (v * m_pSurface->w)] };

		SDL_GetRGBA(pixel, m_pSurface->format, &r, &g, &b, &a);

		constexpr const float divider{ 1.f / 255.f };

		return { r * divider, g * divider, b * divider, a * divider };
	}

	//=======================//
	// hardware
	//=======================//

	TextureDX11::TextureDX11(SDL_Surface* pSurface, ID3D11Device* pDevice)
	{
		Init(pSurface, pDevice);
		//SDL_FreeSurface(pSurface);
	}

	TextureDX11::~TextureDX11()
	{
		m_pSRV->Release();
		m_pResource->Release();
	}

	TextureDX11* TextureDX11::LoadFromFile(const std::string& path, ID3D11Device* pDevice)
	{
		return new TextureDX11(IMG_Load(path.c_str()), pDevice);
	}

	void TextureDX11::Init(SDL_Surface* pSurface, ID3D11Device* pDevice)
	{
		//=============================================================//
		//				1. Create texture resource					   //
		//=============================================================//
		DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
		D3D11_TEXTURE2D_DESC desc{};

		desc.Width = pSurface->w;
		desc.Height = pSurface->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = pSurface->pixels;
		initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
		initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

		HRESULT result{ pDevice->CreateTexture2D(&desc, &initData, &m_pResource) };
		if (FAILED(result))
			std::wcout << L"Creation of resource failed!\n";

		//=============================================================//
		//				2. Create resource view						   //
		//=============================================================//
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		result = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pSRV);
	}
}