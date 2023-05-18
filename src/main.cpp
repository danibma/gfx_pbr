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
	GPUTexturedMaterial textured_material;
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

	//gfxSceneImport(scene, "assets/models/flying_world_battle_of_the_trash_god/FlyingWorld-BattleOfTheTrashGod.gltf");
	//gfxSceneImport(scene, "assets/models/sphere/sphere.gltf");
	gfxSceneImport(scene, "assets/models/cerberus/scene.gltf");
	//gfxSceneImport(scene, "assets/models/DamagedHelmet/DamagedHelmet.gltf");
	gfxSceneImport(scene, "assets/models/skybox.obj");
	const uint32_t instancesCount = gfxSceneGetInstanceCount(scene);
	const GfxInstance* instances  = gfxSceneGetInstances(scene);

	// Load skybox stuff
	GfxTexture environment_map = gfxLoadTexture2D(gfx, "assets/environment/rotes_rathaus_4k.hdr");
	const GfxConstRef<GfxMesh>& skybox_handle = gfxSceneFindObjectByAssetFile<GfxMesh>(scene, "assets/models/skybox.obj");
	GPUMesh skybox_mesh = {};
	skybox_mesh.index_count = static_cast<uint32_t>(skybox_handle->indices.size());
	skybox_mesh.vertex_buffer = gfxCreateBuffer(gfx, sizeof(GfxVertex) * skybox_handle->vertices.size(), skybox_handle->vertices.data());
	skybox_mesh.index_buffer = gfxCreateBuffer(gfx, sizeof(uint32_t) * skybox_handle->indices.size(), skybox_handle->indices.data());

	GfxTexture empty_texture;
	{
		empty_texture = gfxCreateTexture2D(gfx, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1);

		uint32_t const texture_size = 4;

		uint32_t data = 0x00000000;
		GfxBuffer upload_texture_buffer = gfxCreateBuffer(gfx, texture_size, &data, kGfxCpuAccess_Write);

		gfxCommandCopyBufferToTexture(gfx, empty_texture, upload_texture_buffer);
		gfxDestroyBuffer(gfx, upload_texture_buffer);
	}

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
			auto load_texture = [gfx, empty_texture](GfxConstRef<GfxImage> image_ref) -> GfxTexture
			{
				if (image_ref)
				{
					GfxTexture texture = gfxCreateTexture2D(gfx, image_ref->width, image_ref->height, image_ref->format, gfxCalculateMipCount(image_ref->width, image_ref->height));

					uint32_t const texture_size = image_ref->width * image_ref->height * image_ref->channel_count * image_ref->bytes_per_channel;

					GfxBuffer upload_texture_buffer = gfxCreateBuffer(gfx, texture_size, image_ref->data.data(), kGfxCpuAccess_Write);

					gfxCommandCopyBufferToTexture(gfx, texture, upload_texture_buffer);
					gfxDestroyBuffer(gfx, upload_texture_buffer);
					gfxCommandGenerateMips(gfx, texture);

					return texture;
				}
				else
				{
					return empty_texture;
				}
			};

			GfxMaterial mat = *mesh->material;

			gpu_mesh.material.albedo    = mesh->material->albedo;
			gpu_mesh.material.roughness = mesh->material->roughness;
			gpu_mesh.material.metallic  = mesh->material->metallicity;

			gpu_mesh.textured_material.albedo_texture = load_texture(mesh->material->albedo_map);
			gpu_mesh.textured_material.metallic_texture = load_texture(mesh->material->metallicity_map);
			gpu_mesh.textured_material.roughness_texture = load_texture(mesh->material->roughness_map);
			gpu_mesh.textured_material.emissive_texture = load_texture(mesh->material->emissivity_map);
		}
		else
		{
			gpu_mesh.material.albedo    = float4(1.0f);
			gpu_mesh.material.roughness = 1.0f;
			gpu_mesh.material.metallic  = 1.0f;
		}

#if 0 // for the sphere
		gpu_mesh.textured_material.albedo_texture = gfxLoadTexture2D(gfx, "assets/textures/rusted_iron/albedo.png");
		gpu_mesh.textured_material.metallic_texture = gfxLoadTexture2D(gfx, "assets/textures/rusted_iron/metallic.png");
		gpu_mesh.textured_material.roughness_texture = gfxLoadTexture2D(gfx, "assets/textures/rusted_iron/roughness.png");
#endif

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
	GfxTexture emissive_buffer  = gfxCreateTexture2D(gfx, DXGI_FORMAT_R16G16B16A16_FLOAT); emissive_buffer.setName("emissive_buffer");
	GfxTexture depth_buffer		= gfxCreateTexture2D(gfx, DXGI_FORMAT_D32_FLOAT);          

	GfxDrawState deferredShadingDrawState;
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 0, world_pos_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 1, albedo_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 2, normal_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 3, metallic_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 4, roughness_buffer);
	gfxDrawStateSetColorTarget(deferredShadingDrawState, 5, emissive_buffer);
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

	// IBL stuff
	GfxProgram ibl_program = gfxCreateProgram(gfx, "shaders/ibl");

	GfxTexture environment_cube = gfxCreateTextureCube(gfx, 1024, DXGI_FORMAT_R16G16B16A16_FLOAT, 1);
	GfxTexture irradiance_map = gfxCreateTextureCube(gfx, 128, DXGI_FORMAT_R16G16B16A16_FLOAT, 1);
	GfxTexture prefilter_map = gfxCreateTextureCube(gfx, 1024, DXGI_FORMAT_R16G16B16A16_FLOAT, 5);
	GfxTexture brdf_lut_map = gfxCreateTexture2D(gfx, 256, 256, DXGI_FORMAT_R16G16_FLOAT, 1);

	// Transform equirectangular map to cubemap
	{
		GfxKernel equirect_to_cubemap_kernel = gfxCreateComputeKernel(gfx, ibl_program, "EquirectToCubemap");
		gfxCommandBindKernel(gfx, equirect_to_cubemap_kernel);
		gfxProgramSetParameter(gfx, ibl_program, "g_EnvironmentEquirectangular", environment_map);
		gfxProgramSetParameter(gfx, ibl_program, "g_OutEnvironmentCubemap", environment_cube);
		gfxProgramSetParameter(gfx, ibl_program, "LinearWrap", linear_wrap_sampler);
		gfxCommandDispatch(gfx, environment_cube.getWidth() / 32, environment_cube.getHeight() / 32, 6);
	}

	// irradiance map
	{
		GfxKernel irradiance_kernel = gfxCreateComputeKernel(gfx, ibl_program, "DrawIrradianceMap");
		gfxCommandBindKernel(gfx, irradiance_kernel);
		gfxProgramSetParameter(gfx, ibl_program, "g_EnvironmentCubemap", environment_cube);
		gfxProgramSetParameter(gfx, ibl_program, "g_IrradianceMap", irradiance_map);
		gfxProgramSetParameter(gfx, ibl_program, "LinearWrap", linear_wrap_sampler);
		gfxCommandDispatch(gfx, irradiance_map.getWidth() / 32, irradiance_map.getHeight() / 32, 6);
	}

	// Pre filter env map
	{
		GfxKernel prefilter_kernel = gfxCreateComputeKernel(gfx, ibl_program, "PreFilterEnvMap");
		gfxCommandBindKernel(gfx, prefilter_kernel);
		gfxProgramSetParameter(gfx, ibl_program, "g_EnvironmentCubemap", environment_cube);
		gfxProgramSetParameter(gfx, ibl_program, "LinearWrap", linear_wrap_sampler);

		const float deltaRoughness = 1.0f / glm::max(float(prefilter_map.getMipLevels() - 1), 1.0f);
		for (uint32_t level = 0, size = prefilter_map.getWidth(); level < prefilter_map.getMipLevels(); ++level, size /= 2)
		{
			const uint32_t numGroups = glm::max<uint32_t>(1, size / 32);

			gfxProgramSetTexture(gfx, ibl_program, "g_PreFilteredMap", prefilter_map, level);
			gfxProgramSetParameter(gfx, ibl_program, "roughness", level * deltaRoughness);
			gfxCommandDispatch(gfx, numGroups, numGroups, 6);
		}
	}

	// Compute BRDF LUT map
	{
		GfxKernel brdf_lut_kernel = gfxCreateComputeKernel(gfx, ibl_program, "ComputeBRDFLUT");
		gfxCommandBindKernel(gfx, brdf_lut_kernel);
		gfxProgramSetParameter(gfx, ibl_program, "LUT", brdf_lut_map);
		gfxCommandDispatch(gfx, brdf_lut_map.getWidth() / 32, brdf_lut_map.getHeight() / 32, 6);
	}

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
		gfxCommandClearTexture(gfx, emissive_buffer);
		gfxCommandClearTexture(gfx, depth_buffer);

		gfxCommandBindKernel(gfx, deferredShadingKernel);
		gfxProgramSetParameter(gfx, deferredShadingProgram, "view_proj", camera.view_proj);
		gfxProgramSetParameter(gfx, deferredShadingProgram, "LinearWrap", linear_wrap_sampler);
		for (const GPUMesh& mesh : gpu_meshes)
		{
			gfxProgramSetParameter(gfx, deferredShadingProgram, "g_Material", mesh.material);
			gfxProgramSetParameter(gfx, deferredShadingProgram, "albedo_texture", mesh.textured_material.albedo_texture);
			gfxProgramSetParameter(gfx, deferredShadingProgram, "metallic_texture", mesh.textured_material.metallic_texture);
			gfxProgramSetParameter(gfx, deferredShadingProgram, "roughness_texture", mesh.textured_material.roughness_texture);
			gfxProgramSetParameter(gfx, deferredShadingProgram, "emissive_texture", mesh.textured_material.emissive_texture);
			gfxProgramSetParameter(gfx, deferredShadingProgram, "model", glm::scale(mesh.transform, glm::vec3(0.3f)));
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
		gfxProgramSetParameter(gfx, sky_program, "g_EnvironmentCube", environment_cube);
		gfxProgramSetParameter(gfx, sky_program, "LinearWrap", linear_wrap_sampler);
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
		gfxProgramSetParameter(gfx, PBRProgram, "g_Emissive", emissive_buffer);
		// Bind irradiance map
		gfxProgramSetParameter(gfx, PBRProgram, "g_IrradianceMap", irradiance_map);
		gfxProgramSetParameter(gfx, PBRProgram, "g_PrefilterMap", prefilter_map);
		gfxProgramSetParameter(gfx, PBRProgram, "g_LUT", brdf_lut_map);

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