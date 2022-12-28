//=================================//
//      Input/Output Structs       //
//=================================//
struct VS_INPUT
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float4 WorldPos : WORLDPOSITION;
};

//=================================//
//			  Globals			   //
//=================================//
float4x4 gWorldViewProj : WorldViewProjection;

Texture2D gDiffuseMap : DiffuseMap;

SamplerState gSamPoint
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};
SamplerState gSamLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};
SamplerState gSamAnisotropic
{
	Filter = ANISOTROPIC;
	AddressU = Wrap;
	AddressV = Wrap;
};

//=================================//
//			  States			   //
//=================================//
RasterizerState gRasterizerState
{
	CullMode = none;
	FrontCounterClockwise = false; //default
};

BlendState gBlendState
{
	BlendEnable[0] = true;
	SrcBlend = src_alpha;
	DestBlend = inv_src_alpha;
	BlendOp = add;
	SrcBlendAlpha = zero;
	DestBlendAlpha = zero;
	BlendOpAlpha = add;
	RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState gDepthStencilState
{
	DepthEnable = true;
	DepthWriteMask = zero;
	DepthFunc = less;
	StencilEnable = false;

	StencilReadMask = 0x0F;
	StencilWriteMask = 0x0F;

	FrontFaceStencilFunc = always;
	BackFaceStencilFunc = always;

	FrontFaceStencilDepthFail = keep;
	BackFaceStencilDepthFail = keep;

	FrontFaceStencilPass = keep;
	BackFaceStencilPass = keep;

	FrontFaceStencilFail = keep;
	BackFaceStencilFail = keep;
};

//=================================//
//         Vertex Shader           //
//=================================//
VS_OUTPUT VS(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul(float4(input.Position, 1.f), gWorldViewProj);
	output.TexCoord = input.TexCoord;
	output.Normal = input.Normal;
	output.Tangent = input.Tangent;
	return output;
}

//=================================//
//	   Pixel Shader functions	   //
//=================================//

float4 PixelShading(SamplerState samp, VS_OUTPUT input)
{
	return gDiffuseMap.Sample(samp, input.TexCoord);
}

//=================================//
//			Pixel Shaders	       //
//=================================//
float4 PS(VS_OUTPUT input) : SV_TARGET
{
	//point
	return PixelShading(gSamPoint, input);
}

float4 PSLINEAR(VS_OUTPUT input) : SV_TARGET
{
	//Linear
	return PixelShading(gSamLinear, input);
}

float4 PSANISOTRPOIC(VS_OUTPUT input) : SV_TARGET
{
	//Anisotropic
	return PixelShading(gSamAnisotropic, input);
}

//=================================//
//			 Techniques	           //
//=================================//
technique11 DefaultTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}

technique11 LinearTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PSLINEAR()));
	}
}

technique11 AnisotropicTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PSANISOTRPOIC()));
	}
}