#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include <memory>
#include "HardwareRasterizerDX11.h"

namespace dae
{
	std::vector<Material> ResourceManager::s_Materials{};
	std::vector<Texture> ResourceManager::s_Textures{};

	TextureID ResourceManager::AddTexture(const std::string& filepath)
	{
		auto pSurface{ IMG_Load(filepath.c_str()) };

		auto textureSoftware{ std::make_unique<TextureSoftware>(pSurface) };
		auto textureDx11{ std::make_unique<TextureDX11>(pSurface, HardwareRasterizerDX11::GetDevice()) };
		auto texture{ std::make_pair(std::move(textureSoftware), std::move(textureDx11)) };

		s_Textures.push_back(std::move(texture));

		return static_cast<TextureID>(s_Textures.size() - 1);
	}
}