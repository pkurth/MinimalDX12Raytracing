#include "raytracing.h"
#include "context.h"
#include "mesh.h"

#include <dxcapi.h>


void create_raytracing_blas(Mesh& mesh, Arena& arena)
{
	ArenaMarker marker(arena);

	D3D12_RAYTRACING_GEOMETRY_DESC* descs = arena.allocate<D3D12_RAYTRACING_GEOMETRY_DESC>(mesh.submeshes.count);

	for (auto [submesh, index] : mesh.submeshes)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC& desc = descs[index];

		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		desc.Triangles.Transform3x4 = 0;

		desc.Triangles.VertexBuffer.StartAddress = mesh.vertex_buffer.vertex_positions.view.BufferLocation + (mesh.vertex_buffer.vertex_positions.buffer->element_size * submesh.base_vertex);
		desc.Triangles.VertexBuffer.StrideInBytes = mesh.vertex_buffer.vertex_positions.view.StrideInBytes;
		desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.Triangles.VertexCount = submesh.vertex_count;

		desc.Triangles.IndexBuffer = mesh.index_buffer.view.BufferLocation + (mesh.index_buffer.buffer->element_size * submesh.first_index);
		desc.Triangles.IndexFormat = mesh.index_buffer.view.Format;
		desc.Triangles.IndexCount = submesh.index_count;
	}



	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	inputs.NumDescs = (u32)mesh.submeshes.count;
	inputs.pGeometryDescs = descs;

	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;


	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	dx_context.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	info.ScratchDataSizeInBytes = align_to<u64>(info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	info.ResultDataMaxSizeInBytes = align_to<u64>(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);


	std::shared_ptr<DXBuffer> scratch = create_buffer(nullptr, 1, info.ScratchDataSizeInBytes, true, L"BLAS scratch");
	std::shared_ptr<DXBuffer> blas = create_raytracing_buffer(info.ResultDataMaxSizeInBytes, L"BLAS");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC as_desc = {};
	as_desc.Inputs = inputs;
	as_desc.DestAccelerationStructureData = blas->virtual_address();
	as_desc.ScratchAccelerationStructureData = scratch->virtual_address();


	DXCommandList* cl = dx_context.get_free_render_command_list();
	cl->cl->BuildRaytracingAccelerationStructure(&as_desc, 0, 0);
	cl->uav_barrier(blas);
	dx_context.execute_command_list(cl);

	mesh.blas.blas = blas;
}

D3D12_RAYTRACING_INSTANCE_DESC create_raytracing_instance_desc(const DXRaytracingBLAS& blas, const Transform& transform, u32 instance_contribution_to_hitgroup_index)
{
	D3D12_RAYTRACING_INSTANCE_DESC instance;

	instance.Flags = 0;// D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
	instance.InstanceContributionToHitGroupIndex = instance_contribution_to_hitgroup_index;

	mat4 m = transpose(transform_to_mat4(transform));
	memcpy(instance.Transform, &m, sizeof(instance.Transform));
	instance.AccelerationStructure = blas.blas->virtual_address();
	instance.InstanceMask = 0xFF;
	instance.InstanceID = 0; // This value will be exposed to the shader via InstanceID().

	return instance;
}

void create_raytracing_tlas(DXRaytracingTLAS& tlas, Range<D3D12_RAYTRACING_INSTANCE_DESC> instances)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    inputs.NumDescs = (u32)instances.count;
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	//inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    dx_context.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
    info.ScratchDataSizeInBytes = align_to<u64>(info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    info.ResultDataMaxSizeInBytes = align_to<u64>(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);


    if (!tlas.scratch || tlas.scratch->size() < info.ScratchDataSizeInBytes)
    {
        tlas.scratch = create_buffer(nullptr, 1, info.ScratchDataSizeInBytes, true, L"TLAS scratch");
    }

	if (!tlas.tlas || tlas.tlas->size() < info.ResultDataMaxSizeInBytes)
	{
		tlas.tlas = create_raytracing_buffer(info.ResultDataMaxSizeInBytes, L"TLAS");
	}


	DXAllocation gpu_instances = dx_context.push_to_scratch(instances);

    inputs.InstanceDescs = gpu_instances.gpu_ptr;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC as_desc = {};
	as_desc.Inputs = inputs;
	as_desc.DestAccelerationStructureData = tlas.tlas->virtual_address();
	as_desc.ScratchAccelerationStructureData = tlas.scratch->virtual_address();


	DXCommandList* cl = dx_context.get_free_render_command_list();
    cl->cl->BuildRaytracingAccelerationStructure(&as_desc, 0, 0);
    cl->uav_barrier(tlas.tlas);
	dx_context.execute_command_list(cl);
}

static u32 get_shader_binding_table_desc_size(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc)
{
	u32 size = 0;
	for (u32 i = 0; i < root_signature_desc.NumParameters; ++i)
	{
		if (root_signature_desc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
		{
			size += align_to<u32>(root_signature_desc.pParameters[i].Constants.Num32BitValues * 4, 8);
		}
		else
		{
			size += 8;
		}
	}
	return size;
}

DXRaytracingPipelineBuilder::DXRaytracingRootSignature DXRaytracingPipelineBuilder::create_raytracing_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	DXRaytracingRootSignature result;
	result.root_signature = create_root_signature(desc);
	result.ptr = result.root_signature.root_signature.Get();
	return result;
}

DXRaytracingPipelineBuilder& DXRaytracingPipelineBuilder::global_root_signature(D3D12_ROOT_SIGNATURE_DESC root_signature_desc)
{
	global_root_signature_desc = root_signature_desc;
	return *this;
}

DXRaytracingPipelineBuilder& DXRaytracingPipelineBuilder::raygen(const wchar* entry_point, D3D12_ROOT_SIGNATURE_DESC root_signature_desc)
{
	raygen_desc.entry_point = entry_point;
	raygen_desc.root_signature_desc = root_signature_desc;
	raygen_desc.root_signature_desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	return *this;
}

DXRaytracingPipelineBuilder& DXRaytracingPipelineBuilder::hitgroup(const wchar* group_name, const wchar* miss, const wchar* closest_hit, const wchar* any_hit, D3D12_ROOT_SIGNATURE_DESC root_signature_desc)
{
	HitGroup* hitgroup = arena.allocate<HitGroup>(1);
	hitgroup->group_name = group_name;
	hitgroup->miss = miss;
	hitgroup->closest_hit = closest_hit;
	hitgroup->any_hit = any_hit;
	hitgroup->root_signature_desc = root_signature_desc;
	hitgroup->root_signature_desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	
	hitgroup_descs.push_back(hitgroup);

	return *this;
}

DXRaytracingPipelineBuilder& DXRaytracingPipelineBuilder::define(const wchar* label, u32 number)
{
	DXShaderDefine* define = arena.allocate<DXShaderDefine>(1);
	define->define = label;
	define->number = number;

	user_defines.push_back(define);

	return *this;
}


template <typename T>
static Range<T> allocate_empty_range(Arena& arena, u32 count)
{
	Range<T> result = arena.allocate_range<T>(count, true);
	result.count = 0;
	return result;
}

template <typename T>
static T& next(Range<T>& range)
{
	return range[range.count++];
}

static void report_shader_compile_error(com<IDxcBlobEncoding> blob)
{
	char info_log[2048];
	memcpy(info_log, blob->GetBufferPointer(), sizeof(info_log) - 1);
	info_log[sizeof(info_log) - 1] = 0;
	std::cerr << "Error:\n" << info_log << '\n';
}

static com<IDxcBlob> compile_library(const std::filesystem::path& filename, Range<DXShaderDefine> shader_defines, Arena& arena)
{
	com<IDxcUtils> utils;
	check_dx(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));

	com<IDxcIncludeHandler> include_handler;
	check_dx(utils->CreateDefaultIncludeHandler(&include_handler));

	std::wstring shader_source_code = L"#include \"" + filename.wstring() + L"\"\n";

	com<IDxcCompiler3> compiler;
	check_dx(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));


	DxcBuffer source_buffer;
	source_buffer.Ptr = shader_source_code.data();
	source_buffer.Size = shader_source_code.size() * sizeof(wchar);
	source_buffer.Encoding = DXC_CP_UTF16;

	const wchar_t* target_profile = L"lib_6_3";

	bool add_debug_info = false;
#if defined(_DEBUG)
	add_debug_info = true;
#endif


	Range<const wchar*> arguments = allocate_empty_range<const wchar*>(arena, (u32)shader_defines.count * 2 + 2 + add_debug_info * 2);
	for (auto [define, index] : shader_defines)
	{
		wchar* buffer = arena.allocate<wchar>(128);
		wsprintf(buffer, L"%ws=%d", define.define, define.number);

		next(arguments) = L"-D";
		next(arguments) = buffer;
	}
	next(arguments) = L"-T";
	next(arguments) = L"lib_6_3";

	if (add_debug_info)
	{
		next(arguments) = L"-Zi";
		next(arguments) = L"-Qembed_debug";
	}

	com<IDxcResult> compile_result;
	check_dx(compiler->Compile(&source_buffer, arguments.first, (u32)arguments.count, include_handler.Get(), IID_PPV_ARGS(&compile_result)));


	HRESULT result;
	check_dx(compile_result->GetStatus(&result));
	if (FAILED(result))
	{
		com<IDxcBlobEncoding> error;
		check_dx(compile_result->GetErrorBuffer(&error));
		report_shader_compile_error(error);
		__debugbreak();
		return 0;
	}

	com<IDxcBlob> blob;
	check_dx(compile_result->GetResult(&blob));

	return blob;
}

DXRaytracingPipeline DXRaytracingPipelineBuilder::build()
{
	ArenaMarker marker(arena);

	u32 hitgroup_count = hitgroup_descs.count();
	u32 subobject_count = 3 + hitgroup_count * 3 + 6;
	u32 export_count = 1 + hitgroup_count * 3;
	u32 association_count = 1 + hitgroup_count + 2;
	u32 defines_count = hitgroup_count + 2 + user_defines.count();

	Range<D3D12_STATE_SUBOBJECT> subobjects = allocate_empty_range<D3D12_STATE_SUBOBJECT>(arena, subobject_count);
	Range<D3D12_EXPORT_DESC> exports = allocate_empty_range<D3D12_EXPORT_DESC>(arena, export_count);
	Range<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> associations = allocate_empty_range<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>(arena, association_count);
	Range<D3D12_HIT_GROUP_DESC> hitgroups = allocate_empty_range<D3D12_HIT_GROUP_DESC>(arena, hitgroup_count);
	Range<const wchar*> miss_entry_points = allocate_empty_range<const wchar*>(arena, hitgroup_count);
	Range<const wchar*> all_exports = allocate_empty_range<const wchar*>(arena, export_count);
	Range<DXShaderDefine> defines = allocate_empty_range<DXShaderDefine>(arena, defines_count);
	
	u32 table_entry_size = 0;



	// Global

	DXRaytracingRootSignature global_root_signature = create_raytracing_root_signature(global_root_signature_desc);
	global_root_signature.root_signature.root_signature->SetName(L"Raytracing global root signature");

	{
		D3D12_STATE_SUBOBJECT& global_root_signature_so = next(subobjects);
		global_root_signature_so.pDesc = &global_root_signature.ptr;
		global_root_signature_so.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	}


	// Raygen

	DXRaytracingRootSignature raygen_root_signature = create_raytracing_root_signature(raygen_desc.root_signature_desc);
	raygen_root_signature.root_signature.root_signature->SetName(L"Raygen root signature");

	{
		D3D12_EXPORT_DESC& raygen_export = next(exports);
		raygen_export.Name = raygen_desc.entry_point;
		raygen_export.Flags = D3D12_EXPORT_FLAG_NONE;
		raygen_export.ExportToRename = 0;

		D3D12_STATE_SUBOBJECT& root_signature_so = next(subobjects);
		root_signature_so.pDesc = &raygen_root_signature.ptr;
		root_signature_so.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;

		D3D12_STATE_SUBOBJECT& association_so = next(subobjects);
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& association = next(associations);

		association.NumExports = 1;
		association.pExports = &raygen_desc.entry_point;
		association.pSubobjectToAssociate = &root_signature_so;

		association_so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		association_so.pDesc = &association;


		next(all_exports) = raygen_desc.entry_point;

		table_entry_size = max(table_entry_size, get_shader_binding_table_desc_size(raygen_desc.root_signature_desc));
	}


	// Hitgroups

	DXRaytracingRootSignature hitgroup_root_signatures[8] = {};
	u32 hitgroup_index = 0;

	for (HitGroup* hitgroup_desc = hitgroup_descs.first; hitgroup_desc; hitgroup_desc = hitgroup_desc->next)
	{
		DXRaytracingRootSignature& hitgroup_root_signature = hitgroup_root_signatures[hitgroup_index];
		hitgroup_root_signature = create_raytracing_root_signature(hitgroup_desc->root_signature_desc);
		hitgroup_root_signature.root_signature.root_signature->SetName(L"Hitgroup root signature");

		D3D12_HIT_GROUP_DESC& hitgroup = next(hitgroups);
		hitgroup.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		hitgroup.AnyHitShaderImport = hitgroup_desc->any_hit;
		hitgroup.ClosestHitShaderImport = hitgroup_desc->closest_hit;
		hitgroup.IntersectionShaderImport = 0;
		hitgroup.HitGroupExport = hitgroup_desc->group_name;

		D3D12_STATE_SUBOBJECT& hitgroup_so = next(subobjects);
		hitgroup_so.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		hitgroup_so.pDesc = &hitgroup;

		D3D12_EXPORT_DESC& miss_export = next(exports);
		miss_export.Name = hitgroup_desc->miss;
		miss_export.Flags = D3D12_EXPORT_FLAG_NONE;
		miss_export.ExportToRename = 0;

		next(all_exports) = hitgroup_desc->miss;


		if (hitgroup_desc->closest_hit)
		{
			D3D12_EXPORT_DESC& closest_hit_export = next(exports);
			closest_hit_export.Name = hitgroup_desc->closest_hit;
			closest_hit_export.Flags = D3D12_EXPORT_FLAG_NONE;
			closest_hit_export.ExportToRename = 0;

			next(all_exports) = hitgroup_desc->closest_hit;
		}

		if (hitgroup_desc->any_hit)
		{
			D3D12_EXPORT_DESC& any_hit_export = next(exports);
			any_hit_export.Name = hitgroup_desc->any_hit;
			any_hit_export.Flags = D3D12_EXPORT_FLAG_NONE;
			any_hit_export.ExportToRename = 0;

			next(all_exports) = hitgroup_desc->any_hit;
		}





		D3D12_STATE_SUBOBJECT& root_signature_so = next(subobjects);
		root_signature_so.pDesc = &hitgroup_root_signature.ptr;
		root_signature_so.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;

		D3D12_STATE_SUBOBJECT& association_so = next(subobjects);
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& association = next(associations);

		association.NumExports = (hitgroup_desc->closest_hit != 0) + (hitgroup_desc->any_hit != 0);
		association.pExports = hitgroup_desc->hits + (hitgroup_desc->closest_hit ? 0 : 1);
		association.pSubobjectToAssociate = &root_signature_so;

		association_so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		association_so.pDesc = &association;


		next(miss_entry_points) = hitgroup_desc->miss;

		next(defines) = { 0, hitgroup_desc->group_name, hitgroup_index };

		table_entry_size = max(table_entry_size, get_shader_binding_table_desc_size(hitgroup_desc->root_signature_desc));

		++hitgroup_index;
	}

	next(defines) = { 0, L"HITGROUP_COUNT", hitgroup_count };
	next(defines) = { 0, L"MAX_RECURSION_DEPTH", max_recursion_depth };

	for (DXShaderDefine* user_define = user_defines.first; user_define; user_define = user_define->next)
	{
		next(defines) = *user_define;
	}


	// Shader

	auto shader_blob = compile_library(shader_filename, defines, arena);

	D3D12_DXIL_LIBRARY_DESC dxil_lib_desc;
	dxil_lib_desc.DXILLibrary.pShaderBytecode = shader_blob->GetBufferPointer();
	dxil_lib_desc.DXILLibrary.BytecodeLength = shader_blob->GetBufferSize();
	dxil_lib_desc.NumExports = (u32)exports.count;
	dxil_lib_desc.pExports = exports.first;

	D3D12_ROOT_SIGNATURE_DESC empty_root_signature_desc = {};
	empty_root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	DXRaytracingRootSignature empty_root_signature = create_raytracing_root_signature(empty_root_signature_desc);

	{
		D3D12_STATE_SUBOBJECT& library_so = next(subobjects);
		library_so.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		library_so.pDesc = &dxil_lib_desc;

		D3D12_STATE_SUBOBJECT& root_signature_so = next(subobjects);
		root_signature_so.pDesc = &empty_root_signature.ptr;
		root_signature_so.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;

		D3D12_STATE_SUBOBJECT& association_so = next(subobjects);
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& association = next(associations);

		association.NumExports = (u32)miss_entry_points.count;
		association.pExports = miss_entry_points.first;
		association.pSubobjectToAssociate = &root_signature_so;

		association_so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		association_so.pDesc = &association;
	}



	D3D12_RAYTRACING_SHADER_CONFIG shader_config;
	shader_config.MaxAttributeSizeInBytes = sizeof(float) * 2; // 2 floats for the BuiltInTriangleIntersectionAttributes.
	shader_config.MaxPayloadSizeInBytes = payload_size;

	{
		D3D12_STATE_SUBOBJECT& shader_config_so = next(subobjects);
		shader_config_so.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		shader_config_so.pDesc = &shader_config;


		D3D12_STATE_SUBOBJECT& association_so = next(subobjects);
		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& association = next(associations);

		association.NumExports = (u32)all_exports.count;
		association.pExports = all_exports.first;
		association.pSubobjectToAssociate = &shader_config_so;

		association_so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		association_so.pDesc = &association;
	}



	D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config;
	pipeline_config.MaxTraceRecursionDepth = max_recursion_depth;

	{
		D3D12_STATE_SUBOBJECT& pipeline_config_so = next(subobjects);
		pipeline_config_so.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		pipeline_config_so.pDesc = &pipeline_config;
	}


	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = (u32)subobjects.count;
	desc.pSubobjects = subobjects.first;
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	DXRaytracingPipeline result;
	check_dx(dx_context.device->CreateStateObject(&desc, IID_PPV_ARGS(&result.pipeline)));
	result.root_signature = global_root_signature.root_signature;




	table_entry_size += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;




	// Shader binding table

	com<ID3D12StateObjectProperties> rtso_props;
	result.pipeline->QueryInterface(IID_PPV_ARGS(&rtso_props));


	DXRaytracingBindingTableDesc& binding_table_desc = result.binding_table_desc;
	binding_table_desc.entry_size = align_to<u32>(table_entry_size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	binding_table_desc.hitgroup_count = (u32)hitgroups.count;

	binding_table_desc.raygen = rtso_props->GetShaderIdentifier(raygen_desc.entry_point);

	for (u64 i = 0; i < hitgroups.count; ++i)
	{
		binding_table_desc.miss[i] = rtso_props->GetShaderIdentifier(miss_entry_points[i]);
		binding_table_desc.hitgroups[i] = rtso_props->GetShaderIdentifier(hitgroups[i].HitGroupExport);
	}


	binding_table_desc.raygen_offset = 0;
	binding_table_desc.miss_offset = binding_table_desc.raygen_offset + align_to<u64>(table_entry_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	binding_table_desc.hit_offset = binding_table_desc.miss_offset + align_to<u64>(hitgroups.count * table_entry_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);


	return result;
}




