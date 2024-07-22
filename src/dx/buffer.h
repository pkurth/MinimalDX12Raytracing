#pragma once

#include "dx.h"

struct DXBuffer
{
	virtual ~DXBuffer();

	D3D12_GPU_VIRTUAL_ADDRESS virtual_address() const { return resource->GetGPUVirtualAddress(); }
	u64 size() const { return element_size * element_count; }

	DXResource resource;
	D3D12_RESOURCE_DESC desc;
	u64 element_size;
	u64 element_count;
};

struct DXVertexBuffer
{
	DXVertexBuffer() : view({ 0 }) {}
	DXVertexBuffer(std::shared_ptr<DXBuffer> buffer);

	std::shared_ptr<DXBuffer> buffer;
	D3D12_VERTEX_BUFFER_VIEW view;
};

struct DXIndexBuffer
{
	DXIndexBuffer() : view({ 0 }) {}
	DXIndexBuffer(std::shared_ptr<DXBuffer> buffer);

	std::shared_ptr<DXBuffer> buffer;
	D3D12_INDEX_BUFFER_VIEW view;
};

struct DXVertexBufferGroup
{
	DXVertexBuffer vertex_positions;
	DXVertexBuffer vertex_attributes; // Uvs, normals, etc.
};

std::shared_ptr<DXBuffer> create_buffer(const void* data, u64 element_size, u64 element_count, bool allow_unordered_access = false, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, const TCHAR* name = nullptr);
std::shared_ptr<DXBuffer> create_raytracing_tlas_buffer(u64 total_size, D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, const TCHAR* name = nullptr);
