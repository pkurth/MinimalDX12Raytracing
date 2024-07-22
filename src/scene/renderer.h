#pragma once

#include "dx/texture.h"
#include "dx/raytracing.h"
#include "dx/command_list.h"

struct RendererGlobalResources
{
	// Input
	CD3DX12_CPU_DESCRIPTOR_HANDLE tlas;

	// Output
	CD3DX12_CPU_DESCRIPTOR_HANDLE render_output;
};

struct RenderMaterial
{
	vec3 color;
};

struct RadianceHitgroupResources
{
	RenderMaterial material;
	CD3DX12_GPU_DESCRIPTOR_HANDLE descriptors;
};

struct ShadowHitgroupResources
{
	// Shadow needs no resources currently
};

union RendererPerObjectResources
{
	// This is a union, not a struct. One entry is as large as the largest hitgroup.
	RadianceHitgroupResources radiance;
	ShadowHitgroupResources shadow;
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

private:
	DXRaytracingPipeline pipeline;
	std::shared_ptr<DXTexture> render_target;
};

