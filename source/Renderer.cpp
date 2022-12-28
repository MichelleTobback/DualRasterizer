#include "pch.h"
#include "Renderer.h"

namespace dae 
{
	Light* Renderer::m_pLightBuffer{nullptr};
	Camera* Renderer::m_pCameraBuffer{nullptr};

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	}

	Renderer::~Renderer()
	{
		
	}
}
