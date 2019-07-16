#version 100

uniform sampler2D u_fragmentMap0;	/* u_bumpTexture */
uniform sampler2D u_fragmentMap1;	/* u_lightFalloffTexture */
uniform sampler2D u_fragmentMap2;	/* u_lightProjectionTexture */

varying vec4 var_TexResult0;
varying vec4 var_TexResult1;

void main()
{
	const vec4 one = { 1.0, 1.0, 1.0, 1.0 };
	const vec4 two = { 2.0, 2.0, 2.0, 1.0 };
	const vec4 zero = { 0.0, 0.0, 0.0, 0.0 };
	const vec4 _default = { 0.0, 0.0, 0.0, 1.0 };
	const vec4 pointFive	= { 0.5, 0.5, 0.5, 1.0 };

	const vec4 preturbScale = { 0.2, 0.2, 0.0, 1.0 };
	const vec4 faceScale = { 0.1, 0.1, 0.1, 1.0 };
	const vec4 colorShift = { 0.1, 0.1, 0.2, 1.0 };
	const vec4 finalColorScale = { 0.6, 0.6, 1.0, 1.0 };


	vec4 tc = _default;
	vec4 preturb = _default

	tc = var_TexResult0;
	tc.xy -= pointFive.xy;
	tc.xy *= two.xy;

	vec4 temp = texture2D(u_fragmentMap0, tc.xy);
	vec4 scaler = temp;

	temp *= preturbScale
	preturb.xyz = one.xyz - temp.xyz;

	tc = var_TexResult1;
	temp = tc;
	temp.w = 1.0 / temp.w;
	tc.w = 1.0;
	tc.xy *= temp.w;

	tc.xy -= pointFive.xy;
	tc.xy *= preturb.xy;
	tx.xy += pointFive.xy;

	vec4 color = texture2D(u_fragmentMap1, tc.xy);
	vec4 color2 = texture2D(u_fragmentMap2, tc.xy);

	color2 *= faceScale;

	vec4 greyScale = color.xxxx;
	greyScale = finalColorScale * greyScale;
	temp = greyScale * scaler.x;
	tc = one - scaler;
	color *= tc.x;
	color += temp;

	color += color2;

	gl_FragColor = color;
}
