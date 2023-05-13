#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <gfx_scene.h>
#include <gfx_window.h>

struct Camera
{
	glm::vec2 last_mouse_pos;

	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;

	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 view_proj;

	glm::mat4 prev_view;
	glm::mat4 prev_proj;
	glm::mat4 prev_view_proj;
};

Camera CreateCamera(GfxContext gfx, const glm::vec3& eye, const glm::vec3& center);
void UpdateCamera(GfxContext gfx, GfxWindow window, Camera& fly_camera, float deltaTime);
