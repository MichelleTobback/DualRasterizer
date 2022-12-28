#pragma once
#include "Camera.h"

#include <memory>

namespace dae
{
	struct Mesh;

	class Scene
	{
	public:
		Scene();
		virtual ~Scene();

		Scene(const Scene&) = delete;
		Scene(Scene&&) noexcept = delete;
		Scene& operator=(const Scene&) = delete;
		Scene& operator=(Scene&&) noexcept = delete;

		virtual void Initialize() = 0;
		virtual void Update(dae::Timer* pTimer)
		{
			if (m_pCamera)
				m_pCamera.get()->Update(pTimer);
		}

		void AddMesh(const Mesh& mesh);

		inline const Camera& GetCamera() const { return *m_pCamera.get(); }

	protected:
		std::vector<std::unique_ptr<Mesh>> m_pMeshes;
		std::unique_ptr<Camera> m_pCamera;

		friend class Renderer;
		friend class HardwareRasterizerDX11;
		friend class SoftwareRasterizer;
	};

	class ExamScene : public Scene
	{
	public:
		ExamScene();
		virtual ~ExamScene() override;

		ExamScene(const ExamScene&) = delete;
		ExamScene(ExamScene&&) noexcept = delete;
		ExamScene& operator=(const ExamScene&) = delete;
		ExamScene& operator=(ExamScene&&) noexcept = delete;

		virtual void Initialize() override;
		virtual void Update(dae::Timer* pTimer) override;
	};
}
