#include "pch.h"
#include "HardwareRasterizerDX11.h"
#include "Scene.h"
#include "Camera.h"
#include "Effect.h"
#include "ResourceManager.h"
#include "ConsoleLog.h"

namespace dae
{
	using namespace Log;

	ID3D11Device* HardwareRasterizerDX11::s_pDevice{nullptr};
	std::vector<std::unique_ptr<Effect>> HardwareRasterizerDX11::s_pEffects{};

	HardwareRasterizerDX11::HardwareRasterizerDX11(SDL_Window* pWindow)
		: Renderer(pWindow)
	{
		m_ClearColor = ColorRGB{ 0.39f, 0.59f, 0.93f };

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			TSTRING msg{ _T("\nDirectX is initialized and ready!\n") };
			PrintMessage(msg, MSG_LOGGER_HARDWARERASTERIZER, MSG_COLOR_HARDWARERASTERIZER, MSG_COLOR_SUCCESS);
		}
		else
		{
			TSTRING msg{ _T("\nDirectX initialization failed!\n") };
			PrintMessage(msg, MSG_LOGGER_HARDWARERASTERIZER, MSG_COLOR_HARDWARERASTERIZER, MSG_COLOR_WARNING);
		}
	}

	HardwareRasterizerDX11::~HardwareRasterizerDX11()
	{
		ReleaseDirectXResources();
	}

	void HardwareRasterizerDX11::Update(const Timer* pTimer)
	{

	}

	void HardwareRasterizerDX11::Render(Scene* pScene) const
	{
		if (!m_IsInitialized)
			return;

		//=============================================================//
		//					1. Clear RTV & DSV			               //
		//=============================================================//

		const ColorRGB& clearColor{ GetClearColor() };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		//=============================================================//
		//		  2. Set Pipeline	+ Invoke Drawcalls (= render)      //
		//=============================================================//

		for (const auto& pMesh : pScene->m_pMeshes)
		{
			if (!pMesh->render)
				continue;

			RenderMesh(pMesh.get(), pScene->GetCamera());
		}

		//=============================================================//
		//					3. Present Backbuffer (Swap)			   //
		//=============================================================//

		m_pSwapChain->Present(0, 0);
	}

	void HardwareRasterizerDX11::KeyDownEvent(SDL_KeyboardEvent e)
	{
		Renderer::KeyDownEvent(e);

		switch (e.keysym.scancode)
		{
		case SDL_SCANCODE_F4:
		{
			CycleFilterMode();
		}
		break;
		}
	}

	ShaderID HardwareRasterizerDX11::AddEffect(Effect* pEffect)
	{
		std::unique_ptr<Effect> effect{ pEffect };
		s_pEffects.push_back(std::move(effect));
		s_pEffects.back().get()->CreateLayout(s_pDevice);

		return static_cast<ShaderID>(s_pEffects.size() - 1);
	}

	void HardwareRasterizerDX11::RenderMesh(Mesh* pMesh, const Camera& camera) const
	{
		auto pMeshdx11{ dynamic_cast<MeshDX11*>(pMesh) };

		if (!pMeshdx11)
			return;

		Matrix worldViewProjMat{ pMeshdx11->worldMatrix * camera.viewMatrix * camera.ProjectionMatrix };

		auto& material{ ResourceManager::GetMaterial(pMesh->materialId) };
		auto& pActiveEffect{ s_pEffects[material.shaderId] };

		pActiveEffect->SetWorldViewProjMatrix(worldViewProjMat);

		PosTexEffect* pEffect{ dynamic_cast<PosTexEffect*>(pActiveEffect.get()) };
		if (pEffect)
		{
			assert(material.textures.size() == 4 && "material has not enough textures for current shading!\n");

			pEffect->SetTextureMap(&ResourceManager::GetTextureDX11(material.textures[0]), "gDiffuseMap");
			pEffect->SetTextureMap(&ResourceManager::GetTextureDX11(material.textures[1]), "gNormalMap");
			pEffect->SetTextureMap(&ResourceManager::GetTextureDX11(material.textures[2]), "gSpecularMap");
			pEffect->SetTextureMap(&ResourceManager::GetTextureDX11(material.textures[3]), "gGlossinessMap");

			Matrix worldMat{ pMeshdx11->worldMatrix };
			pEffect->SetWorldMatrix(worldMat);
			Matrix onbMat{ camera.invViewMatrix };
			pEffect->SetONBMatrix(onbMat);
			pEffect->SetRasterizerState(m_pRasterizerStates.at(s_Settings.faceCullingMode));
		}
		else
		{
			FlatEffect* pFlatEffect{ dynamic_cast<FlatEffect*>(pActiveEffect.get()) };

			if (pFlatEffect)
			{
				assert(material.textures.size() == 1 && "material has not enough textures for current shading!\n");
				pFlatEffect->SetTextureMap(&ResourceManager::GetTextureDX11(material.textures[0]));
			}
		}
		//=============================================================//
		//					1. Set Primite Topology		               //
		//=============================================================//
		m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//=============================================================//
		//					  2. SetInput Layout					   //
		//=============================================================//
		m_pDeviceContext->IASetInputLayout(pActiveEffect->GetInputLayout());

		//=============================================================//
		//					 3. Set VertexBuffer			           //
		//=============================================================//
		constexpr UINT stride{ sizeof(Vertex) };
		constexpr UINT offset{ 0 };
		m_pDeviceContext->IASetVertexBuffers(0, 1, &pMeshdx11->m_pVertexBuffer, &stride, &offset);

		//=============================================================//
		//					  4. Set IndexBuffer			           //
		//=============================================================//
		m_pDeviceContext->IASetIndexBuffer(pMeshdx11->m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		//=============================================================//
		//							5. Draw							   //
		//=============================================================//

		size_t techniqueIdx{ static_cast<size_t>(m_FilterMode) };
		D3DX11_TECHNIQUE_DESC techDesc{};
		pActiveEffect->GetTechniqueByIndex(techniqueIdx)->GetDesc(&techDesc);
		for (UINT p{}; p < techDesc.Passes; ++p)
		{
			pActiveEffect->GetTechniqueByIndex(techniqueIdx)->GetPassByIndex(p)->Apply(0, m_pDeviceContext);
			m_pDeviceContext->DrawIndexed(pMeshdx11->m_NumIndices, 0, 0);
		}
	}

	HRESULT HardwareRasterizerDX11::InitializeDirectX()
	{
		//=============================================================//
		//				1. Create Device & DeviceContext			   //
		//=============================================================//
		D3D_FEATURE_LEVEL featureLevel{ D3D_FEATURE_LEVEL_11_1 };
		uint32_t createDeviceFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		HRESULT result{ D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
			1, D3D11_SDK_VERSION, &s_pDevice, nullptr, &m_pDeviceContext) };

		if (FAILED(result))
			return result;

		//Create DXGI Factory//
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));

		if (FAILED(result))
			return result;

		//=============================================================//
		//					2. Create SwapChain						   //
		//=============================================================//
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		//Get the handle (HWND) from the SDL Backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_VERSION(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		//Create SwapChain
		result = pDxgiFactory->CreateSwapChain(s_pDevice, &swapChainDesc, &m_pSwapChain);

		if (FAILED(result))
			return result;

		//=============================================================//
		//3. Create DepthStencil (DS) & DepthStencilView (DSV) Resource//
		//=============================================================//
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		//view
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = s_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);

		if (FAILED(result))
			return result;

		result = s_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);

		if (FAILED(result))
			return result;

		//=============================================================//
		//	   4. Create RenderTarget (RT) & RenderTargetView (RTV)    //
		//=============================================================//

		//Resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));

		if (FAILED(result))
			return result;

		//view
		result = s_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);

		if (FAILED(result))
			return result;

		//=============================================================//
		//			5. Bind RTV & DSV to Output Merger Stage           //
		//=============================================================//
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		//=============================================================//
		//						6. Set Viewport                        //
		//=============================================================//
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<FLOAT>(m_Width);
		viewport.Height = static_cast<FLOAT>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		pDxgiFactory->Release();

		result = CreateRasterizerStates();
		if (FAILED(result))
			return result;

		return S_OK;
	}

	void HardwareRasterizerDX11::ReleaseDirectXResources()
	{
		for (auto& pRasterizerState : m_pRasterizerStates)
		{
			pRasterizerState.second->Release();
		}
		m_pRasterizerStates.clear();

		m_pRenderTargetView->Release();
		m_pRenderTargetBuffer->Release();
		m_pDepthStencilView->Release();
		m_pDepthStencilBuffer->Release();
		m_pSwapChain->Release();
		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}
		s_pDevice->Release();
	}
	HRESULT HardwareRasterizerDX11::CreateRasterizerStates()
	{
		HRESULT result{};

		//=============================================================//
		//						1. Backface                            //
		//=============================================================//
		m_pRasterizerStates[Renderer::FaceCullingMode::Backface] = nullptr;

		D3D11_RASTERIZER_DESC rasterizerStateDesc;
		rasterizerStateDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerStateDesc.CullMode = D3D11_CULL_BACK;
		rasterizerStateDesc.FrontCounterClockwise = false;
		rasterizerStateDesc.DepthBias = false;
		rasterizerStateDesc.DepthBiasClamp = 0;
		rasterizerStateDesc.SlopeScaledDepthBias = 0;
		rasterizerStateDesc.DepthClipEnable = true;
		rasterizerStateDesc.ScissorEnable = false;
		rasterizerStateDesc.MultisampleEnable = false;
		rasterizerStateDesc.AntialiasedLineEnable = false;

		result = s_pDevice->CreateRasterizerState(
			&rasterizerStateDesc, 
			&m_pRasterizerStates[Renderer::FaceCullingMode::Backface]);

		if (FAILED(result))
			return result;

		//=============================================================//
		//						2. Frontface                           //
		//=============================================================//

		m_pRasterizerStates[Renderer::FaceCullingMode::Frontface] = nullptr;

		rasterizerStateDesc.CullMode = D3D11_CULL_FRONT;

		result = s_pDevice->CreateRasterizerState(
			&rasterizerStateDesc,
			&m_pRasterizerStates[Renderer::FaceCullingMode::Frontface]);

		if (FAILED(result))
			return result;

		//=============================================================//
		//						2. No culling                          //
		//=============================================================//

		m_pRasterizerStates[Renderer::FaceCullingMode::None] = nullptr;

		rasterizerStateDesc.CullMode = D3D11_CULL_NONE;

		result = s_pDevice->CreateRasterizerState(
			&rasterizerStateDesc,
			&m_pRasterizerStates[Renderer::FaceCullingMode::None]);

		if (FAILED(result))
			return result;

		return S_OK;
	}
	void HardwareRasterizerDX11::CycleFilterMode()
	{
		size_t filterMode{ static_cast<size_t>(m_FilterMode) };
		if (++filterMode == static_cast<size_t>(FilterMode::End))
		{
			filterMode = 0;
		}
		m_FilterMode = static_cast<FilterMode>(filterMode);

		TSTRING msg{ _T("Filter mode : ") };
		switch (m_FilterMode)
		{
		case FilterMode::Point:
			msg.append(_T("Point"));
			break;

		case FilterMode::Linear:
			msg.append(_T("Linear"));
			break;

		case FilterMode::Anisotropic:
			msg.append(_T("Anisotropic"));
			break;
		}
		PrintMessage(msg, MSG_LOGGER_HARDWARERASTERIZER, MSG_COLOR_HARDWARERASTERIZER);
	}
}