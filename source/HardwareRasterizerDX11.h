#pragma once
#include "Renderer.h"
#include "DataTypes.h"
#include <unordered_map>

namespace dae
{
	class Effect;

	class HardwareRasterizerDX11 : public Renderer
	{
	public:
		enum class FilterMode
		{
			Point = 0, Linear = 1, Anisotropic = 2, End = 3
		};

		HardwareRasterizerDX11(SDL_Window* pWindow);
		virtual ~HardwareRasterizerDX11() override;

		HardwareRasterizerDX11(const HardwareRasterizerDX11&) = delete;
		HardwareRasterizerDX11(HardwareRasterizerDX11&&) noexcept = delete;
		HardwareRasterizerDX11& operator=(const HardwareRasterizerDX11&) = delete;
		HardwareRasterizerDX11& operator=(HardwareRasterizerDX11&&) noexcept = delete;

		virtual void Update(const Timer* pTimer) override;
		virtual void Render(Scene* pScene) const override;
		virtual void KeyDownEvent(SDL_KeyboardEvent e) override;

		static ShaderID AddEffect(Effect* pEffect);
		static ID3D11Device* GetDevice() { return s_pDevice; }

	protected:
		virtual void RenderMesh(Mesh* pMesh, const Camera& pCamera) const override;

	private:
		//DIRECTX
		HRESULT InitializeDirectX();
		void ReleaseDirectXResources();

		HRESULT CreateRasterizerStates();

		void CycleFilterMode();

		static ID3D11Device* s_pDevice;
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
		ID3D11Texture2D* m_pDepthStencilBuffer{ nullptr };
		ID3D11DepthStencilView* m_pDepthStencilView{ nullptr };
		ID3D11Texture2D* m_pRenderTargetBuffer{ nullptr };
		ID3D11RenderTargetView* m_pRenderTargetView{ nullptr };
		std::unordered_map<Renderer::FaceCullingMode, ID3D11RasterizerState*> m_pRasterizerStates;

		FilterMode m_FilterMode{ FilterMode::Point };

		static std::vector<std::unique_ptr<Effect>> s_pEffects;
	};
}
