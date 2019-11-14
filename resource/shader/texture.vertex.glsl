attribute vec4 position;
attribute vec2 texcoord;

uniform mat4 transform_mvp;

varying vec2 frag_texcoord;

void main(void) {
	frag_texcoord = texcoord;
	gl_Position = position * transform_mvp;
}
