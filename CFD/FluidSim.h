#ifndef _FLUIDSIM_H_
#define _FLUIDSIM_H_

#include <SDL/SDL.h>
#undef main //main is defined in SDL, which will interfere with our main method if not undefined
#include <GL/glew.h>
#include <libpng\png.h>
#include <SDL/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>
#include "MarchingCubes.h"
#include "CFDSimulation.h"

//Number of VBOs we need. Other constants are in the source file.

class FluidSim
{
public:
	FluidSim();
	~FluidSim();

	//initializes and runs the main loop of the simulation. Returns once it is finished.
	void runSim();
private:

	//Initialize the simulation
	void init();
	
	//Initialize OpenGL
	void initGL();

	/*
	Compiles a shader
	@param srcPath the path to the shader file
	@param shaderType the type of shader
	@return the ID of the shader
	*/
	GLuint compileShader(const char* srcPath, GLenum shaderType);

	/*
	Creates a program and links it up with shaders. Also cleans up the shaders once linked.
	@param vertexShaderID id of vertex shader
	@param fragmentShaderID id of fragment shader
	@return the ID of the program
	*/
	GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID);

	//Load image using libpng
	std::pair<int, int> loadImage(const char* path, std::vector<png_byte>& imageData);

	//Main render loop
	void render();

	//Event handlers
	void handleKeyDownEvent(SDL_Keycode key);
	void handleMouseButtonDownEvent(Uint8 button);
	void handleMouseButtonUpEvent(Uint8 button);
	void handleMouseMotionEvent(Sint32 xrel, Sint32 yrel);

	//End and clean up the simulation and SDL+OpenGL
	void shutdown();

	//custom read function for pnglib. It's a C library and need a custom function to read from C++ structures
	static void readFunction(png_structp pngPtr, png_bytep data, png_size_t length);

	//The window we'll be rendering too
	SDL_Window* mWindow;

	//OpenGL context
	SDL_GLContext mContext;

	//SDL renderer for textures
	SDL_Renderer *renderer;

	//Shader Program
	std::array<GLuint, 2> mProgramID;

	//Vertex Array Object
	GLuint mVAO;

	//Vertex Buffer Objects
	std::array<GLuint, 3> mVBOs;

	//Textures
	GLuint mTexture, mTexture2;

	//matrices
	glm::mat4 mProjection, mView;
	//rotation quaternion
	glm::quat mQuat;
	//scaling vector
	glm::vec3 mScale;

	//vector of triangles that form the fluid
	std::vector<TRIANGLE> mTriangles;

	//Simulation
	CFDSimulation mSim;
};
#endif