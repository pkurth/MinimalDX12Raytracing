#include "command_list.h"
#include "context.h"

DXCommandList::DXCommandList(D3D12_COMMAND_LIST_TYPE type)
	: type(type)
{
	check_dx(dx_context.device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));
	check_dx(dx_context.device->CreateCommandList(0, type, allocator.Get(), 0, IID_PPV_ARGS(&cl)));
}

void DXCommandList::set_scissor(const D3D12_RECT& scissor)
{
	cl->RSSetScissorRects(1, &scissor);
}

void DXCommandList::set_viewport(const D3D12_VIEWPORT& viewport)
{
	cl->RSSetViewports(1, &viewport);
}

void DXCommandList::set_render_target(CD3DX12_CPU_DESCRIPTOR_HANDLE rtv)
{
	cl->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void DXCommandList::clear_render_target(CD3DX12_CPU_DESCRIPTOR_HANDLE rtv, float r, float g, float b, float a)
{
	float clear_color[] = { r, g, b, a };
	cl->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
}

void DXCommandList::transition(const DXResource& resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, u32 subresource)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), from, to, subresource);
	cl->ResourceBarrier(1, &barrier);
}

void DXCommandList::transition(std::shared_ptr<DXTexture> texture, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, u32 subresource)
{
	transition(texture->resource, from, to, subresource);
}

void DXCommandList::transition(std::shared_ptr<DXBuffer> buffer, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, u32 subresource)
{
	transition(buffer->resource, from, to, subresource);
}

void DXCommandList::uav_barrier(const DXResource& resource)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource.Get());
	cl->ResourceBarrier(1, &barrier);
}

void DXCommandList::uav_barrier(std::shared_ptr<DXTexture> texture)
{
	uav_barrier(texture->resource);
}

void DXCommandList::uav_barrier(std::shared_ptr<DXBuffer> buffer)
{
	uav_barrier(buffer->resource);
}

void DXCommandList::copy_resource(const DXResource& from, const DXResource& to)
{
	cl->CopyResource(to.Get(), from.Get());
}

void DXCommandList::set_pipeline_state(DXRaytracingPipelineState pipeline_state)
{
	cl->SetPipelineState1(pipeline_state.Get());
}

void DXCommandList::set_render_root_signature(const DXRootSignature& root_signature)
{
	cl->SetGraphicsRootSignature(root_signature.root_signature.Get());
}

void DXCommandList::set_compute_root_signature(const DXRootSignature& root_signature)
{
	cl->SetComputeRootSignature(root_signature.root_signature.Get());
}

void DXCommandList::set_descriptor_heap(const DXDescriptorHeap& descriptor_heap)
{
	descriptor_heaps[descriptor_heap.type] = descriptor_heap.heap.Get();

	u32 heap_count = 0;
	ID3D12DescriptorHeap* heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	for (u32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* heap = descriptor_heaps[i];
		if (heap)
		{
			heaps[heap_count++] = heap;
		}
	}

	cl->SetDescriptorHeaps(heap_count, heaps);
}

void DXCommandList::set_render_descriptor_table(u32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
{
	cl->SetGraphicsRootDescriptorTable(root_parameter_index, handle);
}

void DXCommandList::set_compute_descriptor_table(u32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
{
	cl->SetComputeRootDescriptorTable(root_parameter_index, handle);
}

void DXCommandList::set_compute_root_constants(u32 root_parameter_index, u32 constant_count, const void* constants)
{
	cl->SetComputeRoot32BitConstants(root_parameter_index, constant_count, constants, 0);
}

void DXCommandList::raytrace(const D3D12_DISPATCH_RAYS_DESC& raytrace_desc)
{
	cl->DispatchRays(&raytrace_desc);
}

void DXCommandList::reset()
{
	check_dx(allocator->Reset());
	check_dx(cl->Reset(allocator.Get(), 0));
}
