#version 400 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 position;

out vec3 UV;
void main(){	
	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  vec4(position,1);
	
	UV = position * 0.5f + 0.5;
}