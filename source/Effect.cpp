#include "pch.h"
#include "Effect.h"
#include "Texture.h"

dae::Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
    m_pEffect = LoadEffect(pDevice, assetFile);

    if (m_pEffect == nullptr)
        return;

    m_pTechniques.push_back(m_pEffect->GetTechniqueByName("DefaultTechnique"));
    if (!m_pTechniques[0]->IsValid())
        std::wcout << L"DefaultTechnique not valid\n";

    m_pTechniques.push_back(m_pEffect->GetTechniqueByName("LinearTechnique"));
    if (!m_pTechniques[1]->IsValid())
        std::wcout << L"LinearTechnique not valid\n";

    m_pTechniques.push_back(m_pEffect->GetTechniqueByName("AnisotropicTechnique"));
    if (!m_pTechniques[2]->IsValid())
        std::wcout << L"AnisotropicTechnique not valid\n";

    m_pWorldViewProjMatrixVar = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
    if (!m_pWorldViewProjMatrixVar->IsValid())
        std::wcout << L"m_pWorldViewProjMatrixVar not valid!\n";
}

dae::Effect::~Effect()
{
    ReleaseResources();
}

ID3DX11Effect* dae::Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
    HRESULT result;
    ID3D10Blob* pErrorBlob{ nullptr };
    ID3DX11Effect* pEffect{ nullptr };

    DWORD shaderFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    result = D3DX11CompileEffectFromFile(assetFile.c_str(),
        nullptr,
        nullptr,
        shaderFlags,
        0,
        pDevice,
        &pEffect,
        &pErrorBlob);

    if (FAILED(result))
    {
        if (pErrorBlob != nullptr)
        {
            const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };

            std::wstringstream ss;
            for (unsigned int i{}; i < pErrorBlob->GetBufferSize(); i++)
                ss << pErrors[i];

            OutputDebugStringW(ss.str().c_str());
            pErrorBlob->Release();
            pErrorBlob = nullptr;

            std::wcout << ss.str() << '\n';
        }
        else
        {
            std::wstringstream ss;
            ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile << '\n';
            std::wcout << ss.str();
            return nullptr;
        }
    }

    return pEffect;
}

void dae::Effect::SetWorldViewProjMatrix(Matrix& worldViewProjMat)
{
    const float* data{ reinterpret_cast<float*>(&worldViewProjMat) };
    m_pWorldViewProjMatrixVar->SetMatrix(data);
}

void dae::Effect::ReleaseResources()
{
    for (auto& pTechnique : m_pTechniques)
        pTechnique->Release();

    m_pEffect->Release();
    m_pInputLayout->Release();
}

HRESULT dae::Effect::CreateInputLayout(ID3D11Device* pDevice, D3D11_INPUT_ELEMENT_DESC* pInputElementDesc, uint32_t numElements)
{
    D3DX11_PASS_DESC passDesc{};
    m_pTechniques[0]->GetPassByIndex(0)->GetDesc(&passDesc);

    HRESULT result{ pDevice->CreateInputLayout
        (
            pInputElementDesc,
            numElements,
            passDesc.pIAInputSignature,
            passDesc.IAInputSignatureSize,
            &m_pInputLayout
        ) };

    if (FAILED(result))
    {
        std::wcout << L"Creation of input layout failed! : " << result << "\n";
    }
    return result;
}

dae::PosTexEffect::PosTexEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
    : Effect(pDevice, assetFile)
{
    CreateTextureVar("gDiffuseMap");
    CreateTextureVar("gNormalMap");
    CreateTextureVar("gSpecularMap");
    CreateTextureVar("gGlossinessMap");

    m_pWorldMatrixVar = m_pEffect->GetVariableByName("gWorldMat")->AsMatrix();
    if (!m_pWorldMatrixVar->IsValid())
        std::wcout << L"m_pWorldMatrixVar not valid!\n";

    m_pONBMatrixVar = m_pEffect->GetVariableByName("gONB")->AsMatrix();
    if (!m_pONBMatrixVar->IsValid())
        std::wcout << L"m_pONBMatrixVar not valid!\n";
}

dae::PosTexEffect::~PosTexEffect()
{

}

HRESULT dae::PosTexEffect::CreateLayout(ID3D11Device* pDevice)
{
    //=============================================================//
    //					1. Create Vertex Layout		               //
    //=============================================================//
    static constexpr uint32_t numElements{ 4 };
    D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

    vertexDesc[0].SemanticName = "POSITION";
    vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[0].AlignedByteOffset = 0;
    vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[1].SemanticName = "TEXCOORD";
    vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    vertexDesc[1].AlignedByteOffset = 12;
    vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[2].SemanticName = "NORMAL";
    vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[2].AlignedByteOffset = 20;
    vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[3].SemanticName = "TANGENT";
    vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[3].AlignedByteOffset = 32;
    vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    //=============================================================//
    //					2. Create Input Layout		               //
    //=============================================================//
    return CreateInputLayout(pDevice, vertexDesc, numElements);
}

void dae::PosTexEffect::SetONBMatrix(Matrix& matrix)
{
    const float* data{ reinterpret_cast<float*>(&matrix) };
    m_pONBMatrixVar->SetMatrix(data);
}

void dae::PosTexEffect::SetWorldMatrix(Matrix& matrix)
{
    const float* data{ reinterpret_cast<float*>(&matrix) };
    m_pWorldMatrixVar->SetMatrix(data);
}

void dae::PosTexEffect::CreateTextureVar(const char* varName)
{
    m_pTextureMapVars[varName] = m_pEffect->GetVariableByName(LPCSTR(varName))->AsShaderResource();
    if (!m_pTextureMapVars[varName]->IsValid())
        std::wcout << varName << L" is not valid!\n";
}

void dae::PosTexEffect::SetTextureMap(TextureDX11* pTexture, const char* slot)
{
    if (m_pTextureMapVars[slot] && pTexture)
        m_pTextureMapVars[slot]->SetResource(pTexture->GetSRV());
}

dae::FlatEffect::FlatEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
    : Effect(pDevice, assetFile)
{
    m_pTexture = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
    if (!m_pTexture->IsValid())
        std::wcout << L"gDiffuseMap is not valid!\n";
}

dae::FlatEffect::~FlatEffect()
{

}

HRESULT dae::FlatEffect::CreateLayout(ID3D11Device* pDevice)
{
    //=============================================================//
    //					1. Create Vertex Layout		               //
    //=============================================================//
    static constexpr uint32_t numElements{ 4 };
    D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

    vertexDesc[0].SemanticName = "POSITION";
    vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[0].AlignedByteOffset = 0;
    vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[1].SemanticName = "TEXCOORD";
    vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    vertexDesc[1].AlignedByteOffset = 12;
    vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[2].SemanticName = "NORMAL";
    vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[2].AlignedByteOffset = 20;
    vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    vertexDesc[3].SemanticName = "TANGENT";
    vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    vertexDesc[3].AlignedByteOffset = 32;
    vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    //=============================================================//
    //					2. Create Input Layout		               //
    //=============================================================//
    return CreateInputLayout(pDevice, vertexDesc, numElements);
}

void dae::FlatEffect::SetTextureMap(TextureDX11* pTexture)
{
    if (m_pTexture->IsValid() && pTexture)
        m_pTexture->SetResource(pTexture->GetSRV());
}
