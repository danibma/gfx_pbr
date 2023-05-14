#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <imgui.h>

Camera CreateCamera(GfxContext gfx, const glm::vec3& eye, const glm::vec3& center)
{
	Camera fly_camera = {};

	float const aspect_ratio = gfxGetBackBufferWidth(gfx) / (float)gfxGetBackBufferHeight(gfx);

	fly_camera.eye = eye;
	fly_camera.center = center;
	fly_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);

	fly_camera.view = glm::lookAt(eye, center, fly_camera.up);
	fly_camera.proj = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);
	fly_camera.view_proj = fly_camera.proj * fly_camera.view;

	fly_camera.prev_view = fly_camera.view;
	fly_camera.prev_proj = fly_camera.proj;
	fly_camera.prev_view_proj = fly_camera.view_proj;

	return fly_camera;
}

void UpdateCamera(GfxContext gfx, GfxWindow window, Camera& fly_camera, float deltaTime)
{
	// Update camera history
	fly_camera.prev_view = fly_camera.view;
	fly_camera.prev_proj = fly_camera.proj;

	// Update projection aspect ratio
	float const aspect_ratio = gfxGetBackBufferWidth(gfx) / (float)gfxGetBackBufferHeight(gfx);

	fly_camera.proj = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);

	// Update view
	float cameraSpeed = 0.02f * deltaTime;

	// Keyboard input
	glm::vec3 rightDirection = glm::cross(fly_camera.center, fly_camera.up);
	if (gfxWindowIsKeyDown(window, 0x57)) // W
		fly_camera.eye += fly_camera.center * cameraSpeed;
	if (gfxWindowIsKeyDown(window, 0x53)) // S
		fly_camera.eye -= fly_camera.center * cameraSpeed;
	if (gfxWindowIsKeyDown(window, 0x41)) // A
		fly_camera.eye += rightDirection * cameraSpeed;
	if (gfxWindowIsKeyDown(window, 0x44)) // D
		fly_camera.eye -= rightDirection * cameraSpeed;
	if (gfxWindowIsKeyDown(window, 0x41)) // A
		fly_camera.eye += rightDirection * cameraSpeed;
	if (gfxWindowIsKeyDown(window, VK_SHIFT)) // Shift
		fly_camera.eye -= fly_camera.up * cameraSpeed;
	if (gfxWindowIsKeyDown(window, VK_SPACE)) // Spacebar
		fly_camera.eye += fly_camera.up * cameraSpeed;

	// mouse input
	glm::vec2 mousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
	glm::vec2 delta = (mousePos - fly_camera.last_mouse_pos) * 0.002f;
	fly_camera.last_mouse_pos = mousePos;

	if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		// Rotation
		if (delta.x != 0.0f || delta.y != 0.0f)
		{
			const float rotation_speed = 1.5f;
			float pitchDelta = delta.y * rotation_speed;
			float yawDelta = -delta.x * rotation_speed;

			glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, rightDirection),
													glm::angleAxis(-yawDelta, glm::vec3(0.f, 1.0f, 0.0f))));
			fly_camera.center = glm::rotate(q, fly_camera.center);
		}
	}

	fly_camera.view = glm::lookAt(fly_camera.eye, fly_camera.eye + fly_camera.center, fly_camera.up);

	// Update view projection matrix
	fly_camera.view_proj = fly_camera.proj * fly_camera.view;
	fly_camera.prev_view_proj = fly_camera.prev_proj * fly_camera.prev_view;
}
