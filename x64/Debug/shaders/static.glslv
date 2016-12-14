#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 position;

out vec3 UV;

uniform mat4 MVP;

void main(){	
	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(position,1);
	
	UV = position;
}