#version 400 core


struct vnpair{
	vec3 vertex;
	vec3 normal;
};

//upper 6 bits are the z value for the position, followed by 6 bits for the y value, etc...
//edge equals the marching cubes edge case. Can be between 0 and 11.
layout(location = 0) in uint z6_y6_x6_edge1_edge2_edge3;

uniform sampler3D sField; //scalar field
uniform ivec3 dimensions; //dimensions of fluid

out vertexData
{
	vnpair vertices[3];
}vdata;

//Gradient calculation that does not check bounds. 
/*
vec3 Gradient(ivec3 p)
{
	return vec3(texelFetch(sField, p + ivec3(1, 0, 0), 0).r - texelFetch(sField, p + ivec3(-1, 0, 0), 0).r,
				texelFetch(sField, p + ivec3(0, 1, 0), 0).r - texelFetch(sField, p + ivec3(0, -1, 0), 0).r,
				texelFetch(sField, p + ivec3(0, 0, 1), 0).r - texelFetch(sField, p + ivec3(0, 0, -1), 0).r)
				* 0.5;
}*/

//Gradient calculation with bounds checking. Out of bounds values have a density of 0.
vec3 Gradient(ivec3 p)
{
	vec3 grad;
	float positive, negative; //positive direction and negative direction
	if(p.x != dimensions.x)
		positive = texelFetch(sField, p + ivec3(1, 0, 0), 0).r;
	else
		positive = 0.0f;
	if(p.x != 0)
		negative = texelFetch(sField, p + ivec3(-1, 0, 0), 0).r;
	else
		negative = 0.0f;
	grad.x = positive - negative;
	
	if(p.y != dimensions.y)
		positive = texelFetch(sField, p + ivec3(0, 1, 0), 0).r;
	else
		positive = 0.0f;
	if(p.y != 0)
		negative = texelFetch(sField, p + ivec3(0, -1, 0), 0).r;
	else
		negative = 0.0f;
	grad.y = positive - negative;
	
	if(p.z != dimensions.z)
		positive = texelFetch(sField, p + ivec3(0, 0, 1), 0).r;
	else
		positive = 0.0f;
	if(p.z != 0)
		negative = texelFetch(sField, p + ivec3(0, 0, -1), 0).r;
	else
		negative = 0.0f;
	grad.z = positive - negative;
	//multiply grad by 0.5 divided by size of an individual cell.
	return grad * 0.5f; 
}

//Interpolate between 2 points for marching cubes
//Taken from Paul Bourke's example, but also computes normals at the same time as the fluid vertex.
vnpair VertexInterp(ivec3 p1, ivec3 p2){
	float valp1 = texelFetch(sField, p1, 0);
	float valp2 = texelFetch(sField, p2, 0);
	vec3 gradp1 = Gradient(p1);
	vec3 gradp2 = Gradient(p2);
	vnpair rValue;
	if (abs(0.0001 - valp1) < 0.00001)
	{
		rValue.vertex = vec3(float(p1.x), float(p1.y), float(p1.z));
		rValue.normal = gradp1;
		return rValue;
	}
	if (abs(0.0001 - valp2) < 0.00001)
	{
		rValue.vertex = vec3(float(p2.x), float(p2.y), float(p2.z));
		rValue.normal = gradp2;
		return rValue;
	}
	if (abs(valp1 - valp2) < 0.00001)
	{
		rValue.vertex = vec3(float(p1.x), float(p1.y), float(p1.z));
		rValue.normal = gradp1;
		return rValue;
	}
	
	float mu = (0.0001 - valp1) / (valp2 - valp1);
	rValue.vertex = vec3(float(p1.x) + mu * (float(p2.x) - float(p1.x)),
						 float(p1.y) + mu * (float(p2.y) - float(p1.y)),
						 float(p1.z) + mu * (float(p2.z) - float(p1.z))
						);
	
	rValue.normal = vec3(gradp1.x + mu * (gradp2.x - gradp1.x),
						 gradp1.y + mu * (gradp2.y - gradp1.y),
						 gradp1.z + mu * (gradp2.z - gradp1.z));
	return rValue;
}


void main(){
	ivec3 gridPosition = ivec3((z6_y6_x6_edge1_edge2_edge3 >> 14) & 0x3F,
							   (z6_y6_x6_edge1_edge2_edge3 >> 20) & 0x3F,
							   (z6_y6_x6_edge1_edge2_edge3 >> 26) & 0x3F);
	uint edgeCase = (z6_y6_x6_edge1_edge2_edge3 >> 8) & 0xF;
	
	//Find the final fluid vertex and normal values using interpolations, according to
	//the edge case.
	//The code below repeats 3 times. Once for each vertex normal pair to output. 
    //Keeping it in  a loop caused problems, so it was manually unrolled.
	switch (edgeCase)
	{
		case 0:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(1, 0, 1));
			break;
		case 1:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 0, 0));
			break;
		case 2:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition 					);
			break;
		case 3:
			vdata.vertices[0] = VertexInterp(gridPosition 			    , gridPosition + ivec3(0, 0, 1));
			break;
		case 4:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(0, 1, 1), gridPosition + ivec3(1, 1, 1));
			break;
		case 5:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(1, 1, 1), gridPosition + ivec3(1, 1, 0));
			break;
		case 6:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(1, 1, 0), gridPosition + ivec3(0, 1, 0));
			break;
		case 7:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(0, 1, 0), gridPosition + ivec3(0, 1, 1));
			break;
		case 8:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(0, 1, 1));
			break;
		case 9:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 1, 1));
			break;
		case 10:
			vdata.vertices[0] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition + ivec3(1, 1, 0));
			break;
		case 11:
			vdata.vertices[0] = VertexInterp(gridPosition  				, gridPosition + ivec3(0, 1, 0));
			break;
		default:
			break;
	}
	
	edgeCase = (z6_y6_x6_edge1_edge2_edge3 >> 4) & 0xF;
	switch (edgeCase)
	{
		case 0:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(1, 0, 1));
			break;
		case 1:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 0, 0));
			break;
		case 2:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition 					);
			break;
		case 3:
			vdata.vertices[1] = VertexInterp(gridPosition 			    , gridPosition + ivec3(0, 0, 1));
			break;
		case 4:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(0, 1, 1), gridPosition + ivec3(1, 1, 1));
			break;
		case 5:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(1, 1, 1), gridPosition + ivec3(1, 1, 0));
			break;
		case 6:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(1, 1, 0), gridPosition + ivec3(0, 1, 0));
			break;
		case 7:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(0, 1, 0), gridPosition + ivec3(0, 1, 1));
			break;
		case 8:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(0, 1, 1));
			break;
		case 9:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 1, 1));
			break;
		case 10:
			vdata.vertices[1] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition + ivec3(1, 1, 0));
			break;
		case 11:
			vdata.vertices[1] = VertexInterp(gridPosition  				, gridPosition + ivec3(0, 1, 0));
			break;
		default:
			break;
	}
	
	edgeCase = (z6_y6_x6_edge1_edge2_edge3) & 0xF;
	switch (edgeCase)
	{
		case 0:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(1, 0, 1));
			break;
		case 1:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 0, 0));
			break;
		case 2:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition 					);
			break;
		case 3:
			vdata.vertices[2] = VertexInterp(gridPosition 			    , gridPosition + ivec3(0, 0, 1));
			break;
		case 4:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(0, 1, 1), gridPosition + ivec3(1, 1, 1));
			break;
		case 5:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(1, 1, 1), gridPosition + ivec3(1, 1, 0));
			break;
		case 6:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(1, 1, 0), gridPosition + ivec3(0, 1, 0));
			break;
		case 7:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(0, 1, 0), gridPosition + ivec3(0, 1, 1));
			break;
		case 8:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(0, 0, 1), gridPosition + ivec3(0, 1, 1));
			break;
		case 9:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(1, 0, 1), gridPosition + ivec3(1, 1, 1));
			break;
		case 10:
			vdata.vertices[2] = VertexInterp(gridPosition + ivec3(1, 0, 0), gridPosition + ivec3(1, 1, 0));
			break;
		case 11:
			vdata.vertices[2] = VertexInterp(gridPosition  				, gridPosition + ivec3(0, 1, 0));
			break;
		default:
			break;
	}
}
