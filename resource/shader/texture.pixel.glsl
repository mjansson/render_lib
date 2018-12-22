#ifdef RENDERAPI_GLES
precision mediump float;
#endif

uniform sampler2D texture;

varying vec2 frag_texcoord;

void main(void) {
	vec4 color = texture2D(texture, frag_texcoord);
	color.a = sqrt(dot(color.rgb, vec3(0.299, 0.587, 0.114)));
	gl_FragColor = color;
}
