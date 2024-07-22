
#include "util.hlsli"

// -----------------------------
// GLOBAL PARAMETERS
// -----------------------------

struct Constants
{
	float4x4 camera_transform;
	float3 light_direction;
	float padding0;
	float3 light_color;
	float padding1;
	float3 sky_color;
	float padding2;
};

ConstantBuffer<Constants> constants						: register(b0, space0);
RaytracingAccelerationStructure scene_tlas				: register(t0, space0);
RWTexture2D<float4> output								: register(u0, space0);



// -----------------------------
// RADIANCE HITGROUP PARAMETERS
// -----------------------------


struct RenderMaterial
{
	float3 color;
};

struct VertexAttribute
{
	float3 normal;
	float2 uv;
};

ConstantBuffer<RenderMaterial> material					: register(b0, space1);
StructuredBuffer<VertexAttribute> vertex_attributes		: register(t0, space1);
ByteAddressBuffer indices								: register(t1, space1);







struct RadiancePayload
{
	float3 color;
	uint recursion;
};

struct ShadowPayload
{
	float visible;
};

static float3 trace_radiance_ray(float3 origin, float3 direction, uint recursion)
{
	if (recursion >= MAX_RECURSION_DEPTH)
	{
		return float3(0, 0, 0);
	}

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.01f;
	ray.TMax = 10000.f;

	RadiancePayload payload = { float3(0.f, 0.f, 0.f), recursion + 1 };

	TraceRay(scene_tlas,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF,				// Cull mask.
		RADIANCE,			// Addend on the hit index.
		HITGROUP_COUNT,		// Multiplier on the geometry index within a BLAS.
		RADIANCE,			// Miss index.
		ray,
		payload);

	return payload.color;
}

static float trace_shadow_ray(float3 origin, float3 direction, uint recursion)
{
	if (recursion >= MAX_RECURSION_DEPTH)
	{
		return 1.f;
	}

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.01f;
	ray.TMax = 10000.f;

	ShadowPayload payload = { 0.f };

	TraceRay(scene_tlas,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, // No need to invoke closest hit shader.
		0xFF,				// Cull mask.
		SHADOW,				// Addend on the hit index.
		HITGROUP_COUNT,		// Multiplier on the geometry index within a BLAS.
		SHADOW,				// Miss index.
		ray,
		payload);

	return payload.visible;
}

static float3 view_dir(const uint2 xy, const uint2 wh, float cam_fov)
{
	const float2 pixel = (xy - wh * 0.5f) / wh.yy;
	const float z = -0.5f / tan(0.5f * M_PI * cam_fov / 180.f);
	return normalize(float3(pixel.x, -pixel.y, z));
}

[shader("raygeneration")]
void raygen()
{
	uint2 launch_index = DispatchRaysIndex().xy;
	uint2 launch_dim   = DispatchRaysDimensions().xy;

	float3 origin = mul(constants.camera_transform, float4(0.f, 0.f, 0.f, 1.f)).xyz;
	float3 direction = mul(constants.camera_transform, float4(view_dir(launch_index, launch_dim, 60.f), 0.f)).xyz;

	float3 color = trace_radiance_ray(origin, direction, 0);

	output[launch_index] = float4(color, 1.f);
}

static float3 get_world_normal(uint3 tri, BuiltInTriangleIntersectionAttributes attribs)
{
	float3 normals[] = { vertex_attributes[tri.x].normal, vertex_attributes[tri.y].normal, vertex_attributes[tri.z].normal };
	return local_direction_to_world(interpolate_attribute(normals, attribs));
}

static float2 get_uv(uint3 tri, BuiltInTriangleIntersectionAttributes attribs)
{
	float2 uvs[] = { vertex_attributes[tri.x].uv, vertex_attributes[tri.y].uv, vertex_attributes[tri.z].uv };
	return interpolate_attribute(uvs, attribs);
}

[shader("closesthit")]
void radiance_closest_hit(inout RadiancePayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	float3 P = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

	uint3 tri = load_3x16_bit_indices(indices);
	
	float3 N = get_world_normal(tri, attribs);
	float3 L = -constants.light_direction;

	// Trace shadow ray
	float light_visible = trace_shadow_ray(P, L, payload.recursion);

	float NdotL = saturate(dot(N, L));
	float3 lighting = material.color * constants.light_color * NdotL * light_visible;

	// Trace bounce ray
	float3 reflection = trace_radiance_ray(P, reflect(WorldRayDirection(), N), payload.recursion);

	payload.color = lighting + reflection * 0.2f;
}

[shader("miss")]
void radiance_miss(inout RadiancePayload payload)
{
	payload.color = constants.sky_color;
}



[shader("miss")]
void shadow_miss(inout ShadowPayload payload)
{
	payload.visible = 1.f;
}
