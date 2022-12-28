#include "pch.h"
#include "Scene.h"
#include "DataTypes.h"
#include "HardwareRasterizerDX11.h"
#include "Effect.h"

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

	void Scene::AddMesh(const Mesh& mesh)
	{
		m_pMeshes.push_back(std::make_unique<Mesh>(mesh));
	}

	//==========================//
	// exam scene
	//==========================//

	ExamScene::ExamScene()
	{
		m_pCamera = std::make_unique<Camera>();

		const auto& pDevice{ HardwareRasterizerDX11::GetDevice() };
		AddMesh(MeshDX11::CreateFromFile(pDevice, "Resources/vehicle.obj", new PosTexEffect(pDevice, L"Resources/PosTex3D.fx")));
	}

	ExamScene::~ExamScene()
	{

	}

	void ExamScene::Initialize()
	{

	}

	void ExamScene::Update(dae::Timer* pTimer)
	{
		Scene::Update(pTimer);
	}
}