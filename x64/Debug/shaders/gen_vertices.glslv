#version 400 core

struct vnpair{
	vec3 vertex;
	vec3 normal;
}

// Input vertex data, different for all executions of this shader.
layout(location = 0) in uint z6_y6_x6_edge1_edge2_edge3;

uniform sampler3D sField;
uniform ivec3 dimensions;

out vnpair vertices[3];

vec3 Gradient(ivec3 p)
{
	return vec3(texelFetch(sField, p + ivec3(1, 0, 0), 0) - texelFetch(sField, p + ivec3(-1, 0, 0), 0),
				texelFetch(sField, p + ivec3(0, 1, 0), 0) - texelFetch(sField, p + ivec3(0, -1, 0), 0),
				texelFetch(sField, p + ivec3(0, 0, 1), 0) - texelFetch(sField, p + ivec3(0, 0, -1), 0))
				* 0.5;
}

vnpair VertexInterp(ivec3 p1, ivec3 p2){
	float valp1 = texelFetch(sField, p1, 0);
	float valp2 = texelFetch(sField, p2, 0);
	if (abs(0.0001 - valp1) < 0.00001)
		return vec3(p1.x, p1.y, p1.z);
	if (abs(0.0001 - valp2) < 0.00001)
		return vec3(p2.x, p2.y, p2.z);
	if (abs(valp1 - valp2) < 0.00001)
		return vec3(p1.x, p1.y, p1.z);
	
	float mu = (0.0001 - valp1) / (valp2 - valp1);
	vnpair rValue;
	rValue.vertex = vec3(float(p1.x) + mu * (float(p2.x) - float(p1.x)),
				float(p1.y) + mu * (float(p2.y) - float(p1.y)),
				float(p1.z) + mu * (float(p2.z) - float(p1.z))
				);
	vec3 gradp1 = Gradient(p1);
	vec3 gradp2 = Gradient(p2);
	
	rValue.normal = vec3(gradp1.x + mu * (gradp2.x - gradp1.x),
						 gradp1.y + mu * (gradp2.y - gradp1.y),
						 gradp1.z + mu * (gradp2.z - gradp1.z));
	return rValue;
}


void main(){
	ivec3 gridPosition = ivec3((z6_y6_x6_edge1_edge2_edge3 >> 26) & 3F,
							   (z6_y6_x6_edge1_edge2_edge3 >> 20) & 3F,
							   (z6_y6_x6_edge1_edge2_edge3 >> 14) & 3F);
	uint edges[3];
	edges[0] = (z6_y6_x6_edge1_edge2_edge3 >> 8) & 0xF; 
	edges[1] = (z6_y6_x6_edge1_edge2_edge3 >> 4) & 0xF; 
	edges[2] = (z6_y6_x6_edge1_edge2_edge3     ) & 0xF; 
	for(int i = 0; i < 3; ++i)
	{
		switch (edge[i])
		{
			case 0:
				vertices[i] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(1, 0, 1));
				break;
			case 1:
				vertices[i] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 0, 0));
				break;
			case 2:
				vertices[i] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition 					);
				break;
			case 3:
				vertices[i] = VertexInterp(gridPosition 			    , gridPosition + ivec3(0, 0, 1));
				break;
			case 4:
				vertices[i] = VertexInterp(gridPosition + ivec3(0, 1, 1), gridPosition + ivec3(1, 1, 1));
				break;
			case 5:
				vertices[i] = VertexInterp(gridPosition + ivec3(1, 1, 1), gridPosition + ivec3(1, 1, 0));
				break;
			case 6:
				vertices[i] = VertexInterp(gridPosition + ivec3(1, 1, 0), gridPosition + ivec3(0, 1, 0));
				break;
			case 7:
				vertices[i] = VertexInterp(gridPosition + ivec3(0, 1, 0), gridPosition + ivec3(0, 1, 1));
				break;
			case 8:
				vertices[i] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(0, 1, 1));
				break;
			case 9:
				vertices[i] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 1, 1));
				break;
			case 10:
				vertices[i] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition + ivec3(1, 1, 0));
				break;
			case 11:
				vertices[i] = VertexInterp(gridPosition  				, gridPosition + ivec3(0, 1, 0));
				break;
			default:
				break;
		}
	}
}
