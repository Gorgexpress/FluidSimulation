#version 400 core

in vec3 normal;
in vec3 position;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
uniform samplerCube cubeMap; //cubemap texture for the room surrounding the fluid
uniform vec3 cameraPos;  //absolute camera position

void main(){
    //Color of the water. Can be set to 0 to make color only based off reflect, reflection etc...
	vec3 water = vec3(0.0196, 0.149, 0.48); 
	
	//Calculate Incident vector
	vec3 I = normalize(position - cameraPos);
	
	//Calculate refraction and reflection direction
	vec3 Refr = refract(I, normalize(normal), 0.75f);
	vec3 Refl = reflect(I, normalize(normal));
	
	//Calculate fresnel 
	float fresnel = 0.018 + (1.0 - 0.018) * pow(1.0 + dot(I, normalize(Refl - I)),5.0);
	
	//Get reflected and refracted colors from the surrounding cubemap
	vec3 reflectedColor = texture(cubeMap, Refl).rgb;
	vec3 refractedColor = texture(cubeMap, Refr).rgb;
	
	//calculate final color
	color = (reflectedColor * fresnel + (refractedColor + (1.0f - fresnel) * water) * (1.0f - fresnel));
}