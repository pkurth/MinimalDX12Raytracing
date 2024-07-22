#pragma once

#include "dx.h"

struct DXRootSignature
{
	com<ID3D12RootSignature> root_signature;
};

DXRootSignature create_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc);




struct DXRootDescriptorTable : CD3DX12_ROOT_PARAMETER
{
	DXRootDescriptorTable(u32 descriptor_range_count,
		const D3D12_DESCRIPTOR_RANGE* descriptor_ranges,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsDescriptorTable(descriptor_range_count, descriptor_ranges, visibility);
	}
};

template <typename T>
struct DXRootConstants : CD3DX12_ROOT_PARAMETER
{
	DXRootConstants(u32 shader_register,
		u32 space = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsConstants(sizeof(T) / 4, shader_register, space, visibility);
	}
};

struct DXRootCBV : CD3DX12_ROOT_PARAMETER
{
	DXRootCBV(u32 shader_register,
		u32 space = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsConstantBufferView(shader_register, space, visibility);
	}
};

struct DXRootSRV : CD3DX12_ROOT_PARAMETER
{
	DXRootSRV(u32 shader_register,
		u32 space = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsShaderResourceView(shader_register, space, visibility);
	}
};

struct DXRootUAV : CD3DX12_ROOT_PARAMETER
{
	DXRootUAV(u32 shader_register,
		u32 space = 0,
		D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		InitAsUnorderedAccessView(shader_register, space, visibility);
	}
};

