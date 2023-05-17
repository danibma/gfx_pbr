#include <gfx_window.h>
#include <gfx_imgui.h>
#include <gfx_scene.h>
#include <chrono>
#include "timer.h"
#include "camera.h"
#include "gpu_shared.h"

#include "imgui_demo.cpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <filesystem>

#include "stb_image.h"

struct GPUMesh
{
	uint32_t index_count;

	glm::mat4 transform = glm::mat4(1.0f);

	GPUMaterial material;
	GfxBuffer vertex_buffer, index_buffer;
};

GfxTexture gfxLoadTexture2D(GfxContext gfx, std::filesystem::path path)
{
	void* data;
	int width, height, num_channels, bytes_per_channel;
	DXGI_FORMAT format;
	if (path.extension() == ".hdr") 
	{
		data = stbi_loadf(path.string().c_str(), &width, &height, &num_channels, 4);
		GFX_ASSERT(data);
		format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		bytes_per_channel = 4;
	}
	else
	{
		data = stbi_load(path.string().c_str(), &width, &height, &num_channels, 4);
		GFX_ASSERT(data);
		if (stbi_is_16_bit(path.string().c_str()))
		{
			bytes_per_channel = 2;
			format = DXGI_FORMAT_R16G16B16A16_UNORM;
		}
		else
		{
			bytes_per_channel = 1;
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	num_channels = num_channels != 4 ? 4 : num_channels;

	bool generate_mips = true;
	if (path.extension() != ".hdr") 
		generate_mips = false;

	GfxTexture texture = gfxCreateTexture2D(gfx, width, height, format, generate_mips ? gfxCalculateMipCount(width, height) : 1);

	uint32_t const texture_size = width * height * num_channels * bytes_per_channel;

	GfxBuffer upload_texture_buffer = gfxCreateBuffer(gfx, texture_size, data, kGfxCpuAccess_Write);

	gfxCommandCopyBufferToTexture(gfx, texture, upload_texture_buffer);
	gfxDestroyBuffer(gfx, upload_texture_buffer);
	if (generate_mips)
		gfxCommandGenerateMips(gfx, texture);

	stbi_image_free(data);

	return texture;
}

int main()
{
	auto window = gfxCreateWindow(1280, 720, "gfx_pbr");

	GfxCreateContextFlags ctxFlags = 0;
#if _DEBUG
	ctxFlags |= kGfxCreateContextFlag_EnableDebugLayer;
#endif
	GfxContext gfx = gfxCreateContext(window, ctxFlags);
	GfxScene scene = gfxCreateScene();
	gfxImGuiInitialize(gfx);

	GfxSamplerState linear_clamp_sampler = gfxCreateSamplerState(gfx, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	GfxSamplerState linear_wrap_sampler = gfxCreateSamplerState(gfx, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	//gfxSceneImport(scene, "assets/flying_world_battle_of_the_trash_god/FlyingWorld-BattleOfTheTrashGod.gltf");
	gfxSceneImport(scene, "assets/sphere/sphere.gltf");
	gfxSceneImport(scene, "assets/skybox.obj");
	const uint32_t instancesCount = gfxSceneGetInstanceCount(scene);
	const GfxInstance* instances  = gfxSceneGetInstances(scene);

	// Load skybox stuff
	GfxTexture environment_map = gfxLoadTexture2D(gfx, "assets/newport_loft.hdr");
	const GfxConstRef<GfxMesh>& skybox_handle = gfxSceneFindObjectByAssetFile<GfxMesh>(scene, "assets/skybox.obj");
	GPUMesh skybox_mesh = {};
	skybox_mesh.index_count = static_cast<uint32_t>(skybox_handle->indices.size());
	skybox_mesh.vertex_buffer = gfxCreateBuffer(gfx, sizeof(GfxVertex) * skybox_handle->vertices.size(), skybox_handle->vertices.data());
	skybox_mesh.index_buffer = gfxCreateBuffer(gfx, sizeof(uint32_t) * skybox_handle->indices.size(), skybox_handle->indices.data());

	// Send mesh data to the gpu
	std::vector<GPUMesh> gpu_meshes(instancesCount - 1);
	for (uint32_t i = 0; i < instancesCount - 1; ++i)
	{
		GfxInstance instance = instances[i];
		const GfxConstRef<GfxMesh>& mesh = instance.mesh;
		GPUMesh& gpu_mesh = gpu_meshes[i];

		gpu_mesh.material = {};
		if (mesh->material)
		{
			gpu_mesh.material.albedo    = mesh->material->albedo;
			gpu_mesh.material.roughness = mesh->material->roughness;
			gpu_mesh.material.metallic  = mesh->material->metallicity;
		}
		else
		{
			gpu_mesh.material.albedo    = float4(1.0f);
			gpu_mesh.material.roughness = 1.0f;
			gpu_mesh.material.metallic  = 0.0f;
		}
		

		gpu_mesh.transform	   = instance.transform;
		gpu_mesh.index_count   = static_cast<uint32_t>(mesh->indices.size());
		gpu_mesh.vertex_buffer = gfxCreateBuffer(gfx, sizeof(GfxVertex) * mesh->vertices.size(), mesh->vertices.data());
		gpu_mesh.index_buffer  = gfxCreateBuffer(gfx, sizeof(uint32_t) * mesh->indices.size(), mesh->indices.data());
	}

	float vertices[] = {  0.5f, -0.5f, 0.0f,
						  0.0f,  0.7f, 0.0f,
						 -0.5f, -0.5f, 0.0f };
	auto vertex_buffer = gfxCreateBuffer(gfx, sizeof(vertices), vertices);

	// Deferred shading buffers
	GfxTexture world_pos_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT); world_pos_buffer.setName("world_pos_buffer");
	GfxTexture albedo_buffer	= gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT); albedo_buffer.setName("albedo_buffer");
	GfxTexture normal_buffer	= gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT); normal_buffer.setName("normal_buffer");
	GfxTexture metallic_buffer  = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT); metallic_buffer.setName("metallic_buffer");
	GfxTexture roughness_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT); roughness_buffer.setName("roughness_buffer");
	GfxTexture depth_buffer		= gfxCreateTexture2D(gfx, DXGI_FORMAT_D32_FLOAT);          

	GfxDrawState deferredShadingDrawState;
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 0, world_pos_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 1, albedo_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 2, normal_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 3, metallic_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 4, roughness_buffer);
	gfxDrawStateSetDepthStencilTarget(deferredShadingDrawState, depth_buffer);

	GfxProgram deferredShadingProgram = gfxCreateProgram(gfx, "shaders/deferred_shading");
	GfxKernel deferredShadingKernel = gfxCreateGraphicsKernel(gfx, deferredShadingProgram, deferredShadingDrawState);

	GfxTexture pbr_color_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GfxTexture pbr_depth_buffer = gfxCreateTexture2D(gfx, DXGI_FORMAT_D32_FLOAT);
	GfxDrawState pbrDrawState;
	gfxDrawStateSetColorTarget(pbrDrawState, 0, pbr_color_buffer);
	gfxDrawStateSetDepthStencilTarget(pbrDrawState, pbr_depth_buffer);
	GfxProgram PBRProgram = gfxCreateProgram(gfx, "shaders/pbr_lighting");
	GfxKernel PBRKernel = gfxCreateGraphicsKernel(gfx, PBRProgram, pbrDrawState);

	// Skybox program
	GfxDrawState sky_draw_state;
	gfxDrawStateSetColorTarget(sky_draw_state, 0, pbr_color_buffer);
	GfxProgram sky_program = gfxCreateProgram(gfx, "shaders/sky");
	GfxKernel  sky_kernel = gfxCreateGraphicsKernel(gfx, sky_program, sky_draw_state);
	GfxKernel  sky_cube_kernel = gfxCreateComputeKernel(gfx, sky_program);
	GfxTexture environment_cube = gfxCreateTextureCube(gfx, 128, DXGI_FORMAT_R16G16B16A16_FLOAT, 5);
	gfxCommandBindKernel(gfx, sky_cube_kernel);
	gfxProgramSetParameter(gfx, sky_program, "inputTexture", environment_map);
	gfxProgramSetParameter(gfx, sky_program, "outputTexture", environment_cube);
	gfxProgramSetParameter(gfx, sky_program, "defaultSampler", linear_wrap_sampler);
	gfxCommandDispatch(gfx, environment_map.getWidth() / 32, environment_map.getHeight() / 32, 6);

	GfxProgram ibl_program = gfxCreateProgram(gfx, "shaders/ibl");
	// irradiance map
	GfxKernel irradiance_kernel = gfxCreateComputeKernel(gfx, ibl_program, "DrawIrradianceMap");
	GfxTexture irradiance_map = gfxCreateTextureCube(gfx, 32, DXGI_FORMAT_R16G16B16A16_FLOAT, 5);
	gfxCommandBindKernel(gfx, irradiance_kernel);
	gfxProgramSetParameter(gfx, ibl_program, "g_OriginalEnvironmentMap", environment_cube);
	gfxProgramSetParameter(gfx, ibl_program, "g_IrradianceMap", irradiance_map);
	gfxProgramSetParameter(gfx, ibl_program, "LinearWrap", linear_wrap_sampler);
	gfxCommandDispatch(gfx, environment_cube.getWidth() / 32, environment_cube.getHeight() / 32, 6);

	GfxProgram compositeProgram = gfxCreateProgram(gfx, "shaders/scene_composite");
	GfxKernel compositeKernel   = gfxCreateGraphicsKernel(gfx, compositeProgram);

	Camera camera = CreateCamera(gfx, glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f));

	// Debug views
	std::array<const char*, 3> debug_views = { "Full", "Color", "Normal" };
	int selected_debug_view = 0;

	glm::vec3 light_position(-1.0f, 1.0f, 2.0f);
	glm::vec3 light_color(1.0f);

	Timer deltaTimer;
	for (float time = 0.0f; !gfxWindowIsCloseRequested(window); time += 0.1f)
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		gfxWindowPumpEvents(window);

		UpdateCamera(gfx, window, camera, deltaTime);

		if (ImGui::Begin("Debug"))
		{
			ImGui::Text("CPU: %.2fms(%.0fFPS)", deltaTime, 1000.0f / deltaTime);

			ImGui::Separator();
			ImGui::Text("Rendering");
			if (ImGui::Button("Reload kernels"))
				gfxKernelReloadAll(gfx);

			ImGui::Separator();
			ImGui::Text("Debug Views");
			const char* combo_preview_value = debug_views[selected_debug_view];
			if (ImGui::BeginCombo("##debug_view", combo_preview_value))
			{
				for (int i = 0; i < debug_views.size(); i++)
				{
					const bool is_selected = (selected_debug_view == i);
					if (ImGui::Selectable(debug_views[i], is_selected))
						selected_debug_view = i;

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// Light
			ImGui::Separator();
			ImGui::Text("Light");
			ImGui::DragFloat3("Position", glm::value_ptr(light_position));
			ImGui::DragFloat3("Color", glm::value_ptr(light_color), 0.1f, 0.0f, 1.0f);

			// Sphere Material
			ImGui::Separator();
			ImGui::Text("Sphere Material");
			ImGui::ColorEdit4("Albedo", glm::value_ptr(gpu_meshes[0].material.albedo));
			ImGui::DragFloat("Metallic", &gpu_meshes[0].material.metallic, 0.05f, 0.0f, 1.0f);
			ImGui::DragFloat("Roughness", &gpu_meshes[0].material.roughness, 0.05f, 0.0f, 1.0f);
			
			//ImGui::ShowDemoWindow();
		}
		ImGui::End();

		// Render geometry
		gfxCommandBeginEvent(gfx, "Geometry Pass");
		// Clear deferred render targets
		gfxCommandClearTexture(gfx, world_pos_buffer);
		gfxCommandClearTexture(gfx, albedo_buffer);
		gfxCommandClearTexture(gfx, normal_buffer);
		gfxCommandClearTexture(gfx, metallic_buffer);
		gfxCommandClearTexture(gfx, roughness_buffer);
		gfxCommandClearTexture(gfx, depth_buffer);

		gfxCommandBindKernel(gfx, deferredShadingKernel);
		gfxProgramSetParameter(gfx, deferredShadingProgram, "view_proj", camera.view_proj);
		for (const GPUMesh& mesh : gpu_meshes)
		{
			gfxProgramSetParameter(gfx, deferredShadingProgram, "g_Material", mesh.material);
			gfxProgramSetParameter(gfx, deferredShadingProgram, "model", mesh.transform);
			gfxCommandBindVertexBuffer(gfx, mesh.vertex_buffer);
			gfxCommandBindIndexBuffer(gfx, mesh.index_buffer);
			gfxCommandDrawIndexed(gfx, mesh.index_count);
		}
		gfxCommandEndEvent(gfx);

		// Clear PBR render targets
		gfxCommandClearTexture(gfx, pbr_color_buffer);
		gfxCommandClearTexture(gfx, pbr_depth_buffer);

		// Render sky
		gfxCommandBeginEvent(gfx, "Sky");
		gfxProgramSetParameter(gfx, sky_program, "view", camera.view);
		gfxProgramSetParameter(gfx, sky_program, "proj", camera.proj);
		gfxProgramSetParameter(gfx, sky_program, "g_EnvironmentCube", irradiance_map);
		gfxProgramSetParameter(gfx, sky_program, "LinearWrap", linear_clamp_sampler);
		gfxCommandBindKernel(gfx, sky_kernel);
		gfxCommandBindVertexBuffer(gfx, skybox_mesh.vertex_buffer);
		gfxCommandBindIndexBuffer(gfx, skybox_mesh.index_buffer);
		gfxCommandDrawIndexed(gfx, skybox_mesh.index_count);
		gfxCommandEndEvent(gfx);

		// PBR lighting
		gfxCommandBeginEvent(gfx, "PBR Lighting Pass");
		
		gfxCommandBindKernel(gfx, PBRKernel);
		// PBR scene info
		gfxProgramSetParameter(gfx, PBRProgram, "camPos", camera.eye);
		gfxProgramSetParameter(gfx, PBRProgram, "lightPos", light_position);
		gfxProgramSetParameter(gfx, PBRProgram, "lightColor", light_color);

		gfxProgramSetParameter(gfx, PBRProgram, "TextureSampler", linear_clamp_sampler);
		// Bind deferred shading render targets
		gfxProgramSetParameter(gfx, PBRProgram, "g_WorldPosition", world_pos_buffer);
		gfxProgramSetParameter(gfx, PBRProgram, "g_Albedo", albedo_buffer);
		gfxProgramSetParameter(gfx, PBRProgram, "g_Normal", normal_buffer);
		gfxProgramSetParameter(gfx, PBRProgram, "g_Metallic", metallic_buffer);
		gfxProgramSetParameter(gfx, PBRProgram, "g_Roughness", roughness_buffer);
		// Bind irradiance map
		gfxProgramSetParameter(gfx, PBRProgram, "g_IrradianceMap", irradiance_map);

		gfxCommandDraw(gfx, 6);
		gfxCommandEndEvent(gfx);

		// render scene composite
		GfxTexture scene_texture;
		if (selected_debug_view == 1)
			scene_texture = albedo_buffer;
		else if (selected_debug_view == 2)
			scene_texture = normal_buffer;
		else
			scene_texture = pbr_color_buffer;

		gfxCommandBeginEvent(gfx, "Scene Composite");
		gfxProgramSetParameter(gfx, compositeProgram, "g_SceneTexture", scene_texture);
		gfxProgramSetParameter(gfx, compositeProgram, "TextureSampler", linear_clamp_sampler);
		gfxCommandBindKernel(gfx, compositeKernel);
		gfxCommandDraw(gfx, 6);
		gfxCommandEndEvent(gfx);

		gfxImGuiRender();
		gfxFrame(gfx);
	}

	gfxImGuiTerminate();
	gfxDestroyContext(gfx);
	gfxDestroyWindow(window);

	return 0;
}