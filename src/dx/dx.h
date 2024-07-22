#pragma once

#include "core/common.h"

#include <dx/d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>


#define NUM_BUFFERED_FRAMES 3

template <typename T>
using com = Microsoft::WRL::ComPtr<T>;


static void _check_dx(HRESULT hr, char* file, i32 line)
{
	if (FAILED(hr))
	{
		char buffer[128];

		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, hr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			buffer,
			sizeof(buffer),
			NULL);

		std::cerr << "Command failed [" << file << " : " << line << "]: " << buffer << ".\n";

		__debugbreak();
	}
}

#define check_dx(hr) _check_dx(hr, __FILE__, __LINE__)


typedef com<IDXGIFactory4> DXFactory;
typedef com<IDXGIAdapter4> DXAdapter;
typedef com<ID3D12Device5> DXDevice;
typedef com<IDXGISwapChain4> DXSwapchain;
typedef com<ID3D12Resource> DXResource;
typedef com<ID3D12Object> DXObject;
typedef com<ID3DBlob> DXBlob;
typedef com<ID3D12StateObject> DXRaytracingPipelineState;

static inline u32 get_reference_count(com<ID3D12Resource>& resource)
{
	resource->AddRef();
	return resource->Release();
}
