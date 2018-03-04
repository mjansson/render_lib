attribute vec4 position;
attribute vec2 uv;

uniform mat4 mvp;

varying vec2 frag_uv;

void main(void) {
	frag_uv = uv;
	gl_Position = position * mvp;
}
