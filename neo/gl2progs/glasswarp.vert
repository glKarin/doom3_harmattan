// !!ARBvp1.0 OPTION ARB_position_invariant ;

#version 100

attribute vec3 attr_Tangent; // 9
attribute vec3 attr_Bitangent; // 10

varying vec2 var_TexResult0;
varying vec2 var_TexResult1;

void main()
{
	var_TexResult0.x = attr_Tangent.x;
	var_TexResult1.x = attr_Bitangent.x;
}

/*
	 MOV result.texcoord[0].x, vertex.attrib[9].x;
	 MOV result.texcoord[1].x, vertex.attrib[10].x;
 */

