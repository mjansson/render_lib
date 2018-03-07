attribute vec4 position;
attribute vec2 texcoord;

uniform mat4 mvp;

varying vec2 frag_texcoord;

void main(void) {
	frag_texcoord = texcoord;
	gl_Position = position * mvp;
}
