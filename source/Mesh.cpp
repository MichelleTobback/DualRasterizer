#include "pch.h"
#include "DataTypes.h"
#include "Effect.h"
#include "DataTypes.h"
#include "Camera.h"
#include "Utils.h"

#include <iostream>

namespace dae
{

	MeshDX11::MeshDX11(ID3D11Device* pDevice, Effect* pEffect)
		: m_pEffect{ pEffect }
	{
		if (!pEffect)
		{
			std::wcout << "Effect is nullptr!\n";
			return;
		}
	}

	MeshDX11::~MeshDX11()
	{
		m_pIndexBuffer->Release();
		m_pVertexBuffer->Release();
	}

	void MeshDX11::Init(ID3D11Device* pDevice)
	{
		//=============================================================//
		//				1. Create Vertex + input Layout				   //
		//=============================================================//
		HRESULT result{ m_pEffect->CreateLayout(pDevice) };

		if (FAILED(result))
			return;

		//=============================================================//
		//					3. Create VertexBuffer		               //
		//=============================================================//
		D3D11_BUFFER_DESC bd{};
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(Vertex) * static_cast<uint32_t>(vertices.size());
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = vertices.data();

		result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);

		if (FAILED(result))
			return;

		//=============================================================//
		//					4. Create IndexBuffer		               //
		//=============================================================//
		m_NumIndices = static_cast<uint32_t>(indices.size());
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		initData.pSysMem = indices.data();

		result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);

		if (FAILED(result))
			return;
	}

	MeshDX11 MeshDX11::CreateFromFile(ID3D11Device* pDevice, const std::string& filename, Effect* pEffect)
	{
		MeshDX11 mesh{ pDevice, pEffect };

		Utils::ParseOBJ(filename, mesh.vertices, mesh.indices);

		mesh.Init(pDevice);

		return mesh;
	}
}