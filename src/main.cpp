#include <gfx_window.h>
#include <gfx_imgui.h>
#include <gfx_scene.h>
#include <chrono>
#include "timer.h"
#include "camera.h"

#include <glm/glm.hpp>

struct GPUMesh
{
	uint32_t index_count;

	glm::mat4 transform = glm::mat4(1.0f);

	GfxConstRef<GfxMaterial> material;
	GfxBuffer vertex_buffer, index_buffer;
};

int main()
{
	auto window = gfxCreateWindow(1280, 720, "gfx - Hello, triangle!");

	GfxCreateContextFlags ctxFlags = 0;
#if _DEBUG
	ctxFlags |= kGfxCreateContextFlag_EnableDebugLayer;
#endif
	GfxContext gfx = gfxCreateContext(window, ctxFlags);
	GfxScene scene = gfxCreateScene();
	gfxImGuiInitialize(gfx);

	gfxSceneImport(scene, "assets/flying_world_battle_of_the_trash_god/FlyingWorld-BattleOfTheTrashGod.gltf");
	const uint32_t instancesCount = gfxSceneGetInstanceCount(scene);
	const GfxInstance* instances  = gfxSceneGetInstances(scene);

	// Send mesh data to the gpu
	std::vector<GPUMesh> gpu_meshes(instancesCount);
	for (uint32_t i = 0; i < instancesCount; ++i)
	{
		GfxInstance instance = instances[i];
		const GfxConstRef<GfxMesh>& mesh = instance.mesh;
		GPUMesh& gpu_mesh = gpu_meshes[i];

		gpu_mesh.transform	   = instance.transform;
		gpu_mesh.material      = mesh->material;
		gpu_mesh.index_count   = static_cast<uint32_t>(mesh->indices.size());
		gpu_mesh.vertex_buffer = gfxCreateBuffer(gfx, sizeof(GfxVertex) * mesh->vertices.size(), mesh->vertices.data());
		gpu_mesh.index_buffer  = gfxCreateBuffer(gfx, sizeof(uint32_t) * mesh->indices.size(), mesh->indices.data());
	}

	float vertices[] = {  0.5f, -0.5f, 0.0f,
						  0.0f,  0.7f, 0.0f,
						 -0.5f, -0.5f, 0.0f };
	auto vertex_buffer = gfxCreateBuffer(gfx, sizeof(vertices), vertices);

	GfxTexture color_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GfxTexture depth_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_D32_FLOAT);

	GfxDrawState pbr_draw_state;
	gfxDrawStateSetColorTarget(pbr_draw_state, 0, color_buffer);
	gfxDrawStateSetDepthStencilTarget(pbr_draw_state, depth_buffer);

	auto objectProgram = gfxCreateProgram(gfx, "shaders/object");
	auto objectKernel = gfxCreateGraphicsKernel(gfx, objectProgram, pbr_draw_state);

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

		// Render objects
		gfxProgramSetParameter(gfx, objectProgram, "view_proj", camera.view_proj);

		gfxCommandBindKernel(gfx, objectKernel);
		for (const GPUMesh& mesh : gpu_meshes)
		{
			gfxProgramSetParameter(gfx, objectProgram, "Color", mesh.material->albedo);
			gfxProgramSetParameter(gfx, objectProgram, "model", mesh.transform);
			gfxCommandBindVertexBuffer(gfx, mesh.vertex_buffer);
			gfxCommandBindIndexBuffer(gfx, mesh.index_buffer);
			gfxCommandDrawIndexed(gfx, mesh.index_count);
		}

		// render scene composite
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