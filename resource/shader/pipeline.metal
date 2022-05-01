
#include <metal_stdlib>

using namespace metal;

// Render arguments
struct render_argument_t {
    uint index_count;
    uint instance_count;
    uint index_offset;
    uint vertex_offset;
    uint instance_offset;
};

// Queued render command
struct render_command_t {
	uint pipeline_state;
    uint argument_buffer;
    uint argument_offset;
	uint index_buffer;
	uint descriptor[4];
};

struct render_buffer_t {
	device void* render_buffer;
};

struct render_buffer_storage_t {
	const device render_buffer_t* buffers[32];
};

struct pipeline_state_storage_t {
    const render_pipeline_state pipeline_state[32];
};

// Compute shader argument buffers
struct encode_data_t {
    command_buffer indirect_buffer [[ id(0) ]];
};

kernel void
encoding_kernel_index16(uint command_index [[ thread_position_in_grid ]],
               const device   render_command_t*         commands [[ buffer(0) ]],
               const device   render_buffer_storage_t*  buffer_storage [[ buffer(1) ]],
               const device   pipeline_state_storage_t* state_storage [[ buffer(2) ]],
               device         encode_data_t*            encode [[ buffer(3) ]])
{
    const render_command_t command = commands[command_index];

    render_command cmd(encode->indirect_buffer, command_index);

    cmd.set_render_pipeline_state(state_storage->pipeline_state[command.pipeline_state]);

    render_argument_t argument = ((const device render_argument_t*)buffer_storage->buffers[command.argument_buffer])[command.argument_offset];

    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[0]], 0); 
    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[1]], 1);
    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[2]], 2);
    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[3]], 3);

    cmd.draw_indexed_primitives(primitive_type::triangle,
                                argument.index_count,
                                (device ushort*)((const device char*)buffer_storage->buffers[command.index_buffer] + argument.index_offset),
                                argument.instance_count,
                                argument.vertex_offset,
                                argument.instance_offset);
}

kernel void
encoding_kernel_index32(uint command_index [[ thread_position_in_grid ]],
               const device   render_command_t*         commands [[ buffer(0) ]],
               const device   render_buffer_storage_t*  buffer_storage [[ buffer(1) ]],
               const device   pipeline_state_storage_t* state_storage [[ buffer(2) ]],
               device         encode_data_t*            encode [[ buffer(3) ]])
{
    const render_command_t command = commands[command_index];

    render_command cmd(encode->indirect_buffer, command_index);

    cmd.set_render_pipeline_state(state_storage->pipeline_state[command.pipeline_state]);

    render_argument_t argument = ((const device render_argument_t*)buffer_storage->buffers[command.argument_buffer])[command.argument_offset];

    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[0]], 0); 
    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[1]], 1);
    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[2]], 2);
    cmd.set_vertex_buffer(buffer_storage->buffers[command.descriptor[3]], 3);

    cmd.draw_indexed_primitives(primitive_type::triangle,
                                argument.index_count,
                                (device uint*)((const device char*)buffer_storage->buffers[command.index_buffer] + argument.index_offset),
                                argument.instance_count,
                                argument.vertex_offset,
                                argument.instance_offset);
}
