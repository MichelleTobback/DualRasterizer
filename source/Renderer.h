#pragma once

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;
	struct Mesh;
	struct Camera;
	struct Light;

	class Renderer
	{
	public:
		enum class RenderMode
		{
			Software = 0, Hardware = 1, End = 2
		};
		enum class FaceCullingMode
		{
			Backface = 0, Frontface = 1, None = 2, End = 3
		};
		enum class ShadingMode
		{
			Combined = 0, ObservedArea = 1, Diffuse = 2, Specular = 3, End = 4
		};
		struct RenderSettings
		{
			FaceCullingMode faceCullingMode{ FaceCullingMode::Backface };
			ShadingMode shadingMode{ ShadingMode::Combined };
			bool visualizeDepthBuffer{ false };
			bool visualizeBoundingBox{ false };
			bool useNormalMap{ true };
			bool useUniformClearColor{ false };
			ColorRGB uniformClearColor{0.1f, 0.1f, 0.1f};
		};

		Renderer(SDL_Window* pWindow);
		virtual ~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		virtual void Update(const Timer* pTimer) = 0;
		virtual void Render(Scene* pScene) const = 0;
		virtual void KeyDownEvent(SDL_KeyboardEvent e);

	protected:
		virtual void RenderMesh(Mesh* pMesh, const Camera& camera) const = 0;
		virtual void CycleFaceCullingMode();

		const ColorRGB& GetClearColor() const;

		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		ColorRGB m_ClearColor{};

		bool m_IsInitialized{ false };

		static RenderSettings s_Settings;
		static Light* m_pLightBuffer;
		static Camera* m_pCameraBuffer;
	};
}
