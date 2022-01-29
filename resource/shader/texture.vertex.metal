#include <metal_stdlib>

using namespace metal;

typedef struct rasterizer_data_t
{
    float4 position [[position]];
    float2 texcoord;
} rasterizer_data_t;

typedef struct vertex_input_t
{
    float4 position;
    float2 texcoord;
} vertex_input_t;

typedef struct uniform_t
{
    float4x4 transform_mvp;
} uniform_t;

vertex rasterizer_data_t
vertex_main(uint vertexid [[ vertex_id ]],
            constant vertex_input_t* vertex_array [[ buffer(0) ]],
            constant uniform_t& uniforms [[ buffer(1) ]])

{
    rasterizer_data_t out;
	
	out.texcoord = vertex_array[vertexid].texcoord;
	out.position = vertex_array[vertexid].position * uniforms.transform_mvp;

    return out;
}
