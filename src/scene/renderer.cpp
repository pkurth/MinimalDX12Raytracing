#include "renderer.h"

#include "dx/context.h"

struct Constants
{
	mat4 camera_transform;
	vec3 light_direction;
	float padding0;
	vec3 light_color;
	float padding1;
	vec3 sky_color;
	float padding2;
};


#define ROOT_CONSTANTS		0
#define ROOT_DESCRIPTORS	1



// Resources which are available to all stages of the raytracing shader.
// This typically includes the TLAS, the render output and resources like the skybox.

struct GlobalRenderInputs
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE tlas;

	void create_descriptors(const DXRaytracingTLAS& tlas)
	{
		create_raytracing_buffer_srv(this->tlas, tlas.tlas->resource);
	}
};

struct GlobalRenderOutputs
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE render_output;

	void create_descriptors(std::shared_ptr<DXTexture> render_output)
	{
		create_2d_texture_uav(this->render_output, render_output->resource, 0);
	}
};


Renderer::Renderer()
{
	constexpr u32 global_inputs = sizeof(GlobalRenderInputs) / sizeof(CD3DX12_CPU_DESCRIPTOR_HANDLE);
	constexpr u32 global_outputs = sizeof(GlobalRenderOutputs) / sizeof(CD3DX12_CPU_DESCRIPTOR_HANDLE);

	CD3DX12_DESCRIPTOR_RANGE global_resource_ranges[] =
	{
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, global_inputs, 0),
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, global_outputs, 0),
	};

	CD3DX12_ROOT_PARAMETER global_root_parameters[] =
	{
		DXRootConstants<Constants>(0),
		DXRootDescriptorTable(global_resource_ranges),
	};

	D3D12_ROOT_SIGNATURE_DESC global_root_signature_desc =
	{
		arraysize(global_root_parameters), global_root_parameters
	};



	CD3DX12_DESCRIPTOR_RANGE radiance_resource_ranges(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 1);

	CD3DX12_ROOT_PARAMETER radiance_hitgroup_parameters[] =
	{
		DXRootConstants<RenderMaterial>(0, 1),
		DXRootDescriptorTable(radiance_resource_ranges),
	};

	D3D12_ROOT_SIGNATURE_DESC radiance_hitgroup_root_signature_desc =
	{
		arraysize(radiance_hitgroup_parameters), radiance_hitgroup_parameters
	};



	u32 payload_size = sizeof(float) * 4; // sizeof(RadiancePayload)
	u32 max_recursion_depth = 4;

	pipeline =
		DXRaytracingPipelineBuilder("shaders/raytracing.hlsl", payload_size, max_recursion_depth)
		.global_root_signature(global_root_signature_desc)
		.raygen(L"raygen", {})
		.hitgroup(L"RADIANCE", L"radiance_miss", L"radiance_closest_hit", nullptr, radiance_hitgroup_root_signature_desc)
		.hitgroup(L"SHADOW", L"shadow_miss", nullptr, nullptr, {})
		.build();
}

std::shared_ptr<DXTexture> Renderer::render(const DXRaytracingTLAS& tlas, const DXRaytracingBindingTable& binding_table, const DXDescriptorHeap& descriptor_heap, RenderParams params,
	u32 render_width, u32 render_height)
{
	if (!render_target || render_target->width() != render_width || render_target->height() != render_height)
	{
		render_target = create_texture(nullptr, render_width, render_height, DXGI_FORMAT_R8G8B8A8_UNORM, true, L"Main render target");
	}

	D3D12_DISPATCH_RAYS_DESC raytrace_desc;
	raytrace_desc.Width = render_width;
	raytrace_desc.Height = render_height;
	raytrace_desc.Depth = 1;

	// Pointer to the entry point of the ray-generation shader.
	raytrace_desc.RayGenerationShaderRecord.StartAddress = binding_table.buffer->virtual_address() + pipeline.binding_table_desc.raygen_offset;
	raytrace_desc.RayGenerationShaderRecord.SizeInBytes = pipeline.binding_table_desc.entry_size;

	// Pointer to the entry point(s) of the miss shader.
	raytrace_desc.MissShaderTable.StartAddress = binding_table.buffer->virtual_address() + pipeline.binding_table_desc.miss_offset;
	raytrace_desc.MissShaderTable.StrideInBytes = pipeline.binding_table_desc.entry_size;
	raytrace_desc.MissShaderTable.SizeInBytes = pipeline.binding_table_desc.entry_size * pipeline.binding_table_desc.hitgroup_count;

	// Pointer to the entry point(s) of the hit shader.
	raytrace_desc.HitGroupTable.StartAddress = binding_table.buffer->virtual_address() + pipeline.binding_table_desc.hit_offset;
	raytrace_desc.HitGroupTable.StrideInBytes = pipeline.binding_table_desc.entry_size;
	raytrace_desc.HitGroupTable.SizeInBytes = pipeline.binding_table_desc.entry_size * pipeline.binding_table_desc.hitgroup_count * binding_table.entry_count;

	raytrace_desc.CallableShaderTable = {};


	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_base = create_global_descriptors(descriptor_heap, tlas);

	Constants constants;
	constants.camera_transform = create_model_matrix(params.camera_position, params.camera_rotation);
	constants.light_color = params.light_color;
	constants.light_direction = params.light_direction;
	constants.sky_color = params.sky_color;


	DXCommandList* cl = dx_context.get_free_render_command_list();

	cl->set_pipeline_state(pipeline.pipeline);
	cl->set_compute_root_signature(pipeline.root_signature);

	cl->set_descriptor_heap(descriptor_heap);
	cl->set_compute_root_constants(ROOT_CONSTANTS, constants);
	cl->set_compute_descriptor_table(ROOT_DESCRIPTORS, gpu_base);

	cl->transition(render_target, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cl->raytrace(raytrace_desc);
	cl->transition(render_target, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);

	dx_context.execute_command_list(cl);

	return render_target;
}




static constexpr u32 global_resource_count = (sizeof(GlobalRenderInputs) + sizeof(GlobalRenderOutputs)) / sizeof(CD3DX12_CPU_DESCRIPTOR_HANDLE);

u64 Renderer::reserved_descriptor_heap_space()
{
	return global_resource_count * NUM_BUFFERED_FRAMES;
}

u64 Renderer::total_descriptor_heap_space(u64 total_submesh_count)
{
	return total_submesh_count * PerObjectRenderResources::descriptor_count + reserved_descriptor_heap_space();
}

void Renderer::setup_hitgroup(Range<PerObjectRenderResources> data_for_all_hitgroups, DXDescriptorHeap& descriptor_heap, const Mesh& mesh, Submesh submesh, vec3 color)
{
	ASSERT(data_for_all_hitgroups.count == 2);

	// Radiance hitgroup
	RadianceHitgroupResources& radiance = data_for_all_hitgroups[0].radiance;
	radiance.material.color = color;
	DXDescriptorAllocation descriptors = descriptor_heap.allocate(PerObjectRenderResources::descriptor_count);
	radiance.descriptor_base = descriptors.gpu_base;

	create_buffer_srv(descriptors.cpu_at(0), mesh.vertex_buffer.vertex_attributes.buffer->resource, BufferRange{ submesh.base_vertex, submesh.vertex_count, mesh.vertex_buffer.vertex_attributes.buffer->element_size });
	create_raw_buffer_srv(descriptors.cpu_at(1), mesh.index_buffer.buffer->resource, BufferRange{ submesh.first_index, submesh.index_count, mesh.index_buffer.buffer->element_size });
	

	// Shadow needs no resources currently
	ShadowHitgroupResources& shadow = data_for_all_hitgroups[1].shadow;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Renderer::create_global_descriptors(const DXDescriptorHeap& descriptor_heap, const DXRaytracingTLAS& tlas)
{
	union
	{
		struct
		{
			GlobalRenderInputs inputs;
			GlobalRenderOutputs outputs;
		};
		CD3DX12_CPU_DESCRIPTOR_HANDLE handles[global_resource_count];
	};

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_base(descriptor_heap.cpu_base, global_resource_count * dx_context.wrapping_frame_id(), descriptor_heap.descriptor_size);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_base(descriptor_heap.gpu_base, global_resource_count * dx_context.wrapping_frame_id(), descriptor_heap.descriptor_size);

	for (u32 i = 0; i < global_resource_count; ++i)
	{
		handles[i].InitOffsetted(cpu_base, i, descriptor_heap.descriptor_size);
	}

	inputs.create_descriptors(tlas);
	outputs.create_descriptors(render_target);

	return gpu_base;
}
