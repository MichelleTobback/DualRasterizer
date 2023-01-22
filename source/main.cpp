#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"
#include "HardwareRasterizerDX11.h"
#include "SoftwareRasterizer.h"
#include "Scene.h"
#include "ConsoleLog.h"

using namespace dae;
using namespace Log;

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

void PrintKeyBindings();
void SwitchRenderMode(Renderer::RenderMode& activeRenderMode);

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//=======================//
	//Create window + surfaces
	//=======================//
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"DirectX - ***Insert Name/Class***",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;
	//=======================//
	// Initialize "framework"
	//=======================//
	const auto pTimer = new Timer();
	bool printFps{ true };

	//Init renderer
	constexpr const size_t numRenderers{ static_cast<size_t>(Renderer::RenderMode::Hardware) + 1 };
	Renderer* pRenderer[numRenderers] = {};
	pRenderer[static_cast<size_t>(Renderer::RenderMode::Software)] = new SoftwareRasterizer(pWindow);
	pRenderer[static_cast<size_t>(Renderer::RenderMode::Hardware)] = new HardwareRasterizerDX11(pWindow);
	Renderer::RenderMode activeRenderer{ Renderer::RenderMode::Software };

	//Init scene
	Scene* pScene{ new ExamScene() };
	pScene->Initialize(float(width), float(height));

	PrintKeyBindings();

	//=======================//
	// Start loop
	//=======================//
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	while (isLooping)
	{
		size_t activeRendererIdx{ static_cast<size_t>(activeRenderer) };
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				//Test for a key
				//if (e.key.keysym.scancode == SDL_SCANCODE_X)
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.scancode == SDL_SCANCODE_F1)
				{
					SwitchRenderMode(activeRenderer);
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F11)
				{
					printFps = !printFps;
					PrintMessage(_T("Print FPS : ") + BoolToString(printFps), MSG_LOGGER_SHARED, MSG_COLOR_MAIN);
				}
				pScene->KeyDownEvent(e.key);
				pRenderer[activeRendererIdx]->KeyDownEvent(e.key);
			default: ;
			}
		}

		//--------- Update ---------
		pScene->Update(pTimer);
		pRenderer[activeRendererIdx]->Update(pTimer);

		//--------- Render ---------
		pRenderer[activeRendererIdx]->Render(pScene);

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			printTimer = 0.f;
			if (printFps)
				std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
		}
	}
	pTimer->Stop();

	//=======================//
	//Shutdown "framework"
	//=======================//

	delete pScene;
	for (auto& r : pRenderer)
		delete r;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}

void SwitchRenderMode(Renderer::RenderMode& activeRenderMode)
{
	size_t renderMode{ static_cast<size_t>(activeRenderMode) };
	if (++renderMode > 1)
	{
		renderMode = 0;
	}
	activeRenderMode = static_cast<Renderer::RenderMode>(renderMode);

	TSTRING msg{ _T("Render mode : ") };
	switch (activeRenderMode)
	{
	case Renderer::RenderMode::Software:
		msg.append(_T("Software"));
		break;

	case Renderer::RenderMode::Hardware:
		msg.append(_T("Hardware"));
		break;
	}
	PrintMessage(msg, MSG_LOGGER_SHARED, MSG_COLOR_MAIN);
}

void PrintKeyBindings()
{
	TSTRING sharedMsg{};
	sharedMsg.append(_T("[Key Bindings]\n"));
	sharedMsg.append(_T("	[F1]	Toggle Rasterizer Mode - (HARDWARE/SOFTWARE)\n"));
	sharedMsg.append(_T("	[F2]	Toggle Vehicle Rotation - (ON/OFF)\n"));
	sharedMsg.append(_T("	[F3]	Toggle FireFX - (ON/OFF)\n"));
	sharedMsg.append(_T("	[F9]	Cycle CullMode - (BACKFACE/FRONTFACE/NONE)\n"));
	sharedMsg.append(_T("	[F10]	Toggle Uniform Clear Color - (ON/OFF)\n"));
	sharedMsg.append(_T("	[F11]	Toggle Print FPS - (ON/OFF)"));
	PrintMessage(sharedMsg, MSG_LOGGER_SHARED, MSG_COLOR_RENDERER, COLOR_GRAY);

	TSTRING hardwareMsg{};
	hardwareMsg.append(_T("[Key Bindings]\n"));
	hardwareMsg.append(_T("	[F4]	Cycle Sampler State - (POINT/LINEAR/ANISOTROPIC)"));
	PrintMessage(hardwareMsg, MSG_LOGGER_HARDWARERASTERIZER, MSG_COLOR_HARDWARERASTERIZER, COLOR_GRAY);

	TSTRING softwareMsg{};
	softwareMsg.append(_T("[Key Bindings]\n"));
	softwareMsg.append(_T("	[F5]	Cycle Shading Mode - (COMBINED/OBSERVED_AREA/DIFFUSE/SPECULAR)\n"));
	softwareMsg.append(_T("	[F6]	Toggle Normal Map - (ON/OFF)\n"));
	softwareMsg.append(_T("	[F7]	Toggle DepthBuffer Visualization - (ON/OFF)\n"));
	softwareMsg.append(_T("	[F8]	Toggle BoundingBox Visualization - (ON/OFF)"));
	PrintMessage(softwareMsg, MSG_LOGGER_SOFTWARERASTERIZER, MSG_COLOR_SOFTWARERASTERIZER, COLOR_GRAY);

	PrintTstring(_T(""), _T("[Extra Features]"), MSG_COLOR_RENDERER);
	PrintTstring(_T("Texture Blending"), MSG_LOGGER_SOFTWARERASTERIZER, MSG_COLOR_SOFTWARERASTERIZER, COLOR_GRAY);
}