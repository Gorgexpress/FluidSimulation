#version 400 core

flat in int layer;

uniform sampler2D particles; //texture containing particle coordinates
uniform int nParticles; //number of particles
uniform float radiusSquared; //radius of particles squared

out float density;

void main(){	
	ivec3 gridPosition = ivec3(gl_FragCoord.x - 0.5f, gl_FragCoord.y - 0.5f, layer);
	density = 0.0f;
	for(int i = 0; i < nParticles; ++i)
	{
		//check if grid position is within the particle radius
		//we compare the square of both values, as it is more efficient than 
		//computing the square root of the distance squared every time.
		vec3 particlePosition = texelFetch(particles, ivec2(i, 0), 0).rgb;
		vec3 delta = gridPosition - particlePosition;
		float distanceSquared = dot(delta, delta);
		if(distanceSquared <= radiusSquared)
		{
			density += pow(1.0f - distanceSquared, 2);
		}
	}
}