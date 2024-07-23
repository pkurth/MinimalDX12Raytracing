#include "dx/dxwindow.h"
#include "dx/context.h"

#include "core/input.h"

#include "scene/scene.h"

i32 main(i32 argc, char** argv)
{
	DXWindow window(TEXT("Minimal DX"), 1280, 720);
	//window.set_vsync(true);


	
	Arena asset_arena;
	Mesh cube = create_cube_mesh(asset_arena);
	Mesh sphere = create_sphere_mesh(asset_arena);

	Scene scene;
	scene.push_object(cube, vec3(1.f, 1.f, 0.f), vec3(0.f, 0.f, 0.f), quat(vec3(0.f, 1.f, 0.f), deg2rad(45.f)));
	scene.push_object(cube, vec3(0.f, 1.f, 1.f), vec3(10.f, 4.f, -10.f), quat(vec3(1.f, 0.f, 0.f), deg2rad(30.f)), vec3(2.f));
	scene.push_object(sphere, vec3(1.f, 0.f, 0.f), vec3(-3.f, 1.f, -2.f), quat::identity);
	scene.push_object(cube, vec3(0.f, 0.f, 1.f), vec3(0.f, -2.2f, -40.f), quat::identity, vec3(50.f, 0.1f, 50.f)); // Ground

	scene.build();


	Input input = {};
	while (WindowUpdate window_update = window.begin_frame(input))
	{
		dx_context.begin_frame();
		
		scene.update(input, window.client_width, window.client_height, window_update.delta_time_seconds);

		std::shared_ptr<DXTexture> rendered_scene = scene.render(window.client_width, window.client_height);
		
		u64 fence = window.blit_to_screen(rendered_scene);
		window.end_frame(fence);
	}

	return EXIT_SUCCESS;
}
