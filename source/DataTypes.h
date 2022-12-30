#pragma once
#include "Math.h"

#include <vector>

namespace dae
{
	struct Light
	{
		Vector3 direction{};
		float intensity{ 6.f };
	};

	struct Vertex
	{
		Vector3 position{};
		//ColorRGB color{ colors::White };
		Vector2 uv{}; 
		Vector3 normal{}; 
		Vector3 tangent{}; 
	};

	struct Vertex_Out
	{
		Vector4 position{};
		//ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
		//Vector4 worldPos{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	typedef uint32_t TextureID;
	typedef uint32_t MaterialID;
	typedef uint32_t ShaderID;

	class Effect;
	struct Material
	{
		ShaderID shaderId; //effect for dx11

		std::vector<TextureID> textures;
	};

	struct Mesh
	{
		virtual ~Mesh() = default;

		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleList };

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};

		MaterialID materialId{};

		inline void RotateY(float angle)
		{
			worldMatrix = Matrix::CreateRotationY(angle * TO_RADIANS) * worldMatrix;
		}
	};

	class Effect;
	struct MeshDX11 : public Mesh
	{
	public:
		MeshDX11(ID3D11Device* pDevice, MaterialID materialId);
		virtual ~MeshDX11() override;

		void Init(ID3D11Device* pDevice);

		static MeshDX11* CreateFromFile(ID3D11Device* pDevice, const std::string& filename, MaterialID materialId);

	private:
		uint32_t m_NumIndices{};

		ID3D11Buffer* m_pVertexBuffer{ nullptr };
		ID3D11Buffer* m_pIndexBuffer{ nullptr };

		friend class HardwareRasterizerDX11;
	};
}