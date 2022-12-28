#pragma once
#include "Renderer.h"
#include "DataTypes.h"

namespace dae
{
	class Effect;

	class HardwareRasterizerDX11 : public Renderer
	{
	public:
		enum class FilterMode
		{
			Point, Linear, Anisotropic
		};

		HardwareRasterizerDX11(SDL_Window* pWindow);
		virtual ~HardwareRasterizerDX11() override;

		HardwareRasterizerDX11(const HardwareRasterizerDX11&) = delete;
		HardwareRasterizerDX11(HardwareRasterizerDX11&&) noexcept = delete;
		HardwareRasterizerDX11& operator=(const HardwareRasterizerDX11&) = delete;
		HardwareRasterizerDX11& operator=(HardwareRasterizerDX11&&) noexcept = delete;

		virtual void Update(const Timer* pTimer) override;
		virtual void Render(Scene* pScene) const override;

		static ShaderID AddEffect(Effect* pEffect);
		static ID3D11Device* GetDevice() { return s_pDevice; }

	protected:
		virtual void RenderMesh(Mesh* pMesh, const Camera& pCamera) const override;

	private:
		//DIRECTX
		HRESULT InitializeDirectX();
		void ReleaseDirectXResources();

		static ID3D11Device* s_pDevice;
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
		ID3D11Texture2D* m_pDepthStencilBuffer{ nullptr };
		ID3D11DepthStencilView* m_pDepthStencilView{ nullptr };
		ID3D11Texture2D* m_pRenderTargetBuffer{ nullptr };
		ID3D11RenderTargetView* m_pRenderTargetView{ nullptr };

		FilterMode m_FilterMode{ FilterMode::Point };

		static std::vector<std::unique_ptr<Effect>> s_pEffects;
	};
}
