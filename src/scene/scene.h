#pragma once

#include "core/memory.h"
#include "core/math.h"
#include "core/input.h"

#include "renderer.h"

#include "dx/mesh.h"
#include "dx/raytracing.h"

struct SceneObject
{
	Mesh mesh;
	vec3 color;
	Transform transform;
};

struct Camera
{
	vec3 center = vec3(0.f, 0.f, 0.f);
	quat rotation = quat::identity;
	float orbit_radius = 7.f;

	vec3 position() const { return center + (rotation * vec3(0.f, 0.f, 1.f) * orbit_radius); }

	void update(const Input& input, u32 viewport_width, u32 viewport_height, float dt);
};

struct Scene
{
	void push_object(const Mesh& mesh, vec3 color, vec3 position, quat rotation, vec3 scale = vec3(1.f, 1.f, 1.f));

	void build();

	void update(const Input& input, u32 render_width, u32 render_height, float dt);
	std::shared_ptr<DXTexture> render(u32 render_width, u32 render_height);

private:

	u64 total_submesh_count();

	void build_binding_table();
	void build_tlas();

	std::vector<SceneObject> objects;

	Arena temp_arena;

	Renderer renderer;

	Camera camera;

	DXRaytracingTLAS tlas;
	DXRaytracingBindingTable binding_table;
	DXDescriptorHeap descriptor_heap;
};

