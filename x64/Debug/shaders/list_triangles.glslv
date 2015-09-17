#version 400 core

out ivec3 gridPosition;
out uint z6_y6_x6_null6_cubeIndex8

uniform sampler3D sField;
uniform ivec3 dimensions;

void main(){	
	gridPosition.x = gl_VertexID % dimensions.x;
	gridPosition.y = (gl_VertexID / dimensions.x) % dimensions.y
	gridPosition.z = ((gl_VertexID . dimensions.x) / dimensions.y ) % dimensions.z;
	
	
	float density[8];
	density[0] = texelFetch(particles, gridPosition + ivec3(0, 0, 1), 0).rgb;
	density[1] = texelFetch(particles, gridPosition + ivec3(1, 0, 1), 0).rgb;
	density[2] = texelFetch(particles, gridPosition + ivec3(1, 0, 0), 0).rgb;
	density[3] = texelFetch(particles, gridPosition, 0).rgb;
	density[4] = texelFetch(particles, gridPosition + ivec3(0, 1, 1), 0).rgb;
	density[5] = texelFetch(particles, gridPosition + ivec3(1, 1, 1), 0).rgb;
	density[6] = texelFetch(particles, gridPosition + ivec3(1, 1, 0), 0).rgb;
	density[7] = texelFetch(particles, gridPosition + ivec3(0, 1, 0), 0).rgb;
	
	if(density[0] < 0.0001) cubeIndex |= 1;
	if(density[0] < 0.0001) cubeIndex |= 2;
	if(density[0] < 0.0001) cubeIndex |= 4;
	if(density[0] < 0.0001) cubeIndex |= 8;
	if(density[0] < 0.0001) cubeIndex |= 16;
	if(density[0] < 0.0001) cubeIndex |= 32;
	if(density[0] < 0.0001) cubeIndex |= 64;
	if(density[0] < 0.0001) cubeIndex |= 128;
	
	z6_y6_x6_null6_cubeIndex8 = (gridPosition.z << 24) |
							   (gridPosition.y << 16) |
							   (gridPosition.x << 8 ) |
							   (cubeIndex			);
}