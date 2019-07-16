#version 100
//#pragma optimize(off)

precision mediump float;

uniform sampler2D u_fragmentMap0;
uniform float u_alpha;

varying vec2 var_TexCoord;

void main(void)
{
	vec4 color = texture2D(u_fragmentMap0, var_TexCoord);
	gl_FragColor = vec4(color.rgb, color.a * u_alpha);
	//gl_FragColor = vec4(1.0,0.0,0.0,1.0);
}

