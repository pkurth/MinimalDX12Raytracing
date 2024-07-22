#include "scene.h"
#include "dx/context.h"

void Scene::push_object(const Mesh& mesh, vec3 color, vec3 position, quat rotation, vec3 scale)
{
	SceneObject& obj = objects.emplace_back();
	obj.mesh = mesh;
	obj.color = color;
	obj.transform = Transform(position, rotation, scale);
}

void Scene::build()
{
	build_binding_table();
	build_tlas();
}

void Scene::update(const Input& input, u32 render_width, u32 render_height, float dt)
{
	camera.update(input, render_width, render_height, dt);
}

std::shared_ptr<DXTexture> Scene::render(u32 render_width, u32 render_height)
{
	RenderParams params;
	params.camera_position = camera.position();
	params.camera_rotation = camera.rotation;
	params.light_color = vec3(1.f, 1.f, 1.f);
	params.light_direction = normalize(vec3(-1.f, -1.f, -1.f));
	params.sky_color = vec3(0.62f, 0.75f, 0.88f);

	return renderer.render(tlas, binding_table, descriptor_heap, params, render_width, render_height);
}

u64 Scene::total_submesh_count()
{
	u64 total_submesh_count = 0;
	for (SceneObject& obj : objects)
	{
		total_submesh_count += obj.mesh.submeshes.count;
	}
	return total_submesh_count;
}

void Scene::build_binding_table()
{
	ArenaMarker marker(temp_arena);


	// Allocate descriptor heap and reserve space needed by the renderer
	descriptor_heap.initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true, renderer.total_descriptor_heap_space(total_submesh_count()));
	descriptor_heap.allocate(renderer.reserved_descriptor_heap_space());


	const DXRaytracingBindingTableDesc& binding_table_desc = renderer.get_pipeline().binding_table_desc;
	Range<PerObjectRenderResources> data_for_all_hitgroups = temp_arena.allocate_range<PerObjectRenderResources>(binding_table_desc.hitgroup_count);

	DXRaytracingBindingTableBuilder<PerObjectRenderResources> binding_table_builder(binding_table_desc);
	for (SceneObject& obj : objects)
	{
		for (auto [submesh, submesh_index] : obj.mesh.submeshes)
		{
			// Let renderer initialize the required data
			renderer.setup_hitgroup(data_for_all_hitgroups, descriptor_heap, obj.mesh, submesh, obj.color);

			// Create binding table entry for this subobject
			binding_table_builder.push(data_for_all_hitgroups);
		}
	}

	binding_table = binding_table_builder.build();
}

void Scene::build_tlas()
{
	ArenaMarker marker(temp_arena);


	const DXRaytracingBindingTableDesc& binding_table_desc = renderer.get_pipeline().binding_table_desc;

	Range<D3D12_RAYTRACING_INSTANCE_DESC> instance_descs = temp_arena.allocate_range<D3D12_RAYTRACING_INSTANCE_DESC>(objects.size());

	u32 instance_contribution_to_hitgroup_index = 0;

	for (auto [obj, obj_index] : Range(objects))
	{
		// Instantiate object in TLAS
		instance_descs[obj_index] = create_raytracing_instance_desc(obj.mesh.blas, obj.transform, instance_contribution_to_hitgroup_index);

		// Adjust the index to the next object. Since each submesh has an entry for each hitgroup, we need to advance by the product of the two.
		instance_contribution_to_hitgroup_index += (u32)obj.mesh.submeshes.count * binding_table_desc.hitgroup_count;
	}

	create_raytracing_tlas(tlas, instance_descs);
}

void Camera::update(const Input& input, u32 viewport_width, u32 viewport_height, float dt)
{
	const float CAMERA_MOVEMENT_SPEED = 3.f;
	const float CAMERA_SENSITIVITY = 4.f;

	float aspect = (float)viewport_width / (float)viewport_height;

	vec2 rel_mouse_delta = input.mouse_delta() / vec2((float)viewport_width, (float)viewport_height);

	if (input.mouse_scroll() != 0.f)
	{
		orbit_radius -= input.mouse_scroll();
	}

	if (input.is_mouse_down(MouseButton::Left))
	{
		vec2 angle = -rel_mouse_delta * CAMERA_SENSITIVITY;

		// https://gamedev.stackexchange.com/questions/136174/im-rotating-an-object-on-two-axes-so-why-does-it-keep-twisting-around-the-thir
		rotation = quat(vec3(0.f, 1.f, 0.f), angle.x) * rotation * quat(vec3(1.f, 0.f, 0.f), angle.y);
		rotation = normalize(rotation);
	}
	else if (input.is_mouse_down(MouseButton::Right))
	{
		vec3 direction = vec3(-rel_mouse_delta.x * aspect, rel_mouse_delta.y, 0.f) * 1000.f * CAMERA_MOVEMENT_SPEED;
		center += rotation * direction * dt;
	}
}
