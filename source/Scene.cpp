#include "pch.h"
#include "Scene.h"
#include "DataTypes.h"
#include "HardwareRasterizerDX11.h"
#include "Effect.h"
#include "ResourceManager.h"

namespace dae
{
	//==========================//
	// base scene
	//==========================//

	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

	void Scene::AddMesh(Mesh* mesh)
	{
		m_pMeshes.push_back(std::unique_ptr<Mesh>(mesh));
	}

	//==========================//
	// exam scene
	//==========================//

	ExamScene::ExamScene()
	{
		
	}

	ExamScene::~ExamScene()
	{

	}

	void ExamScene::Initialize(float windowWidth, float windowHeight)
	{
		//setup camera
		m_pCamera = std::make_unique<Camera>();
		m_pCamera->Initialize(45.f, { 0.f, 0.f, 0.f }, windowWidth / windowHeight);
		m_pCamera->nearClip = 0.1f;
		m_pCamera->farClip = 100.f;
		
		const auto& pDevice{ HardwareRasterizerDX11::GetDevice() };

		//create textures
		auto diffuseMap{ ResourceManager::AddTexture("Resources/vehicle_diffuse.png") };
		auto normalMap{ ResourceManager::AddTexture("Resources/vehicle_normal.png") };
		auto specularMap{ ResourceManager::AddTexture("Resources/vehicle_specular.png") };
		auto glossinessMap{ ResourceManager::AddTexture("Resources/vehicle_gloss.png") };
		auto fireFxMap{ ResourceManager::AddTexture("Resources/fireFX_diffuse.png") };

		//create effects for dx11
		auto lambertPhongEffect{ HardwareRasterizerDX11::AddEffect(new PosTexEffect(pDevice, L"Resources/PosTex3D.fx")) };
		auto flatShaderEffect{ HardwareRasterizerDX11::AddEffect(new FlatEffect(pDevice, L"Resources/Flat.fx")) };

		//create materials
		Material vechicleMaterial{};
		vechicleMaterial.shaderId = lambertPhongEffect;
		vechicleMaterial.textures.reserve(4);
		vechicleMaterial.textures.emplace_back(diffuseMap);
		vechicleMaterial.textures.emplace_back(normalMap);
		vechicleMaterial.textures.emplace_back(specularMap);
		vechicleMaterial.textures.emplace_back(glossinessMap);
		auto vehicleMatId{ ResourceManager::AddMaterial(vechicleMaterial) };

		Material fireFxMaterial{};
		fireFxMaterial.shaderId = flatShaderEffect;
		fireFxMaterial.textures.push_back(fireFxMap);
		auto fireFxeMatId{ ResourceManager::AddMaterial(fireFxMaterial) };

		//create meshes
		auto pVehicleMesh{ MeshDX11::CreateFromFile(pDevice, "Resources/vehicle.obj", vehicleMatId) };
		pVehicleMesh->worldMatrix = Matrix::CreateTranslation(0.f, 0.f, 50.f);
		AddMesh(pVehicleMesh);

		auto pFireFxMesh{ MeshDX11::CreateFromFile(pDevice, "Resources/fireFX.obj", fireFxeMatId) };
		pFireFxMesh->worldMatrix = Matrix::CreateTranslation(0.f, 0.f, 50.f);
		AddMesh(pFireFxMesh);
	}

	void ExamScene::Update(dae::Timer* pTimer)
	{
		Scene::Update(pTimer);

		if (m_Rotate)
		{
			constexpr const float rotationSpeed{ 45.f };
			m_pMeshes[0]->RotateY(rotationSpeed * pTimer->GetElapsed());
			m_pMeshes[1]->RotateY(rotationSpeed * pTimer->GetElapsed());
		}
	}
	void ExamScene::KeyDownEvent(SDL_KeyboardEvent e)
	{
		switch (e.keysym.scancode)
		{
		case SDL_SCANCODE_F2:
		{
			//toggle rotation
			m_Rotate = !m_Rotate;
		}
		break;
		}
	}
}