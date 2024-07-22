#pragma once

#include "dx/texture.h"
#include "dx/raytracing.h"
#include "dx/command_list.h"
#include "dx/mesh.h"

struct RenderMaterial
{
	vec3 color;
};

struct RadianceHitgroupResources
{
	RenderMaterial material;
	CD3DX12_GPU_DESCRIPTOR_HANDLE descriptor_base; // Vertex and index buffer

	static constexpr u64 descriptor_count = 2;
};

struct ShadowHitgroupResources
{
	// Shadow needs no resources currently
	static constexpr u64 descriptor_count = 0;
};

union PerObjectRenderResources
{
	// This is a union, not a struct. One entry is as large as the largest hitgroup.
	RadianceHitgroupResources radiance;
	ShadowHitgroupResources shadow;

	static constexpr u64 descriptor_count = RadianceHitgroupResources::descriptor_count + ShadowHitgroupResources::descriptor_count;
};

struct RenderParams
{
	vec3 camera_position;
	quat camera_rotation;

	vec3 light_direction;
	vec3 light_color;
	vec3 sky_color;
};


struct Renderer
{
	Renderer();

	std::shared_ptr<DXTexture> render(const DXRaytracingTLAS& tlas, const DXRaytracingBindingTable& binding_table, const DXDescriptorHeap& descriptor_heap, RenderParams params,
		u32 render_width, u32 render_height);

	const DXRaytracingPipeline& get_pipeline() const { return pipeline; }

	static u64 reserved_descriptor_heap_space();
	static u64 total_descriptor_heap_space(u64 total_submesh_count);

	static void setup_hitgroup(Range<PerObjectRenderResources> data_for_all_hitgroups, DXDescriptorHeap& descriptor_heap, const Mesh& mesh, Submesh submesh, vec3 color);

private:

	CD3DX12_GPU_DESCRIPTOR_HANDLE create_global_descriptors(const DXDescriptorHeap& descriptor_heap, const DXRaytracingTLAS& tlas);

	DXRaytracingPipeline pipeline;
	std::shared_ptr<DXTexture> render_target;
};

