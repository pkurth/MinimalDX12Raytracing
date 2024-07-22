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

std::shared_ptr<DXTexture> Scene::render(u32 render_width, u32 render_height)
{
	RenderParams params;
	params.camera_position = vec3(0.f, 0.f, 7.f);
	params.camera_rotation = quat::identity;
	params.light_color = vec3(1.f, 1.f, 1.f);
	params.light_direction = normalize(vec3(-1.f, -1.f, -1.f));
	params.sky_color = vec3(0.62f, 0.75f, 0.88f);

	return renderer.render(tlas, binding_table, descriptor_heap, params, render_width, render_height);
}

void Scene::build_binding_table()
{
	ArenaMarker marker(temp_arena);


	descriptor_heap.initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true, 1024); // TODO: Capacity
	descriptor_heap.allocate(renderer.reserved_descriptor_heap_space());


	const DXRaytracingBindingTableDesc& binding_table_desc = renderer.get_pipeline().binding_table_desc;
	Range<PerObjectRenderResources> data_for_all_hitgroups = temp_arena.allocate_range<PerObjectRenderResources>(binding_table_desc.hitgroup_count);

	DXRaytracingBindingTableBuilder<PerObjectRenderResources> binding_table_builder(binding_table_desc);
	for (SceneObject& obj : objects)
	{
		for (auto [submesh, submesh_index] : obj.mesh.submeshes)
		{
			DXDescriptorAllocation descriptors = descriptor_heap.allocate(2);
			create_buffer_srv(descriptors.cpu_at(0), obj.mesh.vertex_buffer.vertex_attributes.buffer->resource, BufferRange{ submesh.base_vertex, submesh.vertex_count, obj.mesh.vertex_buffer.vertex_attributes.buffer->element_size });
			create_raw_buffer_srv(descriptors.cpu_at(1), obj.mesh.index_buffer.buffer->resource, BufferRange{ submesh.first_index, submesh.index_count, obj.mesh.index_buffer.buffer->element_size });

			RadianceHitgroupResources& radiance = data_for_all_hitgroups[0].radiance;
			radiance.material.color = obj.color;
			radiance.descriptors = descriptors.gpu_base;

			ShadowHitgroupResources& shadow = data_for_all_hitgroups[1].shadow;
			// Shadow needs no resources currently

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
		instance_descs[obj_index] = create_raytracing_instance_desc(obj.mesh.blas, obj.transform, instance_contribution_to_hitgroup_index);
		instance_contribution_to_hitgroup_index += (u32)obj.mesh.submeshes.count * binding_table_desc.hitgroup_count;
	}

	create_raytracing_tlas(tlas, instance_descs);
}
