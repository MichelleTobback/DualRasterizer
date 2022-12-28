#include "pch.h"
#include "Renderer.h"

namespace dae {

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
