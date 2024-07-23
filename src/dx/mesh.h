#pragma once

#include "texture.h"
#include "buffer.h"
#include "raytracing.h"

#include "core/range.h"
#include "core/memory.h"

struct Submesh
{
	u32 first_index;
	u32 index_count;
	u32 base_vertex;
	u32 vertex_count;
};

struct Mesh
{
	DXVertexBufferGroup vertex_buffer;
	DXIndexBuffer index_buffer;

	std::vector<Submesh> submeshes;

	DXRaytracingBLAS blas;
};

struct IndexedTriangle16
{
	u16 a, b, c;
};

struct IndexedTriangle32
{
	u32 a, b, c;
};

struct VertexAttribute
{
	vec3 normal;
	vec2 uv;
};




struct MeshBuilder
{
	using IndexedTriangle = IndexedTriangle32;
	using IndexType = decltype(IndexedTriangle::a);

	// You can call multiple of these functions on a single mesh builder. All meshes will be 
	// merged into a single vertex and index buffer. The submeshes vector describes for each 
	// pushed geometry the offset into the buffers.

	MeshBuilder& push_cube_geometry(vec3 center, vec3 radius);
	MeshBuilder& push_sphere_geometry(vec3 center, float radius);

	Mesh build();


private:
	std::tuple<Range<vec3>, Range<VertexAttribute>, Range<IndexedTriangle>> begin_primitive(u64 vertex_count, u64 triangle_count);

	Arena vertex_position_arena;
	Arena vertex_attribute_arena;
	Arena triangle_arena;

	std::vector<Submesh> submeshes;
};









Mesh create_cube_mesh(Arena& arena);
Mesh create_sphere_mesh(Arena& arena);
