#ifdef RENDERAPI_GLES
precision mediump float;
#endif

uniform sampler2D texture;

varying vec2 frag_texcoord;

void main(void) {
	gl_FragColor = texture2D(texture, frag_texcoord);
}
