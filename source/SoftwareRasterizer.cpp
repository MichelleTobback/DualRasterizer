#include "pch.h"
#include "SoftwareRasterizer.h"
#include "Scene.h"
#include "Camera.h"
#include "DataTypes.h"
#include "ResourceManager.h"
#include "Texture.h"

namespace dae
{
	Material* SoftwareRasterizer::m_pMaterialBuffer{ nullptr };

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

		std::cout << "Software rasterizer is initialized and ready!\n";
	}

	SoftwareRasterizer::~SoftwareRasterizer()
	{
		delete[] m_pDepthBuffer;
	}

	void SoftwareRasterizer::Update(const Timer* pTimer)
	{

	}

	void SoftwareRasterizer::Render(Scene* pScene) const
	{
		SDL_LockSurface(m_pBackBuffer);

		// clearColor
		SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

		// set all depthbuffer elements to max float value
		std::fill_n(m_pDepthBuffer, m_Width * m_Height, FLT_MAX);

		//temp-------//
		Light light{};
		light.direction = { 0.577f, -0.577f, 0.577f };
		light.direction.Normalize();
		light.intensity = 7.f;

		m_pLightBuffer = &light;
		m_pCameraBuffer = &pScene->GetCamera();
		//-----------//

		//RENDER LOGIC
		for (auto& pMesh : pScene->m_pMeshes)
		{
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
		m_pMaterialBuffer = &ResourceManager::GetMaterial(pMesh->materialId);

		VertexTransformationFunction(*pMesh, camera);

		size_t step{ GetIndexStep(pMesh->primitiveTopology) };
		size_t triangleIdx{};

		for (size_t i{}; i + 3 <= pMesh->indices.size(); i += step)
		{
			size_t i0{}, i1{}, i2{};
			GetTriangleIndices(*pMesh, triangleIdx, i0, i1, i2);

			std::vector<Vertex_Out> currentTriangleVerts{ pMesh->vertices_out[i0], pMesh->vertices_out[i1], pMesh->vertices_out[i2] };

			RenderTriangle(currentTriangleVerts);
			++triangleIdx;
		}
	}

	void SoftwareRasterizer::VertexTransformationFunction(Mesh& mesh, const Camera& camera) const
	{
		mesh.vertices_out.clear();
		Matrix worldViewProjectionMatrix{ mesh.worldMatrix * camera.viewMatrix * camera.ProjectionMatrix };

		for (Vertex& vertex : mesh.vertices)
		{
			Vertex_Out transformedVertex{};

			//to viewspace
			Vector4 vertexPos{ vertex.position.x, vertex.position.y, vertex.position.z, 1.f };
			transformedVertex.position = worldViewProjectionMatrix.TransformPoint(vertexPos);

			//perspective divide
			transformedVertex.position.x /= transformedVertex.position.w;
			transformedVertex.position.y /= transformedVertex.position.w;
			transformedVertex.position.z /= transformedVertex.position.w;

			//parse unchanged data
			transformedVertex.uv = vertex.uv;
			//transformedVertex.color = vertex.color;

			// to viewspace
			transformedVertex.normal = mesh.worldMatrix.TransformVector(vertex.normal);
			transformedVertex.normal.Normalize();

			transformedVertex.tangent = mesh.worldMatrix.TransformVector(vertex.tangent);
			transformedVertex.tangent.Normalize();

			transformedVertex.viewDirection = (mesh.worldMatrix.TransformPoint(vertex.position)) - camera.origin;
			transformedVertex.viewDirection.Normalize();

			mesh.vertices_out.push_back(transformedVertex);
		}
	}

	Vector2 SoftwareRasterizer::VertexToScreenSpace(const Vector4& vertex) const
	{
		return
		{
			(vertex.x + 1) / 2.f * m_Width,
			(1.f - vertex.y) / 2.f * m_Height
		};
	}

	bool SoftwareRasterizer::IsPixelInTriangle(const std::vector<Vertex_Out>& verts, const Vector2& pixel, float* crossArr) const
	{
		Vector2 v0{ VertexToScreenSpace(verts[0].position) };
		Vector2 v1{ VertexToScreenSpace(verts[1].position) };
		Vector2 v2{ VertexToScreenSpace(verts[2].position) };

		if (!IsPixelAtCorrectSide(v0, v1, pixel, crossArr[2]))
			return false;
		if (!IsPixelAtCorrectSide(v1, v2, pixel, crossArr[0]))
			return false;
		if (!IsPixelAtCorrectSide(v2, v0, pixel, crossArr[1]))
			return false;

		return true;
	}

	bool SoftwareRasterizer::IsPixelAtCorrectSide(const Vector2& v0, const Vector2& v1, const Vector2& pixel, float& cross) const
	{
		Vector2 edge{ v1 - v0 };
		Vector2 vertToPixel{ pixel - v0 };
		cross = Vector2::Cross(vertToPixel, edge);
		if (cross > 0.f)
			return false;

		return true;
	}

	void SoftwareRasterizer::GetBoundingBoxPixelsFromTriangle(const std::vector<Vertex_Out>& triangle, int& minX, int& minY, int& maxX, int& maxY) const
	{
		Vector2 v0{ VertexToScreenSpace(triangle[0].position) };
		Vector2 v1{ VertexToScreenSpace(triangle[1].position) };
		Vector2 v2{ VertexToScreenSpace(triangle[2].position) };

		minX = int(v0.x);
		minY = int(v0.y);
		maxX = minX;
		maxY = minY;

		minX = std::min(minX, int(v1.x));
		minY = std::min(minY, int(v1.y));

		maxX = std::max(maxX, int(v1.x));
		maxY = std::max(maxY, int(v1.y));

		minX = std::min(minX, int(v2.x));
		minY = std::min(minY, int(v2.y));

		maxX = std::max(maxX, int(v2.x));
		maxY = std::max(maxY, int(v2.y));

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

	bool SoftwareRasterizer::IsTriangleInWindow(const Vector4& v0, const Vector4& v1, const Vector4& v2) const
	{
		if (v0.x < -1.f || v0.x > 1.f ||
			v0.y < -1.f || v0.y > 1.f)
			return false;

		if (v1.x < -1.f || v1.x > 1.f ||
			v1.y < -1.f || v1.y > 1.f)
			return false;

		if (v1.x < -1.f || v1.x > 1.f ||
			v1.y < -1.f || v1.y > 1.f)
			return false;

		return true;
	}

	Uint32 SoftwareRasterizer::PixelShading(const Vertex_Out& vertex) const
	{
		assert(m_pMaterialBuffer && "materialbuffer is nullptr!\n");

		//get textures
		auto& diffuseMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[0]) };
		auto& normalMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[1]) };
		auto& specularMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[2]) };
		auto& glossinessMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[3]) };

		ColorRGB colorOut{};

		//normal
		Vector3 normal{};
		const Vector3 binormal = Vector3::Cross(vertex.normal, vertex.tangent);
		const Matrix tbn = Matrix{ vertex.tangent,binormal,vertex.normal,Vector3::Zero };

		const ColorRGB normalColor{ (2 * normalMap.Sample(vertex.uv)) - ColorRGB{1,1,1} };
		const Vector3 normalSample{ normalColor.r,normalColor.g,normalColor.b };
		normal = tbn.TransformVector(normalSample);

		//shading
		float observedArea{ std::max(Vector3::Dot(normal, -m_pLightBuffer->direction), 0.f) };

		ColorRGB baseColor{ diffuseMap.Sample(vertex.uv) };
		ColorRGB ambient{ 0.025f, 0.025f, 0.025f };
		ColorRGB diffuse{ Lambert(1.f, baseColor) };
		float spec{ specularMap.Sample(vertex.uv).r };
		float glossiness{ 25.f };
		float exp{ specularMap.Sample(vertex.uv).r * glossiness };
		ColorRGB specular{ spec * Phong(1.f, exp, -m_pLightBuffer->direction, vertex.viewDirection, normal) };

		//colorOut = diffuse + (diffuse + specular) * observedArea * m_pLightBuffer->intensity;

		colorOut = ambient + specular + diffuse * observedArea * m_pLightBuffer->intensity;

		//Update Color in Buffer
		colorOut.MaxToOne();

		return SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(colorOut.r * 255),
			static_cast<uint8_t>(colorOut.g * 255),
			static_cast<uint8_t>(colorOut.b * 255));
	}

	void SoftwareRasterizer::RenderTriangle(std::vector<Vertex_Out>& triangle) const
	{
		if (!IsTriangleInWindow(triangle[0].position, triangle[1].position, triangle[2].position))
			return;

		//find pixelrange to test overlap
		int minX{}, minY{}, maxX{}, maxY{};
		GetBoundingBoxPixelsFromTriangle(triangle, minX, minY, maxX, maxY);

		for (int px{ minX }; px < maxX; ++px)
		{
			for (int py{ minY }; py < maxY; ++py)
			{
				size_t pixelIndex{ size_t(px + (py * m_Width)) };
				float crossArr[3]{};

				if (IsPixelInTriangle(triangle, { float(px), float(py) }, crossArr))
				{
					float pixelZ{ 1.f / GetBarycentricInterpolation(
						1.f / triangle[0].position.z,
						1.f / triangle[1].position.z,
						1.f / triangle[2].position.z, crossArr) };

					if (pixelZ >= 0.f && pixelZ <= 1.f && //frustum clipping
						pixelZ < m_pDepthBuffer[pixelIndex]) //depthtest
					{
						m_pDepthBuffer[pixelIndex] = pixelZ;
						float interpelatedW{ 1.f / GetBarycentricInterpolation(
						1.f / triangle[0].position.w,
						1.f / triangle[1].position.w,
						1.f / triangle[2].position.w, crossArr) };

						Vector2 currentUv{ interpelatedW * GetBarycentricInterpolation(
							triangle[0].uv / triangle[0].position.w,
							triangle[1].uv / triangle[1].position.w,
							triangle[2].uv / triangle[2].position.w, crossArr) };

						Vector4 currentPos{ GetBarycentricInterpolation(
							triangle[0].position,
							triangle[1].position,
							triangle[2].position, crossArr) * interpelatedW };

						//ColorRGB currentColor{ interpelatedW * GetBarycentricInterpolation(
						//	triangle[0].color,
						//	triangle[1].color,
						//	triangle[2].color, crossArr) };

						Vector3 currentNormal{ interpelatedW * GetBarycentricInterpolation(
							triangle[0].normal,
							triangle[1].normal,
							triangle[2].normal, crossArr) };

						Vector3 currentTangent{ interpelatedW * GetBarycentricInterpolation(
							triangle[0].tangent,
							triangle[1].tangent,
							triangle[2].tangent, crossArr) };

						Vector3 viewDirection{ interpelatedW * GetBarycentricInterpolation(
							triangle[0].viewDirection,
							triangle[1].viewDirection,
							triangle[2].viewDirection, crossArr) };

						Vertex_Out currentPixelData
						{
							currentPos,
							//currentColor,
							currentUv,
							currentNormal.Normalized(),
							currentTangent.Normalized(),
							viewDirection.Normalized()
						};

						m_pBackBufferPixels[pixelIndex] = PixelShading(currentPixelData);
					}
				}
			}
		}
	}

	ColorRGB SoftwareRasterizer::Lambert(float kd, const ColorRGB& cd) const
	{
		return kd * cd / PI;
	}

	ColorRGB SoftwareRasterizer::Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n) const
	{
		Vector3 reflect{ Vector3::Reflect(l, n) };
		float cosAlpha{ std::max(Vector3::Dot(v, reflect), 0.f) };

		float specular{ (cosAlpha > 0.f) ? ks * powf(cosAlpha, exp) : 0.f };
		return ColorRGB{ specular, specular, specular };
	}
}