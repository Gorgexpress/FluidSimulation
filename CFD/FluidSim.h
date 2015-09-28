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
#include <thread>
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

	enum BufferObjects{
		FLUID,
		ROOM_AB,
		ROOM_EB,
		FRAMEBUFFER,
		TRIANGLE_LIST,
		TRI_TABLE,
		SIZE
	};

	enum Programs{
		FLUID,
		ROOM,
		SFIELD,
		LIST_TRIANGLES,
		GEN_VERTICES,
		SIZE
	};

	enum Uniforms{
		FLUID_MVP,
		FLUID_MODEL,
		FLUID_VIEW,
		ROOM_MVP,
		ROOM_CUBE_MAP,
		FLUID_CUBE_MAP,
		SFIELD_PARTICLES,
		SFIELD_NUMPARTICLES,
		SFIELD_DIMENSIONS,
		SFIELD_RADIUS_SQUARED,
		LIST_TRIANGLES_DIMENSIONS,
		LIST_TRIANGLES_SFIELD,
		GEN_VERTICES_DIMENSIONS,
		GEN_VERTICES_SFIELD,
		SIZE
	};
	//Initialize the simulation
	void init();
	
	//Initialize OpenGL
	void initGL();

	//Initialize all OpenGL objects(VAO, VBOs, FBO, etc..)
	void initGLObjects();

	void initGLTextures();

	void getAllUniformLocations();
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
	GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID, GLuint geometryShaderID, const GLchar* varyings[], int varyingsCount);
	GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID, GLuint geometryShaderID);

	/*
	Load image using libpng
	@param path the path to the png file
	@param imageData the vector of png_bytes to store the image data. Any data already existing in the
	vector will be cleared out.
	@return a pair of ints containing the width and height of the image respectively.
	If there was an error, first will be set to -1. 
	*/
	std::pair<int, int> loadImage(const char* path, std::vector<png_byte>& imageData);

	//Main render loop
	void render();

	void genScalarField();
	GLuint genTriangleList();
	GLuint genVertices(GLuint nPrimitives);

	//Event handlers
	void handleKeyDownEvent(SDL_Keycode key);
	void handleMouseButtonDownEvent(Uint8 button);
	void handleMouseButtonUpEvent(Uint8 button);
	void handleMouseMotionEvent(Sint32 xrel, Sint32 yrel);

	//End and clean up the simulation and SDL+OpenGL
	void shutdown();

	//The window we'll be rendering too
	SDL_Window* mWindow;

	//OpenGL context
	SDL_GLContext mContext;

	//SDL renderer for textures
	SDL_Renderer *renderer;

	//Shader Programs
	GLuint mProgramFluid, mProgramStatic, mProgramDensity, mProgramTest, mProgramListTriangles, mProgramGenVertices;

	//Vertex Array Object
	GLuint mVAO;

	//Vertex Buffer Objects
	GLuint mVBOs; //static draw VBO
	GLuint mIBOs; //static draw indices
	GLuint mVBOf; //dynamic VBO for fluid vertices
	GLuint mFBO; //framebuffer
	GLuint mTBO; //transform feedback
	GLuint mEdgeTableBO;
	GLuint mTriTableBO;

	//Textures
	GLuint mCubeMap;
	GLuint mEdgeTable;
	GLuint mTriTable;
	GLuint mParticleTexture;
	GLuint mScalarFieldTexture;

	//Uniform locations
	GLint mUniformLocMVPf, mUniformLocMVPs; //MVP matrix for fluid and static shaders
	GLint mUniformLocM;  // Model Matrix
	GLint mUniformLocV; // View Matrix
	GLint mUniformLocCubeMaps;
	GLint mUniformLocCubeMapf;
	GLint mUniformParticles;
	GLint mUniformDimensions;
	GLint mUniformnParticles;
	GLint mUniformRadiusSquared;
	GLint mUniformSField;
	GLint mUniformDimensions3D;

	//matrices
	glm::mat4 mProjection, mView;
	//rotation quaternion
	glm::quat mQuat;
	//scaling vector
	glm::vec3 mScale;

	//query object
	GLuint mQuery;

	//number of triangles in fluid vbo
	GLuint nTriangles;
	//std::vector of triangles that form the fluid. Vertices and normals interleaved. 
	std::vector<TRIANGLE> mTriangles;

	//Simulation
	CFDSimulation mSim;

	//Width and Height of window
	int mWidth, mHeight;

	std::thread mThreadSim;
};
#endif