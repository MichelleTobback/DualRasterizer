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
		Vector2 uv{}; 
		Vector3 normal{}; 
		Vector3 tangent{}; 
	};

	struct Vertex_Out
	{
		Vector4 position{};
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	struct Triangle
	{
		Vertex_Out v0{};
		Vertex_Out v1{};
		Vertex_Out v2{};

		const size_t size{ 3 };

		Triangle() = default;
		Triangle(const Triangle& other)
			: v0{other.v0}, v1{other.v1}, v2{other.v2}{}
		Triangle(const Vertex_Out& _v0, const Vertex_Out& _v1, const Vertex_Out& _v2)
			: v0{_v0}, v1{_v1}, v2{_v2}{}
		Triangle(Triangle&& other) = default;
		Triangle operator=(const Triangle& other) { return *this; }
		Triangle& operator=(Triangle&& other) noexcept { return *this; }

		size_t Size() const { return size; }

		Vertex_Out& operator[](size_t index)
		{
			//assert((index <= 2 && index) >= 0 && "index out of range!\n");
			switch (index)
			{
			case 0:
				return v0;
				break;

			case 1:
				return v1;
				break;

			case 2:
				return v2;
				break;
			}
			return v0; // no warning
		}

		const Vertex_Out& operator[](size_t index) const
		{
			//assert((index <= 2 && index) >= 0 && "index out of range!\n");
			switch (index)
			{
			case 0:
				return v0;
				break;

			case 1:
				return v1;
				break;

			case 2:
				return v2;
				break;
			}
			return v0;
		}
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

		bool depthWrite{ true };
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

		bool render{ true };

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