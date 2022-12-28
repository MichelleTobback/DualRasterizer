#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
			CalculateViewMatrix();
		}

		Vector3 origin{};
		float fovAngle{ 90.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio{ 1.f };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix ProjectionMatrix{};

		float nearClip{ 0.1f };
		float farClip{ 100.f };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f })
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			CalculateViewMatrix();
		}

		void CalculateViewMatrix()
		{
			//ONB => invViewMatrix
			right = Vector3::Cross(Vector3::UnitY, forward);
			up = Vector3::Cross(forward, right);
			invViewMatrix = { right, up, forward, origin };

			//Inverse(ONB) => ViewMatrix
			viewMatrix = Matrix::Inverse(invViewMatrix);

			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			ProjectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearClip, farClip);

			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			//Camera Update Logic
			const float deltaTime = pTimer->GetElapsed();
			const float movementSpeed{ 8.f };
			const float rotSpeed{ PI_DIV_4 / 2.f };

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			//todo: W2
			bool hasMoved{ false };
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += (forward * movementSpeed * deltaTime);
				hasMoved = true;
			}
			else if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= (forward * movementSpeed * deltaTime);
				hasMoved = true;
			}

			if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += (right * movementSpeed * deltaTime);
				hasMoved = true;
			}
			else if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= (right * movementSpeed * deltaTime);
				hasMoved = true;
			}

			bool hasRotated{ false };
			if (mouseState & SDL_BUTTON(1) && mouseState & SDL_BUTTON(3))
			{
				origin -= (up * movementSpeed * deltaTime * float(mouseY));
				hasMoved = true;
			}
			else
			{
				if (mouseState & SDL_BUTTON(3))
				{
					totalYaw += float(mouseX) * rotSpeed * deltaTime;
					totalPitch -= float(mouseY) * rotSpeed * deltaTime;
					hasRotated = true;
					hasMoved = true;
				}
				else if (mouseState & SDL_BUTTON(1))
				{
					totalYaw += float(mouseX) * rotSpeed * deltaTime;
					origin += forward * -float(mouseY) * movementSpeed * deltaTime;
					hasRotated = true;
					hasMoved = true;
				}
			}

			if (totalPitch >= PI_2)
				totalPitch = 0.f;
			if (totalPitch < 0.f)
				totalPitch = PI_2;
			if (totalYaw >= PI_2)
				totalYaw = 0.f;
			if (totalYaw < 0.f)
				totalYaw = PI_2;

			if (hasRotated)
			{
				Matrix rotMat{ Matrix::CreateRotation(totalPitch, totalYaw, 0.f) };
				forward = rotMat.TransformVector(Vector3::UnitZ);
				forward.Normalize();
				up = rotMat.TransformVector(Vector3::UnitY);
				up.Normalize();
				right = rotMat.TransformVector(Vector3::UnitX);
				right.Normalize();
			}
			if (hasMoved)
			{
				CalculateViewMatrix();
			}
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
