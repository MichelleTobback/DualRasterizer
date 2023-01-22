#include "pch.h"
#include "Renderer.h"
#include "ConsoleLog.h"

namespace dae 
{
	using namespace Log;

	Renderer::RenderSettings Renderer::s_Settings{};
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

	void Renderer::KeyDownEvent(SDL_KeyboardEvent e)
	{
		switch (e.keysym.scancode)
		{
		case SDL_SCANCODE_F9:
			CycleFaceCullingMode();

			break;

		case SDL_SCANCODE_F10:
		{
			//toggle clear color
			s_Settings.useUniformClearColor = !s_Settings.useUniformClearColor;
			TSTRING msg{ _T("Uniform clear color : ") + BoolToString(s_Settings.useUniformClearColor) };
			PrintMessage(msg, MSG_LOGGER_SHARED, MSG_COLOR_RENDERER);
		}
		break;
		}
	}

	const ColorRGB& Renderer::GetClearColor() const
	{
		return (s_Settings.useUniformClearColor) ? s_Settings.uniformClearColor : m_ClearColor;
	}

	void Renderer::CycleFaceCullingMode()
	{
		int faceCullingMode{ static_cast<int>(s_Settings.faceCullingMode) };
		if (++faceCullingMode == static_cast<int>(Renderer::FaceCullingMode::End))
		{
			faceCullingMode = 0;
		}
		s_Settings.faceCullingMode = static_cast<Renderer::FaceCullingMode>(faceCullingMode);

		TSTRING msg{ _T("Shading mode : ") };
		switch (s_Settings.faceCullingMode)
		{
		case FaceCullingMode::Backface:
			msg.append(_T("Backface"));
			break;

		case FaceCullingMode::Frontface:
			msg.append(_T("Frontface"));
			break;

		case FaceCullingMode::None:
			msg.append(_T("None"));
			break;
		}

		PrintMessage(msg, MSG_LOGGER_SHARED, MSG_COLOR_RENDERER);
	}
}
