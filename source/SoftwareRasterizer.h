#pragma once
#include "Renderer.h"

#include <functional>

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;
	struct Vertex;
	struct Vertex_Out;
	struct Mesh;
	enum class PrimitiveTopology;
	struct Material;
	struct Triangle;

	class SoftwareRasterizer : public Renderer
	{
	public:
		SoftwareRasterizer(SDL_Window* pWindow);
		virtual ~SoftwareRasterizer() override;

		SoftwareRasterizer(const SoftwareRasterizer&) = delete;
		SoftwareRasterizer(SoftwareRasterizer&&) noexcept = delete;
		SoftwareRasterizer& operator=(const SoftwareRasterizer&) = delete;
		SoftwareRasterizer& operator=(SoftwareRasterizer&&) noexcept = delete;

		virtual void Update(const Timer* pTimer) override;
		virtual void Render(Scene* pScene) const override;

	protected:
		virtual void RenderMesh(Mesh* pMesh, const Camera& camera) const override;

	private:
		// transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(Mesh& mesh, const Camera& camera) const;
		void VertexShading(size_t i, const Vertex& vertex, Mesh& mesh, const Matrix& worldViewProjectionMatrix, const Camera& camera) const;
		Vector2 VertexToScreenSpace(const Vector4& vertex) const;
		// vertices in NDC space
		// crossArr parameter (sizeof 3!) is used to store the crossproducts which are needed for the weightcalculation
		bool IsPixelInTriangle(const Triangle& verts, const Vector2& pixel, float* crossArr) const;
		//helper function for IsPixelInTriangle()
		//cross parameter is used to store the crossproduct which is needed for the weightcalculation
		bool IsPixelAtCorrectSide(const Vector2& v0, const Vector2& v1, const Vector2& pixel, float& cross) const;
		//crossArr parameter (sizeof 3!)
		template <typename T>
		inline T GetBarycentricInterpolation(const T& v0, const T& v1, const T& v2, float* crossArr) const
		{
			float totalAreaParallogram{ crossArr[0] + crossArr[1] + crossArr[2] };
			const float w0{ crossArr[0] / totalAreaParallogram };
			const float w1{ crossArr[1] / totalAreaParallogram };
			const float w2{ crossArr[2] / totalAreaParallogram };

			return v0 * w0 + v1 * w1 + v2 * w2;
		}
		void PerspectiveDivide(Triangle& triangle) const;
		// used to minimize pixels overlap test
		// triangle in screenspace
		void GetBoundingBoxPixelsFromTriangle(const Triangle& triangle, int& minX, int& minY, int& maxX, int& maxY) const;
		void GetTriangleIndices(const Mesh& mesh, size_t triangleIndex, size_t& i0, size_t& i1, size_t& i2) const;
		size_t GetIndexStep(PrimitiveTopology primitiveTopology) const;
		bool TriangleClipTest(const Triangle& triangle) const;
		void TriangleNearClipTest(Triangle& triangle,
			std::function<void(Triangle& triangle)> test1,
			std::function<void(Triangle& triangle)> test2) const;
		void ProcessTriangle(size_t triangleIndex, Mesh* pMesh) const;
		Vertex_Out LerpVertex(const Vertex_Out& triangle0, const Vertex_Out& triangle1, float t) const;

		void RenderTriangle(Triangle& triangle) const;
		Uint32 PixelShading(const Vertex_Out& vertex) const;

		ColorRGB LambertPixelShader(const Vertex_Out& vertex) const;
		ColorRGB FlatPixelShader(const Vertex_Out& vertex) const;

		//shading
		/**
		 * \param kd Diffuse Reflection Coefficient
		 * \param cd Diffuse Color
		 * \return Lambert Diffuse Color
		 */
		ColorRGB Lambert(float kd, const ColorRGB& cd) const;
		ColorRGB Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n) const;

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		float* m_pDepthBuffer{ nullptr };

		static Material* m_pMaterialBuffer;
		static Matrix* m_pTBNBuffer;
	};
}
