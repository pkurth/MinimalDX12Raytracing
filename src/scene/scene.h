#pragma once

#include "core/memory.h"
#include "core/math.h"

#include "renderer.h"

#include "dx/mesh.h"
#include "dx/raytracing.h"

struct SceneObject
{
	Mesh mesh;
	vec3 color;
	Transform transform;
};

struct Scene
{
	void push_object(const Mesh& mesh, vec3 color, vec3 position, quat rotation, vec3 scale = vec3(1.f, 1.f, 1.f));

	void build();

	std::shared_ptr<DXTexture> render(u32 render_width, u32 render_height);

private:

	u64 total_submesh_count();

	void build_binding_table();
	void build_tlas();

	std::vector<SceneObject> objects;

	Arena temp_arena;

	Renderer renderer;

	DXRaytracingTLAS tlas;
	DXRaytracingBindingTable binding_table;
	DXDescriptorHeap descriptor_heap;
};

