#pragma once

#include "dx.h"

struct DXTexture
{
	~DXTexture();

	u32 width() const { return (u32)desc.Width; }
	u32 height() const { return (u32)desc.Height; }
	u32 depth() const { return (u32)desc.DepthOrArraySize; }

	DXResource resource;
	D3D12_RESOURCE_DESC desc;
};

std::shared_ptr<DXTexture> create_texture_from_data(void* data, u32 width, u32 height, DXGI_FORMAT format, bool allow_unordered_access, const TCHAR* name = nullptr);

