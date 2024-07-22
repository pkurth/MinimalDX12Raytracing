#pragma once

#include "dx.h"

struct TextureMipRange
{
	u32 first = 0;
	u32 count = (u32)-1; // Use all mips.
};

struct BufferRange
{
	u64 first;
	u64 count;
	u64 element_size;
};


void create_2d_texture_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, TextureMipRange mip_range = {}, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
void create_cubemap_texture_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, TextureMipRange mip_range = {}, u32 first_cube = 0, u32 cube_count = 1, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);
void create_volume_texture_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, TextureMipRange mip_range = {}, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);

void create_2d_texture_uav(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, u32 mip_slice, DXGI_FORMAT override_format = DXGI_FORMAT_UNKNOWN);

void create_buffer_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, BufferRange range);
void create_raw_buffer_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource, BufferRange range);
void create_raytracing_buffer_srv(CD3DX12_CPU_DESCRIPTOR_HANDLE handle, const DXResource& resource);
