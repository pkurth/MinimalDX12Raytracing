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

	Range<Submesh> submeshes;

	DXRaytracingBLAS blas;
};

struct IndexedTriangle16
{
	u16 a, b, c;
};

Mesh create_cube_mesh(Arena& arena);
Mesh create_sphere_mesh(Arena& arena);
