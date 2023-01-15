#include "pch.h"
#include "SoftwareRasterizer.h"
#include "Scene.h"
#include "Camera.h"
#include "DataTypes.h"
#include "ResourceManager.h"
#include "Texture.h"

#include <future>
#include <ppl.h> // parallel_for

//#define ASYNC
#define PARALLEL_FOR

namespace dae
{
	Material* SoftwareRasterizer::m_pMaterialBuffer{ nullptr };
	Matrix* SoftwareRasterizer::m_pTBNBuffer{nullptr};

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
		//light.direction.Normalize();
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

#if defined(ASYNC)
		static std::mutex renderMutex;
		const uint32_t numCores{ std::thread::hardware_concurrency() };
		std::vector<std::future<void>> async_futures{};
		const uint32_t numTriangles{ static_cast<uint32_t>(pMesh->indices.size() / step) };
		const uint32_t numTrianglesPerTask{ numTriangles / numCores };
		uint32_t numUnassignedTriangles{ numTriangles % numCores };
		size_t currentTriangleIndex{0};

		for (uint32_t coreId{ 0 }; coreId < numCores; ++coreId)
		{
			uint32_t taskSize{ numTrianglesPerTask };
			if (numUnassignedTriangles > 0)
			{
				++taskSize;
				--numUnassignedTriangles;
			}

			async_futures.push_back(std::async(std::launch::async, [=, this]()->void
				{
					std::lock_guard<std::mutex> lock(renderMutex);
					const size_t triangleIndexEnd{ currentTriangleIndex + taskSize };
					for (size_t triangleIndex{ currentTriangleIndex }; triangleIndex < triangleIndexEnd; ++triangleIndex)
					{
						ProcessTriangle(triangleIndex, pMesh);
					}
				}));
			currentTriangleIndex += taskSize;
		}
//#elif defined(PARALLEL_FOR)
		static std::mutex renderMutex;
		const uint32_t numTriangles{ static_cast<uint32_t>(pMesh->indices.size() / step) };
		concurrency::parallel_for(0u, numTriangles, [=, this](int i)->void
			{
				std::lock_guard<std::mutex> lock(renderMutex);
				ProcessTriangle(i, pMesh);
			});

#else
		size_t triangleIdx{};
		for (size_t i{}; i + 3 <= pMesh->indices.size(); i += step)
		{
			ProcessTriangle(triangleIdx, pMesh);
			++triangleIdx;
		}

#endif
	}

	void SoftwareRasterizer::VertexTransformationFunction(Mesh& mesh, const Camera& camera) const
	{
		mesh.vertices_out.clear();
		mesh.vertices_out.resize(mesh.vertices.size());
		Matrix worldViewProjectionMatrix{ mesh.worldMatrix * camera.viewMatrix * camera.ProjectionMatrix };

		uint32_t numVerts{ static_cast<uint32_t>(mesh.vertices.size()) };
		//static std::mutex renderMutex;
#if defined(PARALLEL_FOR)
		concurrency::parallel_for(0u, numVerts, [&](int i)->void
			{
				//std::lock_guard<std::mutex> lock(renderMutex);
				VertexShading(i, mesh.vertices[i], mesh, worldViewProjectionMatrix, camera);
			});
#else

		for (size_t i{}; i < numVerts; i++)
		{
			VertexShading(i, vertex, mesh, worldViewProjectionMatrix, camera);
		}
#endif
	}

	void SoftwareRasterizer::VertexShading(size_t i, const Vertex& vertex, Mesh& mesh, const Matrix& worldViewProjectionMatrix, const Camera& camera) const
	{
		Vertex_Out transformedVertex{};

		//to viewspace
		Vector4 vertexPos{ vertex.position.x, vertex.position.y, vertex.position.z, 1.f };
		transformedVertex.position = worldViewProjectionMatrix.TransformPoint(vertexPos);

		//parse unchanged data
		transformedVertex.uv = vertex.uv;
		//transformedVertex.color = vertex.color;

		// to viewspace
		transformedVertex.normal = mesh.worldMatrix.TransformVector(vertex.normal);
		//transformedVertex.normal.Normalize();

		transformedVertex.tangent = mesh.worldMatrix.TransformVector(vertex.tangent);
		//transformedVertex.tangent.Normalize();

		transformedVertex.viewDirection = (mesh.worldMatrix.TransformPoint(vertex.position)) - camera.origin;
		//transformedVertex.viewDirection.Normalize();
		//transformedVertex.worldPos = (mesh.worldMatrix.TransformPoint(vertexPos));

		mesh.vertices_out[i] = transformedVertex;
	}

	Vector2 SoftwareRasterizer::VertexToScreenSpace(const Vector4& vertex) const
	{
		return
		{
			(vertex.x + 1) / 2.f * m_Width,
			(1.f - vertex.y) / 2.f * m_Height
		};
	}

	bool SoftwareRasterizer::IsPixelInTriangle(const Triangle& verts, const Vector2& pixel, float* crossArr) const
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
		return cross <= 0.f;
	}

	void SoftwareRasterizer::PerspectiveDivide(Triangle& triangle) const
	{
		for (size_t i{}; i < triangle.size(); i++)
		{
			triangle[i].position.x /= triangle[i].position.w;
			triangle[i].position.y /= triangle[i].position.w;
			triangle[i].position.z /= triangle[i].position.w;
		}

		//for (auto& vertex : triangle)
		//{
		//	vertex.position.x /= vertex.position.w;
		//	vertex.position.y /= vertex.position.w;
		//	vertex.position.z /= vertex.position.w;
		//}
	}

	void SoftwareRasterizer::GetBoundingBoxPixelsFromTriangle(const Triangle& triangle, int& minX, int& minY, int& maxX, int& maxY) const
	{
		Vector2 v0{ VertexToScreenSpace(triangle[0].position) };
		Vector2 v1{ VertexToScreenSpace(triangle[1].position) };
		Vector2 v2{ VertexToScreenSpace(triangle[2].position) };

		minX = int(v0.x);
		minY = int(v0.y);
		maxX = minX;
		maxY = minY;

		minX = Min(minX, int(v1.x));
		minY = Min(minY, int(v1.y));

		maxX = Max(maxX, int(v1.x));
		maxY = Max(maxY, int(v1.y));

		minX = Min(minX, int(v2.x));
		minY = Min(minY, int(v2.y));

		maxX = Max(maxX, int(v2.x));
		maxY = Max(maxY, int(v2.y));

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

		if (!TriangleClipTest(triangle))
			return;

		const auto clip1VertOutside{ [this](Triangle& triangle) -> void
			{
				const float t0{ (-triangle[0].position.z) / (triangle[1].position.z - triangle[0].position.z)};
				const float t1{ (-triangle[0].position.z) / (triangle[2].position.z - triangle[0].position.z)};
				const auto v0Lerped0{ LerpVertex(triangle[0], triangle[1], t0)};
				const auto v0Lerped1{ LerpVertex(triangle[0], triangle[2], t1)};
				//draw 2 triangles
				triangle[0] = v0Lerped0;
				RenderTriangle(triangle);
				triangle[0] = v0Lerped1;
				RenderTriangle(triangle);
			} };
		const auto clip2VertsOutside{ [this](Triangle& triangle) -> void
			{
				const float t0{ (-triangle[0].position.z) / (triangle[2].position.z - triangle[0].position.z)};
				const float t1{ (-triangle[1].position.z) / (triangle[2].position.z - triangle[1].position.z)};
				triangle[0] = LerpVertex(triangle[0], triangle[2], t0);
				triangle[1] = LerpVertex(triangle[1], triangle[2], t1);

				//draw 1 triangle
				RenderTriangle(triangle);
			} };

		TriangleNearClipTest(triangle, clip1VertOutside, clip2VertsOutside);
	}

	bool SoftwareRasterizer::TriangleClipTest(const Triangle& triangle) const
	{
		// clipping tests
		if (triangle[0].position.x > triangle[0].position.w &&
			triangle[1].position.x > triangle[1].position.w &&
			triangle[2].position.x > triangle[2].position.w)
			return false;

		if (triangle[0].position.x < -triangle[0].position.w &&
			triangle[1].position.x < -triangle[1].position.w &&
			triangle[2].position.x < -triangle[2].position.w)
			return false;

		if (triangle[0].position.y > triangle[0].position.w &&
			triangle[1].position.y > triangle[1].position.w &&
			triangle[2].position.y > triangle[2].position.w)
			return false;

		if (triangle[0].position.y < -triangle[0].position.w &&
			triangle[1].position.y < -triangle[1].position.w &&
			triangle[2].position.y < -triangle[2].position.w)
			return false;

		if (triangle[0].position.z > triangle[0].position.w &&
			triangle[1].position.z > triangle[1].position.w &&
			triangle[2].position.z > triangle[2].position.w)
			return false;

		if (triangle[0].position.z < 0.f &&
			triangle[1].position.z < 0.f &&
			triangle[2].position.z < 0.f)
			return false;

		return true;
	}

	void SoftwareRasterizer::TriangleNearClipTest(Triangle& triangle, 
		std::function<void(Triangle& triangle)> case1,
		std::function<void(Triangle& triangle)> case2) const
	{
		if (triangle[0].position.z < 0.f)
		{
			if (triangle[1].position.z < 0.f)
			{
				//v0 and v1 are on the wrong side
				//case2(triangle[0], triangle[1], triangle[2]);
				case2(triangle);
			}
			else if (triangle[2].position.z < 0.f)
			{
				//v0 and v2 are on the wrong side
				//case2(triangle[0], triangle[2], triangle[1]);
				std::swap(triangle[1], triangle[2]);
				case2(triangle);
			}
			else
			{
				// only v0 is at the wrong side
				//case1(triangle[0], triangle[1], triangle[2]);
				case1(triangle);
			}
		}
		else if (triangle[1].position.z < 0.f)
		{
			if (triangle[2].position.z < 0.f)
			{
				//v1 and v2 are on the wrong side
				//case2(triangle[1], triangle[2], triangle[0]);
				std::swap(triangle[0], triangle[1]);
				std::swap(triangle[1], triangle[2]);
				case2(triangle);
			}
			else
			{
				//only v1 is at the wrong side
				//case1(triangle[1], triangle[0], triangle[2]);
				std::swap(triangle[0], triangle[1]);
				case1(triangle);
			}
		}
		else if (triangle[2].position.z < 0.f)
		{
			//only v2 is at the wrong side
			//case1(triangle[2], triangle[0], triangle[1]);
			std::swap(triangle[0], triangle[2]);
			std::swap(triangle[1], triangle[2]);
			case1(triangle);
		}
		else
		{
			// all verts are inside, no clipping
			//rendertriangle
			RenderTriangle(triangle);
		}
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
		assert(m_pMaterialBuffer && "materialbuffer is nullptr!\n");

		ColorRGB colorOut{};

		switch (m_pMaterialBuffer->shaderId)
		{
		case 0:
			colorOut = LambertPixelShader(vertex);
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

	ColorRGB SoftwareRasterizer::LambertPixelShader(const Vertex_Out& vertex) const
	{
		//Vector3 viewDir{ (vertex.worldPos.GetXYZ() - m_pCameraBuffer->invViewMatrix[3].GetXYZ())};
		Vector3 viewDir{ (vertex.viewDirection) };
		//viewDir.Normalize();

		//get textures
		auto& diffuseMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[0]) };
		auto& normalMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[1]) };
		auto& specularMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[2]) };
		auto& glossinessMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[3]) };

		ColorRGB colorOut{};

		//normal
		Vector3 normal{};
		const Vector3 binormal = Vector3::Cross(vertex.normal, vertex.tangent).Normalized();
		const Matrix tbn = Matrix{ vertex.tangent,binormal,vertex.normal,Vector3::Zero };

		const ColorRGB normalColor{ (2 * normalMap.Sample(vertex.uv)) - ColorRGB{1,1,1} };
		const Vector3 normalSample{ normalColor.r,normalColor.g,normalColor.b };
		normal = tbn.TransformVector(normalSample);
		normal.Normalize();

		//shading
		float observedArea{ Max(Vector3::Dot(normal, -m_pLightBuffer->direction), 0.f) };

		ColorRGB baseColor{ diffuseMap.Sample(vertex.uv) };
		ColorRGB ambient{ 0.025f, 0.025f, 0.025f };
		ColorRGB diffuse{ Lambert(1.f, baseColor) };
		float spec{ specularMap.Sample(vertex.uv).r };
		float glossiness{ 25.f };
		float exp{ specularMap.Sample(vertex.uv).r * glossiness };
		ColorRGB specular{ spec * Phong(1.f, exp, -m_pLightBuffer->direction, viewDir, normal) };

		colorOut = ambient + specular + diffuse * observedArea * m_pLightBuffer->intensity;

		return colorOut;
	}

	ColorRGB SoftwareRasterizer::FlatPixelShader(const Vertex_Out& vertex) const
	{
		//get textures
		auto& diffuseMap{ ResourceManager::GetTexture(m_pMaterialBuffer->textures[0]) };

		ColorRGBA colorSample{ diffuseMap.SampleRGBA(vertex.uv)};
		Uint8 r{}, g{}, b{};
		SDL_GetRGB(m_pBackBufferPixels[s_RenderStats.currentPixel], m_pBackBuffer->format, &r, &g, &b);
		ColorRGB backBufferColor{r / 255.f, g / 255.f, b / 255.f};

		ColorRGB colorOut{ ColorRGB::Lerp(backBufferColor, colorSample.Rgb(), colorSample.a)};

		return colorOut;
	}

	void SoftwareRasterizer::RenderTriangle(Triangle& triangle) const
	{
		PerspectiveDivide(triangle);

		//find pixelrange to test overlap
		int minX{}, minY{}, maxX{}, maxY{};
		GetBoundingBoxPixelsFromTriangle(triangle, minX, minY, maxX, maxY);

		for (int px{ minX }; px < maxX; ++px)
		{
			for (int py{ minY }; py < maxY; ++py)
			{
				s_RenderStats.currentPixel = size_t(px + (py * m_Width));
				float crossArr[3]{};

				if (IsPixelInTriangle(triangle, { float(px), float(py) }, crossArr))
				{
					float pixelZ{ 1.f / GetBarycentricInterpolation(
						1.f / triangle[0].position.z,
						1.f / triangle[1].position.z,
						1.f / triangle[2].position.z, crossArr) };

					if (pixelZ >= 0.f && pixelZ <= 1.f && //frustum clipping
						pixelZ < m_pDepthBuffer[s_RenderStats.currentPixel]) //depthtest
					{
						m_pDepthBuffer[s_RenderStats.currentPixel] = pixelZ;
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

						//Vector4 currentWorldPos{ GetBarycentricInterpolation(
						//	triangle[0].worldPos,
						//	triangle[1].worldPos,
						//	triangle[2].worldPos, crossArr) * interpelatedW };

						currentNormal.Normalize();
						currentTangent.Normalize();
						viewDirection.Normalize();

						Vertex_Out currentPixelData
						{
							currentPos,
							//currentColor,
							currentUv,
							currentNormal,
							currentTangent,
							viewDirection
							//currentWorldPos
						};

						m_pBackBufferPixels[s_RenderStats.currentPixel] = PixelShading(currentPixelData);
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
		float cosAlpha{ Max(Vector3::Dot(v, reflect), 0.f) };

		float specular{ (cosAlpha > 0.f) ? ks * powf(cosAlpha, exp) : 0.f };
		return ColorRGB{ specular, specular, specular };
	}
}