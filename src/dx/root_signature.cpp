#include "root_signature.h"
#include "context.h"

DXRootSignature create_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	DXBlob root_signature_blob;
	DXBlob error_blob;
	check_dx(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob));

	DXRootSignature root_signature = {};

	check_dx(dx_context.device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
		root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature.root_signature)));

	return root_signature;
}
