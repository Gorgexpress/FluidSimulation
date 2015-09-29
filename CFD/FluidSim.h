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

	//The following enums act as array indices for the std::arrays containing the buffer objects, programs, uniforms,
	//and textures respectively
	struct BufferObjects{
		enum BufferObjectsEnum{
			FLUID,
			ROOM_AB,
			ROOM_EB,
			TRIANGLE_LIST,
			TRI_TABLE,
			SIZE
		};
	};

	struct Programs{
		enum ProgramsEnum{
			FLUID,
			ROOM,
			SFIELD,
			LIST_TRIANGLES,
			GEN_VERTICES,
			SIZE
		};
	};

	struct Uniforms{
		enum UniformsEnum{
			FLUID_MVP,
			FLUID_MODEL,
			FLUID_VIEW,
			ROOM_MVP,
			ROOM_CUBE_MAP,
			FLUID_CUBE_MAP,
			SFIELD_PARTICLES,
			SFIELD_NUM_PARTICLES,
			SFIELD_RADIUS_SQUARED,
			LIST_TRIANGLES_DIMENSIONS,
			LIST_TRIANGLES_SFIELD,
			GEN_VERTICES_DIMENSIONS,
			GEN_VERTICES_SFIELD,
			SIZE
		};
	};

	struct Textures{
		enum TexturesEnum{
			CUBE_MAP,
			TRI_TABLE,
			PARTICLES,
			SFIELD,
			SIZE
		};
	};
	//Initialize the simulation
	void init();
	
	//Initialize OpenGL
	void initGL();

	//Initialize all OpenGL objects(VAO, VBOs, FBO, etc..)
	void initGLObjects();

	//Initialize the textures
	void initGLTextures();

	//Sets the uniform locations for the uniforms array
	void getAllUniformLocations();




	//Main render loop
	void render();

	void genScalarField();
	GLuint genTriangleList();
	GLuint genVertices(GLuint in_nPrimitives);

	//Event handlers
	void handleKeyDownEvent(SDL_Keycode key);
	void handleMouseButtonDownEvent(Uint8 button);
	void handleMouseButtonUpEvent(Uint8 button);
	void handleMouseMotionEvent(Sint32 xrel, Sint32 yrel);


	//The window we'll be rendering too
	SDL_Window* mWindow;

	//OpenGL context
	SDL_GLContext mContext;

	//SDL renderer for textures
	SDL_Renderer *renderer;

	//Shader Programs
	//GLuint mProgramFluid, mProgramStatic, mProgramDensity, mProgramTest, mProgramListTriangles, mProgramGenVertices;
	std::array<GLuint, Programs::SIZE> mPrograms;
	//Vertex Array Object
	GLuint mVAO;

	/*
	//Vertex Buffer Objects
	GLuint mVBOs; //static draw VBO
	GLuint mIBOs; //static draw indices
	GLuint mVBOf; //dynamic VBO for fluid vertices
	GLuint mFBO; //framebuffer
	GLuint mTBO; //transform feedback
	GLuint mEdgeTableBO;
	GLuint mTriTableBO;
	*/
	std::array<GLuint, BufferObjects::SIZE> mBufferObjects;
	/*
	GLuint mCubeMap;
	GLuint mEdgeTable;
	GLuint mTriTable;
	GLuint mParticleTexture;
	GLuint mScalarFieldTexture;
	*/
	std::array<GLuint, Textures::SIZE> mTextures;
	/*
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
	*/
	std::array<GLint, Uniforms::SIZE> mUniforms;

	GLuint mFrameBuffer;

	//matrices
	glm::mat4 mProjection, mView;
	//rotation quaternion
	glm::quat mQuat;
	//scaling vector
	glm::vec3 mScale;

	//query object
	GLuint mQuery;

	//number of triangles in fluid vbo
	GLuint nVertices;

	//Simulation
	CFDSimulation mSim;

	//Width and Height of window
	int mWidth, mHeight;

	std::thread mThreadSim;
};
#endif