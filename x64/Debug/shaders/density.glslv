#version 400 core
//This program should be used to output to a texture using drawarrays,
//using gl points with count being the depth of the 3d texture.
void main(){	
	//value of gl_vertexID equals the layer of the texture to midfy
	//TODO - should be able to get rid of gl_Position and use an out int instead.
	gl_Position = vec4(gl_VertexID, 0.0f, 0.0f, 0.0f);
}