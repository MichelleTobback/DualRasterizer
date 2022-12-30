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
			Software = 0, Hardware = 1
		};

		Renderer(SDL_Window* pWindow);
		virtual ~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		virtual void Update(const Timer* pTimer) = 0;
		virtual void Render(Scene* pScene) const = 0;
		virtual void KeyDownEvent(SDL_KeyboardEvent e) {}

	protected:
		virtual void RenderMesh(Mesh* pMesh, const Camera& camera) const = 0;

		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		static Light* m_pLightBuffer;
		static Camera* m_pCameraBuffer;
	};
}
