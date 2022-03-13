
#include <metal_stdlib>

using namespace metal;

// Vertex declaration
typedef struct vertex_t {
	vector_float4 position;
	vector_float4 color;
} vertex_t;

// Vertex shader argument buffer
struct vertex_shader_arg_t {
	const device vertex_t* vertices [[ id(0) ]];
	const float4x4 mvp [[ id(1) ]];
};

// Vertex shader to pixel shader data exchange
struct rasterizer_data_t {
	float4 position [[position]];
	half4  color;
};

vertex rasterizer_data_t
vertex_shader(uint vid [[ vertex_id ]],
              const device vertex_shader_arg_t* arg [[ buffer(0) ]]) {
	rasterizer_data_t out;

	float4 position = arg->vertices[vid].position * arg->mvp;

	out.position.xy = position.xy;
	out.position.z  = 0.0;
	out.position.w  = 1.0;

	out.color = (half4)arg->vertices[vid].color;

	return out;
}

fragment float4
pixel_shader(rasterizer_data_t in [[ stage_in ]]) {
	return (float4)in.color;
}
