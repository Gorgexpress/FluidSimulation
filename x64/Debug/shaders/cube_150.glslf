#version 330 core

in vec3 normal;
in vec3 position;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform samplerCube cubeMap;
uniform vec3 cameraPos;

void main(){
	vec3 water = vec3(0.0196, 0.149, 0.48);
	vec3 I = normalize(position - cameraPos);
	vec3 R = refract(I, normalize(normal), 0.75f);
	vec3 Refl = reflect(I, normalize(normal));
	float fresnel = 0.018 + (1.0 - 0.018) * pow(1.0 + dot(I, normalize(Refl - I)),5.0);
	vec3 reflectedColor = texture(cubeMap, Refl).rgb;
	vec3 refractedColor = texture(cubeMap, R).rgb;
	
	color = (reflectedColor * fresnel + (refractedColor + (1.0f - fresnel) * water) * (1.0f - fresnel));
}