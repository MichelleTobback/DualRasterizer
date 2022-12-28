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

float3 gLightDirection = { 0.577f, -0.577f, 0.577f };

float4x4 gWorldViewProj : WorldViewProjection;
float4x4 gWorldMat : WORLD;
float4x4 gONB : VIEWINVERSE;

Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossinessMap : GlossinessMap;

//global constants
float PI = 3.14159265358979323846f;
float LIGHT_INTENSITY = 7.0f;
float SHININESS = 25.0f;

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
	CullMode = back; //default
	FrontCounterClockwise = false; //default
};

BlendState gBlendState {
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = FALSE;
};

DepthStencilState gDepthStencilState
{
	DepthEnable = true;
	DepthWriteMask = 0x0F;
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
	output.Normal = normalize(mul(input.Normal, (float3x3)gWorldMat));
	output.Tangent = normalize(mul(input.Tangent, (float3x3)gWorldMat));
	output.WorldPos = mul(float4(input.Position, 1.0f), gWorldMat);
	return output;
}

//=================================//
//	   Pixel Shader functions	   //
//=================================//

float3 SampleNormalMap(float3 n, float3 t, float2 texCoord, SamplerState samp)
{
	float3 b = normalize(cross(n, t));
	float3x3 tbn = float3x3(t, b, n);

	float3 normalSample = 2.0f * gNormalMap.Sample(samp, texCoord) - 1.0f;
	return normalize(mul(normalSample, tbn));
}

float3 Lambert(float kd, float3 cd)
{
	return mul(kd, cd) / PI;
}

float3 Phong(float ks, float exp, float3 l, float3 v, float3 n)
{
	float3 r = reflect(l, n);
	float cosAlpha = saturate(dot(v, r));
	float specular = ks * pow(cosAlpha, exp);

	return float3( specular, specular, specular );
}

float4 PixelShading(SamplerState samp, VS_OUTPUT input)
{
	float3 viewDir = normalize(input.WorldPos.xyz - gONB[3].xyz);
	float3 normal = SampleNormalMap(input.Normal, input.Tangent, input.TexCoord, samp);
	float3 l = normalize(gLightDirection);

	float observedArea = max(dot(normal, -l), 0.0f);

	float3 ambient = { 0.025f, 0.025f, 0.025f };
	float3 diffuse =  Lambert(1.0f, gDiffuseMap.Sample(samp, input.TexCoord));

	float spec = gSpecularMap.Sample(samp, input.TexCoord).r;
	float exp = gGlossinessMap.Sample(samp, input.TexCoord).r * SHININESS;
	float3 specular = spec * Phong(1.0f, exp, -l, viewDir, normal);

	//return saturate(float4(specular, 1.0f));
	return saturate(float4(ambient + specular + diffuse * observedArea * LIGHT_INTENSITY, 1.0f));
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