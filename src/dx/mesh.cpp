#include "mesh.h"
#include "raytracing.h"
#include "core/math.h"


MeshBuilder& MeshBuilder::push_cube_geometry(vec3 center, vec3 radius)
{
	Submesh& submesh = begin_primitive();


	Range<vec3> vertex_positions = vertex_position_arena.allocate_range<vec3>(24);
	Range<VertexAttribute> vertex_attributes = vertex_attribute_arena.allocate_range<VertexAttribute>(24);
	Range<IndexedTriangle> triangles = triangle_arena.allocate_range<IndexedTriangle>(24);

	auto push_vertex = [&, push_index = 0u](vec3 position, vec3 normal, vec2 uv) mutable
	{
		vertex_positions[push_index] = position + center;
		vertex_attributes[push_index] = { normal, uv };
		++push_index;
	};

	auto push_triangle = [&, push_index = 0u](u32 a, u32 b, u32 c) mutable
	{
		triangles[push_index++] = { (IndexType)a, (IndexType)b, (IndexType)c };
	};


	vec3 x = vec3(radius.x, 0.f, 0.f);
	vec3 y = vec3(0.f, radius.y, 0.f);
	vec3 z = vec3(0.f, 0.f, radius.z);

	push_vertex(- x - y + z,  z, vec2(0.f, 0.f));
	push_vertex(  x - y + z,  z, vec2(1.f, 0.f));
	push_vertex(- x + y + z,  z, vec2(0.f, 1.f));
	push_vertex(  x + y + z,  z, vec2(1.f, 1.f));
	push_vertex(  x - y + z,  x, vec2(0.f, 0.f));
	push_vertex(  x - y - z,  x, vec2(1.f, 0.f));
	push_vertex(  x + y + z,  x, vec2(0.f, 1.f));
	push_vertex(  x + y - z,  x, vec2(1.f, 1.f));
	push_vertex(  x - y - z, -z, vec2(0.f, 0.f));
	push_vertex(- x - y - z, -z, vec2(1.f, 0.f));
	push_vertex(  x + y - z, -z, vec2(0.f, 1.f));
	push_vertex(- x + y - z, -z, vec2(1.f, 1.f));
	push_vertex(- x - y - z, -x, vec2(0.f, 0.f));
	push_vertex(- x - y + z, -x, vec2(1.f, 0.f));
	push_vertex(- x + y - z, -x, vec2(0.f, 1.f));
	push_vertex(- x + y + z, -x, vec2(1.f, 1.f));
	push_vertex(- x + y + z,  y, vec2(0.f, 0.f));
	push_vertex(  x + y + z,  y, vec2(1.f, 0.f));
	push_vertex(- x + y - z,  y, vec2(0.f, 1.f));
	push_vertex(  x + y - z,  y, vec2(1.f, 1.f));
	push_vertex(- x - y - z, -y, vec2(0.f, 0.f));
	push_vertex(  x - y - z, -y, vec2(1.f, 0.f));
	push_vertex(- x - y + z, -y, vec2(0.f, 1.f));
	push_vertex(  x - y + z, -y, vec2(1.f, 1.f));

	push_triangle( 0,  1,  2);
	push_triangle( 1,  3,  2);
	push_triangle( 4,  5,  6);
	push_triangle( 5,  7,  6);
	push_triangle( 8,  9, 10);
	push_triangle( 9, 11, 10);
	push_triangle(12, 13, 14);
	push_triangle(13, 15, 14);
	push_triangle(16, 17, 18);
	push_triangle(17, 19, 18);
	push_triangle(20, 21, 22);
	push_triangle(21, 23, 22);

	submesh.vertex_count = (u32)vertex_positions.count;
	submesh.index_count = (u32)triangles.count * 3;

	return *this;
}

MeshBuilder& MeshBuilder::push_sphere_geometry(vec3 center, float radius)
{
	Submesh& submesh = begin_primitive();



	constexpr u32 slices = 15;
	constexpr u32 rows = 15;

	constexpr float vert_delta_angle = M_PI / (rows + 1);
	constexpr float horz_delta_angle = 2.f * M_PI / slices;

	constexpr u32 vertex_count = slices * rows + 2;
	constexpr u32 triangle_count = 2 * rows * slices;

	Range<vec3> vertex_positions = vertex_position_arena.allocate_range<vec3>(vertex_count);
	Range<VertexAttribute> vertex_attributes = vertex_attribute_arena.allocate_range<VertexAttribute>(vertex_count);
	Range<IndexedTriangle> triangles = triangle_arena.allocate_range<IndexedTriangle>(triangle_count);

	auto push_vertex = [&, push_index = 0u](vec3 position) mutable
	{
		ASSERT(push_index < triangle_count);

		vec3 normal = normalize(position);
		vec2 uv = direction_to_panorama_uv(normal);

		vertex_positions[push_index] = position + center;
		vertex_attributes[push_index] = { normal, uv };
		++push_index;
	};

	auto push_triangle = [&, push_index = 0u](u32 a, u32 b, u32 c) mutable
	{
		triangles[push_index++] = { (IndexType)a, (IndexType)b, (IndexType)c };
	};


	push_vertex(vec3(0.f, -radius, 0.f));

	for (u32 y = 0; y < rows; ++y)
	{
		float vert_angle = (y + 1) * vert_delta_angle - M_PI;
		float vertex_y = cos(vert_angle);
		float current_circle_radius = sin(vert_angle);
		for (u32 x = 0; x < slices; ++x)
		{
			float horz_angle = x * horz_delta_angle;
			float vertex_x = cos(horz_angle) * current_circle_radius;
			float vertex_z = sin(horz_angle) * current_circle_radius;

			push_vertex(vec3(vertex_x, vertex_y, vertex_z) * radius);
		}
	}

	push_vertex(vec3(0.f, radius, 0.f));

	u32 last_vertex_index = slices * rows + 2;

	for (u32 x = 0; x < slices - 1; ++x)
	{
		push_triangle(0, x + 1, x + 2);
	}
	push_triangle(0, slices, 1);

	for (u32 y = 0; y < rows - 1; ++y)
	{
		for (u32 x = 0; x < slices - 1; ++x)
		{
			push_triangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
			push_triangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
		}
		push_triangle(y * slices + slices, (y + 1) * slices + 1, y * slices + 1);
		push_triangle(y * slices + slices, (y + 1) * slices + slices, (y + 1) * slices + 1);
	}
	for (u32 x = 0; x < slices - 1; ++x)
	{
		push_triangle(last_vertex_index - 2 - x, last_vertex_index - 3u - x, last_vertex_index - 1);
	}
	push_triangle(last_vertex_index - 1 - slices, last_vertex_index - 2, last_vertex_index - 1);


	submesh.vertex_count = (u32)vertex_positions.count;
	submesh.index_count = (u32)triangles.count * 3;

	return *this;
}

Mesh MeshBuilder::build()
{
	ASSERT(vertex_position_arena.current % sizeof(vec3) == 0);
	ASSERT(vertex_attribute_arena.current % sizeof(VertexAttribute) == 0);
	ASSERT(triangle_arena.current % sizeof(IndexedTriangle) == 0);

	u64 vertex_count = vertex_position_arena.current / sizeof(vec3);
	u64 triangle_count = triangle_arena.current / sizeof(IndexedTriangle);

	Range<vec3> vertex_positions = { (vec3*)vertex_position_arena.memory, vertex_count };
	Range<VertexAttribute> vertex_attributes = { (VertexAttribute*)vertex_attribute_arena.memory, vertex_count };
	Range<IndexedTriangle> triangles = { (IndexedTriangle*)triangle_arena.memory, triangle_count };

	DXVertexBufferGroup vertex_buffer =
	{
		create_buffer(vertex_positions, false, L"Vertex position buffer"),
		create_buffer(vertex_attributes, false, L"Vertex attribute buffer"),
	};

	DXIndexBuffer index_buffer = create_buffer(triangles.cast<IndexType>(), false, L"Index buffer");

	Mesh result =
	{
		vertex_buffer,
		index_buffer,
		submeshes,
	};

	{
		Arena& temp_arena = vertex_position_arena;

		ArenaMarker marker(temp_arena);
		create_raytracing_blas(result, temp_arena);
	}
	return result;
}

Submesh& MeshBuilder::begin_primitive()
{
	// Each submesh index buffer must be aligned to a 16 byte boundary.
	// This calculation wastes a bit of space, but is easy.
	triangle_arena.align_next_to(alignof(IndexedTriangle) * 16);

	Submesh& submesh = submeshes.emplace_back();
	submesh.base_vertex = (u32)(vertex_position_arena.current / sizeof(vec3));
	submesh.first_index = (u32)(triangle_arena.current / sizeof(IndexType));
	return submesh;
}

Mesh create_cube_mesh(Arena& arena)
{
	return MeshBuilder()
		.push_cube_geometry(vec3(0.f, 0.f, 0.f), vec3(1.f, 1.f, 1.f))
		.build();
}

Mesh create_sphere_mesh(Arena& arena)
{
	return MeshBuilder()
		.push_sphere_geometry(vec3(0.f, 0.f, 0.f), 1.f)
		.build();
}