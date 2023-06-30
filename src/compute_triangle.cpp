#include <gfx.h>
#include <gfx_window.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

int mains()
{
	auto window = gfxCreateWindow(1280, 720, "compute_triangle");

	GfxCreateContextFlags ctxFlags = 0;
#if _DEBUG
	ctxFlags |= kGfxCreateContextFlag_EnableDebugLayer;
#endif
	GfxContext gfx = gfxCreateContext(window, ctxFlags);

	GfxProgram program = gfxCreateProgram(gfx, "shaders/compute_triangle");
	GfxKernel kernel = gfxCreateComputeKernel(gfx, program, "DrawTriangle");
	const uint32_t* num_threads = gfxKernelGetNumThreads(gfx, kernel);
	const uint32_t num_groups_x = 1280 / num_threads[0];
	const uint32_t num_groups_y = 720 / num_threads[1];

	GfxTexture output_texture = gfxCreateTexture2D(gfx, 1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM);

	while (!gfxWindowIsCloseRequested(window))
	{
		gfxWindowPumpEvents(window);

		gfxCommandBindKernel(gfx, kernel);
		gfxProgramSetParameter(gfx, program, "output", output_texture);
		gfxCommandDispatch(gfx, num_groups_x, num_groups_y, 1);

		gfxCommandCopyTextureToBackBuffer(gfx, output_texture);

		gfxFrame(gfx);
	}

	return 0;
}