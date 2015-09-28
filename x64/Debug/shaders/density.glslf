#version 400 core

flat in int layer;

uniform sampler2D particles;
uniform int nParticles;
uniform ivec2 dimensions;
uniform float radiusSquared;

out float density;

void main(){	
	ivec3 gridPosition = ivec3(gl_FragCoord.x - 0.5f, gl_FragCoord.y - 0.5f, layer);
	density = 0.0f;
	for(int i = 0; i < nParticles; ++i)
	{
		vec3 particlePosition = texelFetch(particles, ivec2(i, 0), 0).rgb;
		vec3 delta = gridPosition - particlePosition;
		float distance = dot(delta, delta);
		if(distance <= radiusSquared)
		{
			density += pow(1.0f - distance, 2);
			//density += 1.0f / distance;
		}
	}
}