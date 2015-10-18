#version 400 core

// Interpolated values from the vertex shaders
in vec3 UV;

//Output data
out vec3 color;

//Values that stay constant for the whole mesh.
uniform samplerCube cubeMap; //cubemap containing texture of the room

void main(){
    //just get the color of the texture at the given UV coords
	color = texture(cubeMap, UV).rgb;

}