#version 330 core

// Interpolated values from the vertex shaders
in vec3 UV;

//Output data
out vec3 color;

//Values that stay constant for the whole mesh.
uniform samplerCube cubeMap;

void main(){
	color = texture(cubeMap, UV).rgb;

}