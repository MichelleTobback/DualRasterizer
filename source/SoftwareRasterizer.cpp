#include "pch.h"
#include "SoftwareRasterizer.h"
#include "Scene.h"
#include "Camera.h"
#include "DataTypes.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "ConsoleLog.h"

#include <array>

namespace dae
{
	using namespace Log;

	Material* SoftwareRasterizer::s_pMaterialBuffer{ nullptr };

	struct RenderStats
	{
		size_t currentPixel{};
	};

	static RenderStats s_RenderStats{};

	SoftwareRasterizer::SoftwareRasterizer(SDL_Window* pWindow)
		: Renderer(pWindow)
	{
		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;
		//init depthbuffer
		m_pDepthBuffer = new float[m_Width * m_Height];
		// set all depthbuffer elements to max float value
		std::fill_n(m_pDepthBuffer, m_Width * m_Height, FLT_MAX);

		m_ClearColor = ColorRGB{ 0.39f, 0.39f, 0.39f };

		TSTRING msg{ _T("\nSoftware rasterizer is initialized and ready!\n") };
		PrintMessage(msg, MSG_LOGGER_SOFTWARERASTERIZER, MSG_COLOR_SOFTWARERASTERIZER, MSG_COLOR_SUCCESS);
	}

	SoftwareRasterizer::~SoftwareRasterizer()
	{
		delete[] m_pDepthBuffer;
	}

	void SoftwareRasterizer::Update(const Timer* pTimer)
	{

	}

	void SoftwareRasterizer::KeyDownEvent(SDL_KeyboardEvent e)
	{
		Renderer::KeyDownEvent(e);

		switch (e.keysym.scancode)
		{
		case SDL_SCANCODE_F5:
		{
			//cycle shading modes
			ToggleShadingMode();
		}
		break;

		case SDL_SCANCODE_F6:
		{
			//toggle normal map sampling
			s_Settings.useNormalMap = !s_Settings.useNormalMap;
			TSTRING msg{ _T("Normal map : ") + BoolToString(s_Settings.useNormalMap) };
			PrintMessage(msg, MSG_LOGGER_SOFTWARERASTERIZER, MSG_COLOR_SOFTWARERASTERIZER);
		}
			break;

		case SDL_SCANCODE_F7:
		{
			//toggle depth buffer visualization
			s_Settings.visualizeDepthBuffer = !s_Settings.visualizeDepthBuffer;
			TSTRING msg{ _T("Visualize depth buffer : ") + BoolToString(s_Settings.visualizeDepthBuffer) };
			PrintMessage(msg, MSG_LOGGER_SOFTWARERASTERIZER, MSG_COLOR_SOFTWARERASTERIZER);
		}
			break;

		case SDL_SCANCODE_F8:
		{
			//toggle bounding box visualization
			s_Settings.visualizeBoundingBox = !s_Settings.visualizeBoundingBox;
			TSTRING msg{ _T("Visualize bounding box : ") + BoolToString(s_Settings.visualizeBoundingBox) };
			PrintMessage(msg, MSG_LOGGER_SOFTWARERASTERIZER, MSG_COLOR_SOFTWARERASTERIZER);
		}
			break;
		}
	}

	void SoftwareRasterizer::ToggleShadingMode()
	{
		size_t shadingMode{ static_cast<size_t>(s_Settings.shadingMode) };
		if (++shadingMode == static_cast<size_t>(Renderer::ShadingMode::End))
		{
			shadingMode = 0;
		}
		s_Settings.shadingMode = static_cast<Renderer::ShadingMode>(shadingMode);

		TSTRING msg{ _T("Shading mode : ") };
		switch (s_Settings.shadingMode)
		{
		case ShadingMode::Combined:
			msg.append(_T("Combined"));
			break;

		case ShadingMode::ObservedArea:
			msg.append(_T("Observed Area"));
			break;

		case ShadingMode::Diffuse:
			msg.append(_T("Diffuse"));
			break;

		case ShadingMode::Specular:
			msg.append(_T("Specular"));
			break;
		}
		PrintMessage(msg, MSG_LOGGER_SOFTWARERASTERIZER, MSG_COLOR_SOFTWARERASTERIZER);
	}

	void SoftwareRasterizer::Render(Scene* pScene) const
	{
		SDL_LockSurface(m_pBackBuffer);

		// clearColor
		const ColorRGB& clearColor{ GetClearColor() };
		Uint8 r{ static_cast<Uint8>(clearColor.r * 255) };
		Uint8 g{ static_cast<Uint8>(clearColor.g * 255) };
		Uint8 b{ static_cast<Uint8>(clearColor.b * 255) };
		SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, r, g, b));

		// set all depthbuffer elements to max float value
		std::fill_n(m_pDepthBuffer, m_Width * m_Height, FLT_MAX);

		//temp-------//
		Light light{};
		light.direction = { 0.577f, -0.577f, 0.577f };
		//light.direction.Normalize();
		light.intensity = 7.f;

		m_pLightBuffer = &light;
		m_pCameraBuffer = &pScene->GetCamera();
		//-----------//

		//RENDER LOGIC
		for (auto& pMesh : pScene->m_pMeshes)
		{
			if (!pMesh->render)
				continue;

			RenderMesh(pMesh.get(), pScene->GetCamera());
		}

		//@END
	//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void SoftwareRasterizer::RenderMesh(Mesh* pMesh, const Camera& camera) const
	{
		s_pMaterialBuffer = &ResourceManager::GetMaterial(pMesh->materialId);

		VertexTransformationFunction(*pMesh, camera);

		size_t step{ GetIndexStep(pMesh->primitiveTopology) };
		size_t triangleIdx{};
		const size_t maxIndices{ pMesh->indices.size() };
		for (size_t i{}; i + 3 <= maxIndices; i += step)
		{
			ProcessTriangle(triangleIdx, pMesh);
			++triangleIdx;
		}
	}

	void SoftwareRasterizer::VertexTransformationFunction(Mesh& mesh, const Camera& camera) const
	{
		mesh.vertices_out.clear();
		mesh.vertices_out.resize(mesh.vertices.size());
		const Matrix worldViewProjectionMatrix{ mesh.worldMatrix * camera.viewMatrix * camera.ProjectionMatrix };

		const uint32_t numVerts{ static_cast<uint32_t>(mesh.vertices.size()) };

		for (size_t i{}; i < numVerts; i++)
		{
			const auto& vertex{ mesh.vertices[i] };
			Vertex_Out transformedVertex{};

			//to viewspace
			Vector4 vertexPos{ vertex.position.x, vertex.position.y, vertex.position.z, 1.f };
			transformedVertex.position = worldViewProjectionMatrix.TransformPoint(vertexPos);

			//parse unchanged data
			transformedVertex.uv = vertex.uv;

			// to viewspace
			transformedVertex.normal = mesh.worldMatrix.TransformVector(vertex.normal);

			transformedVertex.tangent = mesh.worldMatrix.TransformVector(vertex.tangent);

			transformedVertex.viewDirection = (mesh.worldMatrix.TransformPoint(vertex.position)) - camera.origin;
			transformedVertex.viewDirection.Normalize();

			mesh.vertices_out[i] = transformedVertex;
		}
	}

	Vector2 SoftwareRasterizer::VertexToScreenSpace(const Vector4& vertex) const
	{
		return
		{
			(vertex.x + 1) * 0.5f * m_Width,
			(1.f - vertex.y) * 0.5f * m_Height
		};
	}

	bool SoftwareRasterizer::IsPixelInTriangle(const TriangleVec2& verts, const Vector2& pixel, float* crossArr) const
	{
		Vector2 edge0{ verts[1] - verts[0]};
		Vector2 vertToPixel0{ pixel - verts[0]};
		crossArr[2] = Vector2::Cross(vertToPixel0, edge0);

		Vector2 edge1{ verts[2] - verts[1] };
		Vector2 vertToPixel1{ pixel - verts[1] };
		crossArr[0] = Vector2::Cross(vertToPixel1, edge1);

		Vector2 edge2{ verts[0] - verts[2] };
		Vector2 vertToPixel2{ pixel - verts[2] };
		crossArr[1] = Vector2::Cross(vertToPixel2, edge2);

		switch (s_Settings.faceCullingMode)
		{
		case FaceCullingMode::Frontface:
			return crossArr[0] >= 0.f && crossArr[1] >= 0.f && crossArr[2] >= 0.f;
			break;

		case FaceCullingMode::Backface:
			return crossArr[0] <= 0.f && crossArr[1] <= 0.f && crossArr[2] <= 0.f;
			break;

		case FaceCullingMode::None:
			return (crossArr[0] >= 0.f && crossArr[1] >= 0.f && crossArr[2] >= 0.f)
				|| (crossArr[0] <= 0.f && crossArr[1] <= 0.f && crossArr[2] <= 0.f);
			break;
		}

		return false;
	}

	bool SoftwareRasterizer::IsPointInFrustum(const Vector4& point) const
	{
		return !(point.x < -1.f || point.x > 1.f 
			|| point.y < -1.f || point.y > 1.f 
			|| point.z < 0.f || point.z > 1.f);
	}

	void SoftwareRasterizer::PerspectiveDivide(Triangle& triangle) const
	{
		for (size_t i{}; i < triangle.Size(); i++)
		{
			float invW{ 1.f / triangle[i].position.w };

			triangle[i].position.x *= invW;
			triangle[i].position.y *= invW;
			triangle[i].position.z *= invW;
		}
	}

	void SoftwareRasterizer::GetBoundingBoxPixelsFromTriangle(const TriangleVec2& triangle, int& minX, int& minY, int& maxX, int& maxY) const
	{
		minX = int(triangle[0].x);
		minY = int(triangle[0].y);
		maxX = minX;
		maxY = minY;

		minX = Min(minX, int(triangle[1].x));
		minY = Min(minY, int(triangle[1].y));

		maxX = Max(maxX, int(triangle[1].x));
		maxY = Max(maxY, int(triangle[1].y));

		minX = Min(minX, int(triangle[2].x));
		minY = Min(minY, int(triangle[2].y));

		maxX = Max(maxX, int(triangle[2].x));
		maxY = Max(maxY, int(triangle[2].y));

		minX = Clamp(minX - 1, 0, m_Width - 1);
		minY = Clamp(minY - 1, 0, m_Height - 1);
		maxX = Clamp(maxX + 1, 0, m_Width - 1);
		maxY = Clamp(maxY + 1, 0, m_Height - 1);
	}

	void SoftwareRasterizer::GetTriangleIndices(const Mesh& mesh, size_t triangleIndex, size_t& i0, size_t& i1, size_t& i2) const
	{
		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			i0 = mesh.indices[triangleIndex * 3];
			i1 = mesh.indices[triangleIndex * 3 + 1];
			i2 = mesh.indices[triangleIndex * 3 + 2];
			break;

		case PrimitiveTopology::TriangleStrip:
			bool isEvenTriangle{ triangleIndex % 2 == 0 };
			if (isEvenTriangle)
			{
				i0 = mesh.indices[triangleIndex];
				i1 = mesh.indices[triangleIndex + 1];
				i2 = mesh.indices[triangleIndex + 2];
			}
			else
			{
				i0 = mesh.indices[triangleIndex];
				i1 = mesh.indices[triangleIndex + 2];
				i2 = mesh.indices[triangleIndex + 1];
			}
			break;
		}
	}

	size_t SoftwareRasterizer::GetIndexStep(PrimitiveTopology primitiveTopology) const
	{
		size_t step{};

		switch (primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			step = 3;
			break;

		case PrimitiveTopology::TriangleStrip:
			step = 1;
			break;
		}

		return step;
	}

	void SoftwareRasterizer::ProcessTriangle(size_t triangleIndex, Mesh* pMesh) const
	{
		size_t i0{}, i1{}, i2{};
		GetTriangleIndices(*pMesh, triangleIndex, i0, i1, i2);

		Triangle triangle{ pMesh->vertices_out[i0], pMesh->vertices_out[i1], pMesh->vertices_out[i2] };

		PerspectiveDivide(triangle);

		if (!IsPointInFrustum(triangle[0].position)
			|| !IsPointInFrustum(triangle[1].position)
			|| !IsPointInFrustum(triangle[2].position))
			return;

		RenderTriangle(triangle);
	}

	Vertex_Out SoftwareRasterizer::LerpVertex(const Vertex_Out& triangle0, const Vertex_Out& triangle1, float t) const
	{
		Vertex_Out lerpedVertex{};
		lerpedVertex.position = Vector4::Lerp(triangle0.position, triangle1.position, t);
		lerpedVertex.normal = Vector3::Lerp(triangle0.normal, triangle1.normal, t);
		lerpedVertex.tangent = Vector3::Lerp(triangle0.tangent, triangle1.tangent, t);
		lerpedVertex.uv = Vector2::Lerp(triangle0.uv, triangle1.uv, t);
		lerpedVertex.viewDirection = Vector3::Lerp(triangle0.viewDirection, triangle1.viewDirection, t);
		//lerpedVertex.color = ColorRGB::Lerp(triangle0.color, triangle1.color, t);

		return lerpedVertex;
	}

	Uint32 SoftwareRasterizer::PixelShading(const Vertex_Out& vertex) const
	{
		assert(s_pMaterialBuffer && "materialbuffer is nullptr!\n");

		ColorRGB colorOut{};

		switch (s_pMaterialBuffer->shaderId)
		{
		case 0:
			colorOut = VehiclePixelShader(vertex);
			break;
		
		case 1:
			colorOut = FlatPixelShader(vertex);
			break;
		}

		//Update Color in Buffer
		colorOut.MaxToOne();

		return SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(colorOut.r * 255),
			static_cast<uint8_t>(colorOut.g * 255),
			static_cast<uint8_t>(colorOut.b * 255));
	}

	Vector3 SoftwareRasterizer::SampleNormalMap(const Vector3& normal, const Vector3& tangent, const Vector2& uv, const TextureSoftware& normalMap) const
	{
		const Vector3 binormal = Vector3::Cross(normal, tangent).Normalized();
		const Matrix tbn = Matrix{ tangent ,binormal, normal, Vector3::Zero };

		const ColorRGB normalColor{ (2 * normalMap.Sample(uv)) - ColorRGB{1,1,1} };
		const Vector3 normalSample{ normalColor.r,normalColor.g,normalColor.b };
		return tbn.TransformVector(normalSample).Normalized();
	}

	ColorRGB SoftwareRasterizer::VehiclePixelShader(const Vertex_Out& vertex) const
	{
		switch (s_Settings.shadingMode)
		{
		case ShadingMode::Combined:
			return LambertPixelShader(vertex);
			break;

		case ShadingMode::ObservedArea:
			return ObservedAreaPixelShader(vertex);
			break;

		case ShadingMode::Diffuse:
			return DiffusePixelShader(vertex);
			break;

		case ShadingMode::Specular:
			return SpecularPixelShader(vertex);
			break;
		}
		return ColorRGB();
	}

	ColorRGB SoftwareRasterizer::LambertPixelShader(const Vertex_Out& vertex) const
	{
		Vector3 viewDir{ (vertex.viewDirection) };

		//get textures
		auto& diffuseMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[0]) };
		auto& normalMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[1]) };
		auto& specularMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[2]) };
		auto& glossinessMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[3]) };

		ColorRGB colorOut{};

		//normal
		Vector3 normal{(s_Settings.useNormalMap) 
			? SampleNormalMap(vertex.normal, vertex.tangent, vertex.uv, normalMap)
			: vertex.normal};

		//shading
		float observedArea{ Max(Vector3::Dot(normal, -m_pLightBuffer->direction), 0.f) };

		ColorRGB baseColor{ diffuseMap.Sample(vertex.uv) };
		ColorRGB ambient{ 0.025f, 0.025f, 0.025f };
		ColorRGB diffuse{ Lambert(1.f, baseColor) };
		float spec{ specularMap.Sample(vertex.uv).r };
		float glossiness{ 25.f };
		float exp{ glossinessMap.Sample(vertex.uv).r * glossiness };
		ColorRGB specular{ spec * Phong(1.f, exp, -m_pLightBuffer->direction, viewDir, normal) };

		colorOut = ambient + specular + diffuse * observedArea * m_pLightBuffer->intensity;

		return colorOut;
	}

	ColorRGB SoftwareRasterizer::FlatPixelShader(const Vertex_Out& vertex) const
	{
		//get textures
		auto& diffuseMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[0]) };

		ColorRGBA colorSample{ diffuseMap.SampleRGBA(vertex.uv)};
		Uint8 r{}, g{}, b{};
		SDL_GetRGB(m_pBackBufferPixels[s_RenderStats.currentPixel], m_pBackBuffer->format, &r, &g, &b);
		constexpr const float colorDivider{ 1.f / 255.f };
		ColorRGB backBufferColor{r * colorDivider, g * colorDivider, b * colorDivider };

		if (colorSample.a <= 0.0001f)
			return backBufferColor;

		ColorRGB colorOut{ ColorRGB::Lerp(backBufferColor, colorSample.Rgb(), colorSample.a)};
		//ColorRGB colorOut{ (1.f - colorSample.a) * backBufferColor
		//	+ colorSample.a * colorSample.Rgb() };

		return colorOut;
	}

	ColorRGB SoftwareRasterizer::ObservedAreaPixelShader(const Vertex_Out& vertex) const
	{
		//normal
		auto& normalMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[1]) };
		Vector3 normal{ (s_Settings.useNormalMap)
			? SampleNormalMap(vertex.normal, vertex.tangent, vertex.uv, normalMap)
			: vertex.normal };

		float oa{ Max(Vector3::Dot(normal, -m_pLightBuffer->direction), 0.f) };
		return { oa, oa, oa };
	}

	ColorRGB SoftwareRasterizer::DiffusePixelShader(const Vertex_Out& vertex) const
	{
		auto& diffuseMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[0]) };
		auto& normalMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[1]) };
		Vector3 normal{ (s_Settings.useNormalMap)
			? SampleNormalMap(vertex.normal, vertex.tangent, vertex.uv, normalMap)
			: vertex.normal };
		float oa{ Max(Vector3::Dot(normal, -m_pLightBuffer->direction), 0.f) };
		ColorRGB baseColor{ diffuseMap.Sample(vertex.uv) };
		return Lambert(1.f, baseColor) * oa * m_pLightBuffer->intensity;
	}

	ColorRGB SoftwareRasterizer::SpecularPixelShader(const Vertex_Out& vertex) const
	{
		auto& normalMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[1]) };
		auto& specularMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[2]) };
		auto& glossinessMap{ ResourceManager::GetTexture(s_pMaterialBuffer->textures[3]) };

		//normal
		Vector3 normal{ (s_Settings.useNormalMap)
			? SampleNormalMap(vertex.normal, vertex.tangent, vertex.uv, normalMap)
			: vertex.normal };

		float spec{ specularMap.Sample(vertex.uv).r };
		float glossiness{ 25.f };
		float exp{ glossinessMap.Sample(vertex.uv).r * glossiness };
		return { spec * Phong(1.f, exp, -m_pLightBuffer->direction, vertex.viewDirection, normal) };
	}

	void SoftwareRasterizer::RenderTriangle(Triangle& triangle) const
	{
		const float invPosW0{ 1.f / triangle[0].position.w };
		const float invPosW1{ 1.f / triangle[1].position.w };
		const float invPosW2{ 1.f / triangle[2].position.w };

		TriangleVec2 triangleScreenSpace
		{
			VertexToScreenSpace(triangle[0].position),
			VertexToScreenSpace(triangle[1].position),
			VertexToScreenSpace(triangle[2].position)
		};

		//find pixelrange to test overlap
		int minX{}, minY{}, maxX{}, maxY{};
		GetBoundingBoxPixelsFromTriangle(triangleScreenSpace, minX, minY, maxX, maxY);

		for (int px{ minX }; px < maxX; ++px)
		{
			for (int py{ minY }; py < maxY; ++py)
			{
				s_RenderStats.currentPixel = size_t(px + (py * m_Width));

				if (s_Settings.visualizeBoundingBox)
				{
					m_pBackBufferPixels[s_RenderStats.currentPixel] = RGB(255, 255, 255);
					continue;
				}

				float crossArr[3]{};

				if (IsPixelInTriangle(triangleScreenSpace, { float(px), float(py) }, crossArr))
				{
					std::array<float, 3> weights{};
					CalculateBarycentricWeights(weights[0], weights[1], weights[2], crossArr);

					float pixelZ{ 1.f / GetBarycentricInterpolation(
						1.f / triangle[0].position.z,
						1.f / triangle[1].position.z,
						1.f / triangle[2].position.z, weights) };

					if (pixelZ > 1.f || pixelZ < 0.f)
						break;

					if (pixelZ < m_pDepthBuffer[s_RenderStats.currentPixel]) //depthtest
					{
						if (s_pMaterialBuffer->depthWrite)
							m_pDepthBuffer[s_RenderStats.currentPixel] = pixelZ;

						if (s_Settings.visualizeDepthBuffer)
						{
							ColorRGB depthColor{ pixelZ, pixelZ, pixelZ };
							depthColor.MaxToOne();
							float depthRemapped = Remap(depthColor.r, 0.997f, 1.f);
							m_pBackBufferPixels[s_RenderStats.currentPixel] = SDL_MapRGB(m_pBackBuffer->format,
								static_cast<uint8_t>(depthRemapped * 255),
								static_cast<uint8_t>(depthRemapped * 255),
								static_cast<uint8_t>(depthRemapped * 255));
							continue;
						}

						const float interpelatedW{ 1.f / GetBarycentricInterpolation(
						invPosW0,
						invPosW1,
						invPosW2, weights) };

						Vertex_Out currentPixelData{};

						currentPixelData.uv = interpelatedW * GetBarycentricInterpolation(
							triangle[0].uv * invPosW0,
							triangle[1].uv * invPosW1,
							triangle[2].uv * invPosW2, weights);

						currentPixelData.position = GetBarycentricInterpolation(
							triangle[0].position * invPosW0,
							triangle[1].position * invPosW1,
							triangle[2].position * invPosW2, weights) * interpelatedW;

						currentPixelData.normal = interpelatedW * GetBarycentricInterpolation(
							triangle[0].normal * invPosW0,
							triangle[1].normal * invPosW1,
							triangle[2].normal * invPosW2, weights);

						currentPixelData.tangent = interpelatedW * GetBarycentricInterpolation(
							triangle[0].tangent * invPosW0,
							triangle[1].tangent * invPosW1,
							triangle[2].tangent * invPosW2, weights);

						currentPixelData.viewDirection = interpelatedW * GetBarycentricInterpolation(
							triangle[0].viewDirection * invPosW0,
							triangle[1].viewDirection * invPosW1,
							triangle[2].viewDirection * invPosW2, weights);

						currentPixelData.normal.Normalize();
						currentPixelData.tangent.Normalize();
						currentPixelData.viewDirection.Normalize();

						m_pBackBufferPixels[s_RenderStats.currentPixel] = PixelShading(currentPixelData);
					}
				}
			}
		}

	}

	

	ColorRGB SoftwareRasterizer::Lambert(float kd, const ColorRGB& cd) const
	{
		return kd * cd * PI_INV;
	}

	ColorRGB SoftwareRasterizer::Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n) const
	{
		Vector3 reflect{ Vector3::Reflect(l, n) };
		float cosAlpha{ Max(Vector3::Dot(v, reflect), 0.f) };

		float specular{ (cosAlpha > 0.f) ? ks * powf(cosAlpha, exp) : 0.f };
		return ColorRGB{ specular, specular, specular };
	}
}