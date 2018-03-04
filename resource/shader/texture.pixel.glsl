#ifdef RENDERAPI_GLES
precision mediump float;
#endif

uniform sampler2D tex;

varying vec2 frag_uv;

void main(void) {
	gl_FragColor = texture(tex, frag_uv);
}
