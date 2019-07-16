#version 100
//#pragma optimize(off)

precision mediump float;

attribute vec4 attr_TexCoord;
attribute vec4 attr_Vertex;

uniform mat4 u_modelViewProjectionMatrix;

varying vec2 var_TexCoord;

void main(void)
{
	var_TexCoord = attr_TexCoord.xy;
	gl_Position = u_modelViewProjectionMatrix * attr_Vertex;
}

