#pragma once

#include "dx.h"
#include "descriptor_heap.h"
#include "buffer.h"
#include "root_signature.h"

#include "core/memory.h"
#include "core/math.h"
#include "core/range.h"


struct Mesh;

struct DXRaytracingBLAS
{
	std::shared_ptr<DXBuffer> blas;
};

struct DXRaytracingTLAS
{
	std::shared_ptr<DXBuffer> tlas;
	std::shared_ptr<DXBuffer> scratch;
};

void create_raytracing_blas(Mesh& mesh, Arena& arena);

D3D12_RAYTRACING_INSTANCE_DESC create_raytracing_instance_desc(const DXRaytracingBLAS& blas, const Transform& transform, u32 instance_contribution_to_hitgroup_index);
void create_raytracing_tlas(DXRaytracingTLAS& tlas, Range<D3D12_RAYTRACING_INSTANCE_DESC> instances);


struct DXRaytracingBindingTableDesc
{
	static constexpr u32 max_hitgroup_count = 8;

	u32 entry_size;
	u32 hitgroup_count;

	void* raygen;
	void* miss[max_hitgroup_count];
	void* hitgroups[max_hitgroup_count];

	u64 raygen_offset;
	u64 miss_offset;
	u64 hit_offset;
};

struct DXRaytracingPipeline
{
	DXRaytracingPipelineState pipeline;
	DXRootSignature root_signature;

	DXRaytracingBindingTableDesc binding_table_desc;
};

struct DXShaderDefine
{
	DXShaderDefine* next;

	const wchar* define;
	u32 number;
};

struct DXRaytracingPipelineBuilder
{
	DXRaytracingPipelineBuilder(const std::filesystem::path& shader_filename, u32 payload_size, u32 max_recursion_depth)
		: shader_filename(shader_filename), payload_size(payload_size), max_recursion_depth(max_recursion_depth)
	{}

	DXRaytracingPipelineBuilder& global_root_signature(D3D12_ROOT_SIGNATURE_DESC root_signature_desc);

	DXRaytracingPipelineBuilder& raygen(const wchar* entry_point, D3D12_ROOT_SIGNATURE_DESC root_signature_desc);
	DXRaytracingPipelineBuilder& hitgroup(const wchar* group_name, const wchar* miss, const wchar* closest_hit, const wchar* any_hit, D3D12_ROOT_SIGNATURE_DESC root_signature_desc);

	DXRaytracingPipelineBuilder& define(const wchar* label, u32 number);

	DXRaytracingPipeline build();

private:

	struct DXRaytracingRootSignature
	{
		DXRootSignature root_signature;
		ID3D12RootSignature* ptr;
	};

	DXRaytracingRootSignature create_raytracing_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc);


	struct RayGen
	{
		const wchar* entry_point;
		D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
	};

	struct HitGroup
	{
		HitGroup* next;

		const wchar* group_name; 
		const wchar* miss; 
		union
		{
			struct
			{
				const wchar* closest_hit;
				const wchar* any_hit;
			};
			const wchar* hits[2];
		};
		D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
	};

	std::filesystem::path shader_filename;
	u32 payload_size;
	u32 max_recursion_depth;

	D3D12_ROOT_SIGNATURE_DESC global_root_signature_desc;

	RayGen raygen_desc;
	LinkedList<HitGroup> hitgroup_descs;
	LinkedList<DXShaderDefine> user_defines;

	Arena arena;
};



struct DXRaytracingBindingTable
{
	std::shared_ptr<DXBuffer> buffer;
	u32 entry_count;
};

template <typename ShaderData>
struct DXRaytracingBindingTableBuilder
{
	DXRaytracingBindingTableBuilder(const DXRaytracingBindingTableDesc& desc);

	void push(Range<ShaderData> data_for_all_hitgroups);

	DXRaytracingBindingTable build();

private:

	struct alignas(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) BindingTableEntry
	{
		u8 identifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
		ShaderData shader_data;
	};

	DXRaytracingBindingTableDesc binding_table_desc;
	u32 entry_count = 0;

	Arena arena;
};

template<typename ShaderData>
inline DXRaytracingBindingTableBuilder<ShaderData>::DXRaytracingBindingTableBuilder(const DXRaytracingBindingTableDesc& desc) 
	: binding_table_desc(desc)
{
	ASSERT(arena.current == desc.raygen_offset);
	BindingTableEntry* raygen_entry = arena.allocate<BindingTableEntry>(1);
	memcpy(raygen_entry->identifier, desc.raygen, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	arena.align_next_to(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	ASSERT(arena.current == desc.miss_offset);

	for (u32 i = 0; i < desc.hitgroup_count; ++i)
	{
		BindingTableEntry* miss_entry = arena.allocate<BindingTableEntry>(1);
		memcpy(miss_entry->identifier, desc.miss[i], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}

	arena.align_next_to(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	ASSERT(arena.current == desc.hit_offset);
}

template<typename ShaderData>
inline void DXRaytracingBindingTableBuilder<ShaderData>::push(Range<ShaderData> data_for_all_hitgroups)
{
	ASSERT(data_for_all_hitgroups.count == binding_table_desc.hitgroup_count);

	for (auto [shader_data, index] : data_for_all_hitgroups)
	{
		BindingTableEntry* hitgroup_entry = arena.allocate<BindingTableEntry>(1);
		memcpy(hitgroup_entry->identifier, binding_table_desc.hitgroups[index], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		hitgroup_entry->shader_data = shader_data;
	}

	++entry_count;
}

template<typename ShaderData>
inline DXRaytracingBindingTable DXRaytracingBindingTableBuilder<ShaderData>::build()
{
	ASSERT(arena.current > 0);

	std::shared_ptr<DXBuffer> buffer = create_buffer(arena.memory, arena.current, 1, false, L"Raytracing binding table");
	return DXRaytracingBindingTable{ buffer, entry_count };
}




