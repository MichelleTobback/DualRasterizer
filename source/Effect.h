#pragma once
#include <unordered_map>

namespace dae
{
	class Effect
	{
	public:
		Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~Effect();

		Effect() = delete;
		Effect(const Effect&) = delete;
		Effect(Effect&&) noexcept = delete;
		Effect& operator=(const Effect&) = delete;
		Effect& operator=(Effect&&) noexcept = delete;

		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);

		inline ID3DX11Effect* GetEffect() const { return m_pEffect; }
		inline ID3DX11EffectTechnique* GetTechniqueByIndex(size_t idx = 0) const { return m_pTechniques[idx]; }
		inline ID3D11InputLayout* GetInputLayout() const { return m_pInputLayout; }
		void SetWorldViewProjMatrix(Matrix& worldViewProjMat);

		virtual HRESULT CreateLayout(ID3D11Device* pDevice) = 0;

	protected:
		void ReleaseResources();
		HRESULT CreateInputLayout(ID3D11Device* pDevice, D3D11_INPUT_ELEMENT_DESC* pInputElementDesc, uint32_t numElements);

		ID3DX11Effect* m_pEffect{ nullptr };
		std::vector<ID3DX11EffectTechnique*> m_pTechniques;
		ID3D11InputLayout* m_pInputLayout{ nullptr };

	private:
		ID3DX11EffectMatrixVariable* m_pWorldViewProjMatrixVar{ nullptr };
	};

	class TextureDX11;
	class PosTexEffect : public Effect
	{
	public:
		PosTexEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~PosTexEffect() override;

		PosTexEffect() = delete;
		PosTexEffect(const PosTexEffect&) = delete;
		PosTexEffect(PosTexEffect&&) noexcept = delete;
		PosTexEffect& operator=(const PosTexEffect&) = delete;
		PosTexEffect& operator=(PosTexEffect&&) noexcept = delete;

		virtual HRESULT CreateLayout(ID3D11Device* pDevice) override;
		void SetTextureMap(TextureDX11* pTexture, const char* slot);
		void SetONBMatrix(Matrix& matrix);
		void SetWorldMatrix(Matrix& matrix);
		void SetRasterizerState(ID3D11RasterizerState* pRasterizerState);

	private:
		void CreateTextureVar(const char* varName);

		std::unordered_map<const char*, ID3DX11EffectShaderResourceVariable*> m_pTextureMapVars;

		ID3DX11EffectMatrixVariable* m_pWorldMatrixVar{ nullptr };
		ID3DX11EffectMatrixVariable* m_pONBMatrixVar{ nullptr };
		ID3DX11EffectRasterizerVariable* m_pFaceCullModeVar{ nullptr };
	};

	class FlatEffect : public Effect
	{
	public:
		FlatEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~FlatEffect() override;

		FlatEffect() = delete;
		FlatEffect(const FlatEffect&) = delete;
		FlatEffect(FlatEffect&&) noexcept = delete;
		FlatEffect& operator=(const FlatEffect&) = delete;
		FlatEffect& operator=(FlatEffect&&) noexcept = delete;

		virtual HRESULT CreateLayout(ID3D11Device* pDevice) override;
		void SetTextureMap(TextureDX11* pTexture);

	private:

		ID3DX11EffectShaderResourceVariable* m_pTexture{ nullptr };
	};
}



