#pragma once
#include "DataTypes.h"

namespace dae
{
	struct Material;
	class TextureSoftware;
	class TextureDX11;

	typedef std::pair<std::unique_ptr<TextureSoftware>, std::unique_ptr<TextureDX11>> Texture;

	struct Texture_Hash
	{
		std::size_t operator () (Texture& stateAction) const
		{
			auto hash1 = std::hash<std::unique_ptr<TextureSoftware>>{}(stateAction.first);
			auto hash2 = std::hash<std::unique_ptr<TextureDX11>>{}(stateAction.second);

			return hash1 ^ hash2;
		}
	};

	class ResourceManager
	{
	public:

		static inline MaterialID AddMaterial(const Material& material) 
		{ s_Materials.push_back(material); return static_cast<MaterialID>(s_Materials.size() - 1); }
		static inline Material& GetMaterial(size_t materialIdx) { return s_Materials[materialIdx]; }
		
		static TextureID AddTexture(const std::string& filepath);
		static inline TextureSoftware& GetTexture(TextureID texId) { return *s_Textures[texId].first.get(); }
		static inline TextureDX11& GetTextureDX11(TextureID texId) { return *(s_Textures[texId].second.get()); }

	private:
		static std::vector<Material> s_Materials;
		static std::vector<Texture> s_Textures;
	};
}
