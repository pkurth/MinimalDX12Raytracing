#include "mesh.h"
#include "raytracing.h"
#include "core/math.h"

struct VertexAttribute
{
	vec3 normal;
	vec2 uv;
};

Mesh create_cube_mesh(Arena& arena)
{
	vec3 x = vec3(1.f, 0.f, 0.f);
	vec3 y = vec3(0.f, 1.f, 0.f);
	vec3 z = vec3(0.f, 0.f, 1.f);

	const vec3 vertex_positions[] =
	{
		- x - y + z,
		  x - y + z,
		- x + y + z,
		  x + y + z,
		  x - y + z,
		  x - y - z,
		  x + y + z,
		  x + y - z,
		  x - y - z,
		- x - y - z,
		  x + y - z,
		- x + y - z,
		- x - y - z,
		- x - y + z,
		- x + y - z,
		- x + y + z,
		- x + y + z,
		  x + y + z,
		- x + y - z,
		  x + y - z,
		- x - y - z,
		  x - y - z,
		- x - y + z,
		  x - y + z,
	};

	const VertexAttribute vertex_attributes[] =
	{
		 {  z, vec2(0.f, 0.f) },
		 {  z, vec2(1.f, 0.f) },
		 {  z, vec2(0.f, 1.f) },
		 {  z, vec2(1.f, 1.f) },
		 {  x, vec2(0.f, 0.f) },
		 {  x, vec2(1.f, 0.f) },
		 {  x, vec2(0.f, 1.f) },
		 {  x, vec2(1.f, 1.f) },
		 { -z, vec2(0.f, 0.f) },
		 { -z, vec2(1.f, 0.f) },
		 { -z, vec2(0.f, 1.f) },
		 { -z, vec2(1.f, 1.f) },
		 { -x, vec2(0.f, 0.f) },
		 { -x, vec2(1.f, 0.f) },
		 { -x, vec2(0.f, 1.f) },
		 { -x, vec2(1.f, 1.f) },
		 {  y, vec2(0.f, 0.f) },
		 {  y, vec2(1.f, 0.f) },
		 {  y, vec2(0.f, 1.f) },
		 {  y, vec2(1.f, 1.f) },
		 { -y, vec2(0.f, 0.f) },
		 { -y, vec2(1.f, 0.f) },
		 { -y, vec2(0.f, 1.f) },
		 { -y, vec2(1.f, 1.f) },
	};

	const IndexedTriangle16 triangles[] =
	{
		{  0,  1,  2 },
		{  1,  3,  2 },
		{  4,  5,  6 },
		{  5,  7,  6 },
		{  8,  9, 10 },
		{  9, 11, 10 },
		{ 12, 13, 14 },
		{ 13, 15, 14 },
		{ 16, 17, 18 },
		{ 17, 19, 18 },
		{ 20, 21, 22 },
		{ 21, 23, 22 },
	};

	DXVertexBufferGroup vertex_buffer =
	{
		DXVertexBuffer(create_buffer(Range(vertex_positions), false, L"Cube vertex positions")),
		DXVertexBuffer(create_buffer(Range(vertex_attributes), false, L"Cube vertex attributes")),
	};

	DXIndexBuffer index_buffer = create_buffer(Range(triangles).cast<u16>(), false, L"Cube indices");

	Range<Submesh> submeshes = arena.allocate_range<Submesh>(1);
	submeshes[0].first_index = 0;
	submeshes[0].index_count = arraysize(triangles) * 3;
	submeshes[0].base_vertex = 0;
	submeshes[0].vertex_count = arraysize(vertex_positions);

	Mesh result = 
	{ 
		vertex_buffer,
		index_buffer,
		submeshes,
	};

	create_raytracing_blas(result, arena);

	return result;
}

Mesh create_sphere_mesh(Arena& arena)
{
	constexpr u32 slices = 15;
	constexpr u32 rows = 15;
	constexpr float radius = 1.f;

	constexpr float vert_delta_angle = M_PI / (rows + 1);
	constexpr float horz_delta_angle = 2.f * M_PI / slices;

	constexpr u32 vertex_count = slices * rows + 2;
	constexpr u32 triangle_count = 2 * rows * slices;



	DXVertexBufferGroup vertex_buffer;
	DXIndexBuffer index_buffer;


	{
		ArenaMarker marker(arena);

		Range<vec3> vertex_positions = arena.allocate_range<vec3>(vertex_count);
		Range<VertexAttribute> vertex_attributes = arena.allocate_range<VertexAttribute>(vertex_count);
		Range<IndexedTriangle16> triangles = arena.allocate_range<IndexedTriangle16>(triangle_count);

		auto push_vertex = [&, push_index = 0](vec3 position) mutable
		{
			vec3 normal = normalize(position);
			vec2 uv = direction_to_panorama_uv(normal);

			vertex_positions[push_index] = position;
			vertex_attributes[push_index] = { normal, uv };
			++push_index;
		};

		auto push_triangle = [&, push_index = 0](u32 a, u32 b, u32 c) mutable
		{
			triangles[push_index++] = { (u16)a, (u16)b, (u16)c };
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


		vertex_buffer =
		{
			DXVertexBuffer(create_buffer(vertex_positions, false, L"Sphere vertex positions")),
			DXVertexBuffer(create_buffer(vertex_attributes, false , L"Sphere vertex attributes")),
		};

		index_buffer = create_buffer(triangles.cast<u16>(), false, L"Sphere indices");
	}

	Range<Submesh> submeshes = arena.allocate_range<Submesh>(1);
	submeshes[0].first_index = 0;
	submeshes[0].index_count = triangle_count * 3;
	submeshes[0].base_vertex = 0;
	submeshes[0].vertex_count = vertex_count;

	Mesh result =
	{
		vertex_buffer,
		index_buffer,
		submeshes,
	};

	create_raytracing_blas(result, arena);

	return result;
}