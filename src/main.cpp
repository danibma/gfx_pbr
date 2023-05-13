#include <gfx_window.h>
#include <gfx_imgui.h>
#include <gfx_scene.h>
#include <chrono>
#include "timer.h"
#include "camera.h"

#include <glm/glm.hpp>

int main()
{
	auto window = gfxCreateWindow(1280, 720, "gfx - Hello, triangle!");

	GfxCreateContextFlags ctxFlags = 0;
#if _DEBUG
	ctxFlags |= kGfxCreateContextFlag_EnableDebugLayer;
#endif
	auto gfx = gfxCreateContext(window, ctxFlags);
	gfxImGuiInitialize(gfx);

	float vertices[] = {  0.5f, -0.5f, 0.0f,
						  0.0f,  0.7f, 0.0f,
						 -0.5f, -0.5f, 0.0f };
	auto vertex_buffer = gfxCreateBuffer(gfx, sizeof(vertices), vertices);

	GfxTexture color_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GfxTexture depth_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_D32_FLOAT);

	GfxDrawState pbr_draw_state;
	gfxDrawStateSetColorTarget(pbr_draw_state, 0, color_buffer);
	gfxDrawStateSetDepthStencilTarget(pbr_draw_state, depth_buffer);

	auto program = gfxCreateProgram(gfx, "shaders/triangle");
	auto kernel = gfxCreateGraphicsKernel(gfx, program, pbr_draw_state);

	GfxProgram compositeProgram = gfxCreateProgram(gfx, "shaders/scene_composite");
	GfxKernel compositeKernel   = gfxCreateGraphicsKernel(gfx, compositeProgram);

	GfxSamplerState textureSampler = gfxCreateSamplerState(gfx, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	Camera camera = CreateCamera(gfx, glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	Timer deltaTimer;
	for (float time = 0.0f; !gfxWindowIsCloseRequested(window); time += 0.1f)
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		gfxWindowPumpEvents(window);

		UpdateCamera(gfx, window, camera, deltaTime);

		// Clear our render targets
		gfxCommandClearTexture(gfx, color_buffer);
		gfxCommandClearTexture(gfx, depth_buffer);

		if (ImGui::Begin("Debug"))
		{
			ImGui::Text("CPU: %.2fms(%.0fFPS)", deltaTime, 1000.0f / deltaTime);
		}
		ImGui::End();

		float color[] = { 0.5f * cosf(time) + 0.5f,
						  0.5f * sinf(time) + 0.5f,
						  1.0f };
		gfxProgramSetParameter(gfx, program, "Color", color);
		gfxProgramSetParameter(gfx, program, "view_proj", camera.view_proj);
		gfxProgramSetParameter(gfx, program, "model", glm::mat4(1.0f));

		gfxCommandBindKernel(gfx, kernel);
		gfxCommandBindVertexBuffer(gfx, vertex_buffer);

		gfxCommandDraw(gfx, 3);

		gfxProgramSetParameter(gfx, compositeProgram, "g_SceneTexture", color_buffer);
		gfxProgramSetParameter(gfx, compositeProgram, "TextureSampler", textureSampler);
		gfxCommandBindKernel(gfx, compositeKernel);
		gfxCommandDraw(gfx, 6);

		gfxImGuiRender();
		gfxFrame(gfx);
	}

	gfxImGuiTerminate();
	gfxDestroyContext(gfx);
	gfxDestroyWindow(window);

	return 0;
}