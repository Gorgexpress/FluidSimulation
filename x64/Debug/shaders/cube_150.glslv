#version 400 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexNormal_modelspace;

out vec3 normal;
out vec3 position;

uniform mat4 MVP;
uniform mat4 M;


void main(){

	// Output position of the vertex, in clip space : MVP * position
	gl_Position = MVP * vec4(vertexPosition_modelspace,1);
	normal = mat3(transpose(inverse(M))) * vertexNormal_modelspace;
	position = vec3(M* vec4(vertexPosition_modelspace, 1.0f));
}
