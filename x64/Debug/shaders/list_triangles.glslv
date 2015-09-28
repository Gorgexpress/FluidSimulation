#version 400 core

out uint z6_y6_x6_null6_cubeIndex8;

uniform sampler3D sField;
uniform ivec3 dimensions;

void main(){	
	ivec3 gridPosition;
	gridPosition.x = gl_VertexID % dimensions.x;
	gridPosition.y = (gl_VertexID / dimensions.x) % dimensions.y;
	gridPosition.z = ((gl_VertexID / dimensions.x) / dimensions.y ) % dimensions.z;
	
	
	float density[8];
	density[0] = texelFetch(sField, gridPosition + ivec3(0, 0, 1), 0).r;
	density[1] = texelFetch(sField, gridPosition + ivec3(1, 0, 1), 0).r;
	density[2] = texelFetch(sField, gridPosition + ivec3(1, 0, 0), 0).r;
	density[3] = texelFetch(sField, gridPosition, 0).r;
	density[4] = texelFetch(sField, gridPosition + ivec3(0, 1, 1), 0).r;
	density[5] = texelFetch(sField, gridPosition + ivec3(1, 1, 1), 0).r;
	density[6] = texelFetch(sField, gridPosition + ivec3(1, 1, 0), 0).r;
	density[7] = texelFetch(sField, gridPosition + ivec3(0, 1, 0), 0).r;
	
	uint cubeIndex = 0;
	
	if(density[0] < 0.0001) cubeIndex |= 1;
	if(density[1] < 0.0001) cubeIndex |= 2;
	if(density[2] < 0.0001) cubeIndex |= 4;
	if(density[3] < 0.0001) cubeIndex |= 8;
	if(density[4] < 0.0001) cubeIndex |= 16;
	if(density[5] < 0.0001) cubeIndex |= 32;
	if(density[6] < 0.0001) cubeIndex |= 64;
	if(density[7] < 0.0001) cubeIndex |= 128;
	
	z6_y6_x6_null6_cubeIndex8 = (gridPosition.z << 26) |
							   (gridPosition.y << 20) |
							   (gridPosition.x << 14 ) |
							   (cubeIndex			);
							   
}