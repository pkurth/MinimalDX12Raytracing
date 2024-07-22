#pragma once

#include "dx.h"
#include "texture.h"
#include "buffer.h"
#include "root_signature.h"
#include "descriptor_heap.h"

struct DXCommandList
{
	DXCommandList(D3D12_COMMAND_LIST_TYPE type);


	void set_scissor(const D3D12_RECT& scissor);
	void set_viewport(const D3D12_VIEWPORT& viewport);

	void set_render_target(CD3DX12_CPU_DESCRIPTOR_HANDLE rtv);
	void clear_render_target(CD3DX12_CPU_DESCRIPTOR_HANDLE rtv, float r, float g, float b, float a = 1.f);

	void transition(const DXResource& resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, u32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void transition(std::shared_ptr<DXTexture> texture, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, u32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	void transition(std::shared_ptr<DXBuffer> buffer, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, u32 subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	void uav_barrier(const DXResource& resource);
	void uav_barrier(std::shared_ptr<DXTexture> texture);
	void uav_barrier(std::shared_ptr<DXBuffer> buffer);

	void copy_resource(const DXResource& from, const DXResource& to);

	void set_pipeline_state(DXRaytracingPipelineState pipeline_state);
	void set_render_root_signature(const DXRootSignature& root_signature);
	void set_compute_root_signature(const DXRootSignature& root_signature);

	void set_descriptor_heap(const DXDescriptorHeap& descriptor_heap);
	void set_render_descriptor_table(u32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle);
	void set_compute_descriptor_table(u32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle);

	void set_compute_root_constants(u32 root_parameter_index, u32 constant_count, const void* constants);
	
	template<typename T> 
	void set_compute_root_constants(u32 root_parameter_index, const T& constants)
	{
		static_assert(sizeof(T) % 4 == 0, "Size of type must be a multiple of 4 bytes.");
		set_compute_root_constants(root_parameter_index, sizeof(T) / 4, &constants);
	}

	void raytrace(const D3D12_DISPATCH_RAYS_DESC& raytrace_desc);


	void reset();


	com<ID3D12GraphicsCommandList6> cl;
	com<ID3D12CommandAllocator> allocator;

	D3D12_COMMAND_LIST_TYPE type;
	DXCommandList* next = nullptr;
	u64 last_fence_value = 0;

private:
	ID3D12DescriptorHeap* descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
};
