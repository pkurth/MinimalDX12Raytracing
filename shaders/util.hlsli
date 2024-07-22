

#define M_PI float(3.14159265358979323846)

static float2 interpolate_attribute(float2 vertex_attribute[3], BuiltInTriangleIntersectionAttributes attribs)
{
    return vertex_attribute[0] +
		attribs.barycentrics.x * (vertex_attribute[1] - vertex_attribute[0]) +
		attribs.barycentrics.y * (vertex_attribute[2] - vertex_attribute[0]);
}

static float3 interpolate_attribute(float3 vertex_attribute[3], BuiltInTriangleIntersectionAttributes attribs)
{
    return vertex_attribute[0] +
		attribs.barycentrics.x * (vertex_attribute[1] - vertex_attribute[0]) +
		attribs.barycentrics.y * (vertex_attribute[2] - vertex_attribute[0]);
}

static float4 interpolate_attribute(float4 vertex_attribute[3], BuiltInTriangleIntersectionAttributes attribs)
{
    return vertex_attribute[0] +
		attribs.barycentrics.x * (vertex_attribute[1] - vertex_attribute[0]) +
		attribs.barycentrics.y * (vertex_attribute[2] - vertex_attribute[0]);
}

static uint3 load_3x16_bit_indices(ByteAddressBuffer mesh_indices)
{
    const uint index_size_in_bytes = 2;
    const uint indices_per_triangle = 3;
    const uint triangle_index_stride = indices_per_triangle * index_size_in_bytes;
    uint base_index = PrimitiveIndex() * triangle_index_stride;

    uint3 indices;

	// ByteAdressBuffer loads must be aligned at a 4 byte boundary.
	// Since we need to read three 16 bit indices: { 0, 1, 2 } 
	// aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
	// we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
	// based on first index's base_index being aligned at the 4 byte boundary or not:
	//  Aligned:     { 0 1 | 2 - }
	//  Not aligned: { - 0 | 1 2 }
    const uint dword_aligned_offset = base_index & ~3;
    const uint2 four_16_bit_indices = mesh_indices.Load2(dword_aligned_offset);

	// Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dword_aligned_offset == base_index)
    {
        indices.x = four_16_bit_indices.x & 0xffff;
        indices.y = (four_16_bit_indices.x >> 16) & 0xffff;
        indices.z = four_16_bit_indices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four_16_bit_indices.x >> 16) & 0xffff;
        indices.y = four_16_bit_indices.y & 0xffff;
        indices.z = (four_16_bit_indices.y >> 16) & 0xffff;
    }

    return indices;
}

static uint3 load_3x32_bit_indices(ByteAddressBuffer mesh_indices)
{
    const uint index_size_in_bytes = 4;
    const uint indices_per_triangle = 3;
    const uint triangle_index_stride = indices_per_triangle * index_size_in_bytes;
    uint base_index = PrimitiveIndex() * triangle_index_stride;

    uint3 indices = mesh_indices.Load3(base_index);

    return indices;
}

static float3 local_direction_to_world(float3 direction)
{
    float3x4 M = ObjectToWorld3x4();
    return normalize(mul(M, float4(direction, 0.f)).xyz);
}
