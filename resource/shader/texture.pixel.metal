#include <metal_stdlib>

using namespace metal;

typedef struct rasterizer_data_t
{
    float4 position [[position]];
} rasterizer_data_t;

fragment float4
fragment_main(rasterizer_data_t in [[stage_in]])
{
    return float4(0.0, 0.0, 0.0, 1.0);
}
