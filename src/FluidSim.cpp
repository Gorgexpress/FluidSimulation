#include "FluidSim.h"
#include "SDLException.h"
#include "OpenGLException.h"
#include "Utility.h"
#include "LUTs.h"
#include <GL/GLU.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <ostream>
#include <iterator>
#include <thread>


//Constants for default window width and height
const int DEFAULT_WINDOW_WIDTH = 640;
const int DEFAULT_WINDOW_HEIGHT = 480;

//timestep
const float TIMESTEP = 0.0025f;

//constants for fluid simulation dimensions
const int SIM_WIDTH = 25;
const int SIM_HEIGHT = 25;
const int SIM_DEPTH = 25;

//Constant for file paths to shaders
const char* STATIC_VERTEX_SHADER_PATH = "shaders/room.glslv";
const char* STATIC_FRAGMENT_SHADER_PATH = "shaders/room.glslf";
const char* FLUID_VERTEX_SHADER_PATH = "shaders/fluid.glslv";
const char* FLUID_FRAGMENT_SHADER_PATH = "shaders/fluid.glslf";
const char* DENSITY_VERTEX_SHADER_PATH = "shaders/density.glslv";
const char* DENSITY_FRAGMENT_SHADER_PATH = "shaders/density.glslf";
const char* DENSITY_GEOMETRY_SHADER_PATH = "shaders/density.glslg";
const char* LIST_TRIANGLES_VERTEX_SHADER_PATH = "shaders/list_triangles.glslv";
const char* LIST_TRIANGLES_GEOMETRY_SHADER_PATH = "shaders/list_triangles.glslg";
const char* GEN_VERTICES_VERTEX_SHADER_PATH = "shaders/gen_vertices.glslv";
const char* GEN_VERTICES_GEOMETRY_SHADER_PATH = "shaders/gen_vertices.glslg";



//Path to texture
const char* TEXTURE_PATH = "floor_diamond_tileIII.png";
const char* TEXTURE_WALL_PATH = "stone wall 3.png";



const GLfloat STATIC_VERTICES[] = {
	//cube
	-1.0f, -1.0f, 1.0f, //0
	-1.0f, 1.0f, 1.0f, //1
	1.0f, 1.0f, 1.0f, //2
	1.0f, -1.0f, 1.0f,//3
	-1.0f, -1.0f, -1.0f, //4
	-1.0f, 1.0f, -1.0f,//5
	1.0f, 1.0f, -1.0f,//6
	1.0f, -1.0f, -1.0f,//7
	//quad
	-1.0f, -1.0f, 0.0f,//8
	-1.0f, 1.0f, 0.0f,//9
	1.0f, 1.0f, 0.0f,//10
	1.0f, -1.0f, 0.0f //11
};


const GLushort STATIC_INDICES[] = {
	//cube (room)
	0, 2, 1, 0, 3, 2,
	4, 3, 0, 4, 7, 3,
	4, 1, 5, 4, 0, 1,
	3, 6, 2, 3, 7, 6,
	1, 6, 5, 1, 2, 6,
	7, 5, 6, 7, 4, 5,
	//quad
	8, 9, 10, 8, 10, 11
};

//Position of camera in worldspace.
const glm::vec3 CAMERA_POS(10.0f, 15.0f, -30.0f);


//set size variables to defaults
FluidSim::FluidSim(){
	mWidth = DEFAULT_WINDOW_WIDTH;
	mHeight = DEFAULT_WINDOW_HEIGHT;
}

FluidSim::~FluidSim(){
	//delete context and window
	SDL_GL_DeleteContext(mContext);
	SDL_DestroyWindow(mWindow);
	mWindow = nullptr;

	//Quit SDL
	SDL_Quit();
}
void FluidSim::init(){
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::string errorMessage = "SDL could not initialize! SDL Error: ";
		errorMessage += SDL_GetError();
		throw SDLException(errorMessage);
	}
	//Use OpenGL 4.0 core
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	//Create Window
	mWindow = SDL_CreateWindow("A Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
		, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (mWindow == nullptr)
	{
		std::string errorMessage = "Window could not be created! SDL Error: ";
		errorMessage += SDL_GetError();
		throw SDLException(errorMessage);
	}
	//Create Context
	mContext = SDL_GL_CreateContext(mWindow);
	if (mContext == nullptr)
	{
		std::string errorMessage = "OpenGL Context could not be created! SDL Error: ";
		errorMessage += SDL_GetError();
		throw SDLException(errorMessage);
	}

	//Initialize GLEW
	glewExperimental = GL_TRUE; //needed to use modern OpenGL features
	GLenum glewError = glewInit();
	if (glewError != GLEW_OK)
	{
		std::string errorMessage(reinterpret_cast<char const*>(glewGetErrorString(glewError)));
		throw OpenGLException(errorMessage);
	}
	//Use VSync
	if (SDL_GL_SetSwapInterval(1) < 0)
	{
		std::cerr << "Could not use Vsync";
	}

	glGetError(); //discard single 1280 error that glew always causes on initialization

	
	//Initialize OpenGL
	initGL();

	
	//Disable polling of mouse motion events by default
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

	//Perform steps necessary to populate the VBO used to hold fluid vertices so we can render the fluid.
	genScalarField();
	mThreadSimRunning = true;
	mThreadSim = std::thread(&CFDSimulation::update, &mSim, TIMESTEP);
	GLuint nPrimitives = genTriangleList();
	nVertices = genVertices(nPrimitives);
	mAutoRun = false;
	
}

void FluidSim::initGL(){
	//Start by creating all of our shader programs

	//Room rendering program
	GLuint vertexShaderID = util::compileShader(STATIC_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	GLuint fragmentShaderID = util::compileShader(STATIC_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);
	mPrograms[Programs::ROOM] = util::linkProgram(vertexShaderID, fragmentShaderID, 0);

	//Fluid rendering program
	vertexShaderID = util::compileShader(FLUID_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	fragmentShaderID = util::compileShader(FLUID_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);
	mPrograms[Programs::FLUID] = util::linkProgram(vertexShaderID, fragmentShaderID, 0);

	//Scalar Field generation
	vertexShaderID = util::compileShader(DENSITY_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	fragmentShaderID = util::compileShader(DENSITY_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);
	GLuint geometryShaderID = util::compileShader(DENSITY_GEOMETRY_SHADER_PATH, GL_GEOMETRY_SHADER);
	mPrograms[Programs::SFIELD] = util::linkProgram(vertexShaderID, fragmentShaderID, geometryShaderID);

	//Generate list of triangles
	vertexShaderID = util::compileShader(LIST_TRIANGLES_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	geometryShaderID = util::compileShader(LIST_TRIANGLES_GEOMETRY_SHADER_PATH, GL_GEOMETRY_SHADER);
	//Create the program and link the shaders to it
	const GLchar* feedbackVaryings[] = { "z6_y6_x6_edge1_edge2_edge3" };
	mPrograms[Programs::LIST_TRIANGLES] = util::linkProgram(vertexShaderID, 0, geometryShaderID, feedbackVaryings, 1);

	//Generate vertices and normals for our fluid
	vertexShaderID = util::compileShader(GEN_VERTICES_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	geometryShaderID = util::compileShader(GEN_VERTICES_GEOMETRY_SHADER_PATH, GL_GEOMETRY_SHADER);
	const GLchar* feedbackVaryings2[] = { "vnVertice",
										  "vnNormal"};
	mPrograms[Programs::GEN_VERTICES] = util::linkProgram(vertexShaderID, 0, geometryShaderID, feedbackVaryings2, 2);

	//Iniitalize textures and GL objects
	initGLTextures();
	initGLObjects();

	//set all uniform locations
	getAllUniformLocations();

	//initialize our rotation quaternion
	mQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	//Initialize projection and view matrices, and the vec3 used for scaling.
	mProjection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 1000.0f);
	mView = glm::lookAt(
		CAMERA_POS,
		glm::vec3(10.0f, 5.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
		);
	mScale = glm::vec3(1.0f, 1.0f, 1.0f);

	//Enable depth test with GL_LESS depth func
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

}

void FluidSim::initGLObjects(){
	//Create a vertex array object
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);

	//Generate buffers
	glGenBuffers(BufferObjects::SIZE, &mBufferObjects[0]);

	//Initialize VBO for static vertices(room containing the fluid)
	glBindBuffer(GL_ARRAY_BUFFER, mBufferObjects[BufferObjects::ROOM_AB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(STATIC_VERTICES), STATIC_VERTICES, GL_STATIC_DRAW);

	//Initialize element array buffer for indices into the above array buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferObjects[BufferObjects::ROOM_EB]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(STATIC_INDICES), STATIC_INDICES, GL_STATIC_DRAW);

	//Initialize VBO for fluid vertices
	glBindBuffer(GL_ARRAY_BUFFER, mBufferObjects[BufferObjects::FLUID]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE) * 300000, nullptr, GL_DYNAMIC_COPY);
	//glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TRIANGLE) *mTriangles.size(), &mTriangles[0]);

	//Initialize array buffer to hold the list of triangles to generate
	glBindBuffer(GL_ARRAY_BUFFER, mBufferObjects[BufferObjects::TRIANGLE_LIST]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint) * (SIM_WIDTH) * (SIM_HEIGHT) * (SIM_DEPTH) * 5, nullptr, GL_DYNAMIC_COPY);

	//Initialize texture buffer to hold the marching cubes lookup table triTable.
	glBindBuffer(GL_TEXTURE_BUFFER, mBufferObjects[BufferObjects::TRI_TABLE]);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(LUTS::triTable), LUTS::triTable, GL_STATIC_READ);
	glBindTexture(GL_TEXTURE_BUFFER, mTextures[Textures::TRI_TABLE]);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, mBufferObjects[BufferObjects::TRI_TABLE]);

	//initialize framebuffer and give its output texture to it, the output texture being the 3d scalar field texture.
	glGenFramebuffers(1, &mFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTextures[Textures::SFIELD], 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw  OpenGLException("Error initializing framebuffer");
	//set the list of draw buffers
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	//unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Initialize Query Object
	glGenQueries(1, &mQuery);
}

void FluidSim::initGLTextures(){

	//Generate textures
	glGenTextures(Textures::SIZE, &mTextures[0]);

	//Load floor image
	std::vector<png_byte> imageData;
	util::dimensions2D dimensions = util::loadImage(TEXTURE_PATH, imageData);

	//Bind the cubemap texture and give it the floor image
	glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[Textures::CUBE_MAP]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, dimensions.width, dimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);

	//load texture for walls and ceiling, then send it to the rest of the cube map
	dimensions = util::loadImage(TEXTURE_WALL_PATH, imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, dimensions.width, dimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, dimensions.width, dimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, dimensions.width, dimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, dimensions.width, dimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, dimensions.width, dimensions.height, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);



	//Initialize texture to store particle positions
	glBindTexture(GL_TEXTURE_2D, mTextures[Textures::PARTICLES]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, 10000, 1, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Initialize texture to store scalar field
	glBindTexture(GL_TEXTURE_3D, mTextures[Textures::SFIELD]);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, SIM_WIDTH + 1, SIM_HEIGHT + 1, SIM_DEPTH + 1, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

}

void FluidSim::getAllUniformLocations(){
	mUniforms[Uniforms::ROOM_MVP] = glGetUniformLocation(mPrograms[Programs::ROOM], "MVP");
	mUniforms[Uniforms::FLUID_MVP] = glGetUniformLocation(mPrograms[Programs::FLUID], "MVP");
	mUniforms[Uniforms::FLUID_MODEL] = glGetUniformLocation(mPrograms[Programs::FLUID], "M");
	mUniforms[Uniforms::ROOM_CUBE_MAP] = glGetUniformLocation(mPrograms[Programs::ROOM], "cubeMap");
	mUniforms[Uniforms::FLUID_CUBE_MAP] = glGetUniformLocation(mPrograms[Programs::FLUID], "cubeMap");
	mUniforms[Uniforms::SFIELD_PARTICLES] = glGetUniformLocation(mPrograms[Programs::SFIELD], "particles");
	mUniforms[Uniforms::SFIELD_NUM_PARTICLES] = glGetUniformLocation(mPrograms[Programs::SFIELD], "nParticles");
	mUniforms[Uniforms::SFIELD_RADIUS_SQUARED] = glGetUniformLocation(mPrograms[Programs::SFIELD], "radiusSquared");
	mUniforms[Uniforms::LIST_TRIANGLES_SFIELD] = glGetUniformLocation(mPrograms[Programs::LIST_TRIANGLES], "sField");
	mUniforms[Uniforms::LIST_TRIANGLES_DIMENSIONS] = glGetUniformLocation(mPrograms[Programs::LIST_TRIANGLES], "dimensions");
	mUniforms[Uniforms::LIST_TRIANGLES_TRI_TABLE] = glGetUniformLocation(mPrograms[Programs::LIST_TRIANGLES], "triTable");
	mUniforms[Uniforms::GEN_VERTICES_DIMENSIONS] = glGetUniformLocation(mPrograms[Programs::GEN_VERTICES], "dimensions");
	mUniforms[Uniforms::GEN_VERTICES_SFIELD] = glGetUniformLocation(mPrograms[Programs::GEN_VERTICES], "sField");
}

void FluidSim::render(){

	//Generate MVP matrix for the room.
	glm::mat4 translation = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 rotation = glm::mat4(1.0f);
	glm::mat4 scaling = glm::scale(glm::mat4(), glm::vec3(50.0f, 50.0f, 50.0f)); 
	glm::mat4 model = translation * rotation * scaling;
	glm::mat4 MVP = mProjection * mView * model;


	//use our room shader program
	glUseProgram(mPrograms[Programs::ROOM]);
	//Clear Buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Send the shader our MVP matrix
	glUniformMatrix4fv(mUniforms[Uniforms::ROOM_MVP], 1, GL_FALSE, &MVP[0][0]);

	//Bind the cube map texture then send it to the shader
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures[Textures::CUBE_MAP]);
	glUniform1i(mUniforms[Uniforms::ROOM_CUBE_MAP], 0);

	//Enable vertex attrib array 0, vertices, and bind the static vbo.
	glEnableVertexAttribArray(0); 
	glBindBuffer(GL_ARRAY_BUFFER, mBufferObjects[BufferObjects::ROOM_AB]);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // vertices

	//Bind IBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferObjects[BufferObjects::ROOM_EB]); 

	//draw the room using all but the last 6 indices
	glDrawElements(GL_TRIANGLES, (sizeof(STATIC_INDICES) / sizeof(GLushort)) - 6, GL_UNSIGNED_SHORT, nullptr);

	//calculate MVP matrix for fluid
	translation = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
	rotation = glm::mat4_cast(mQuat);
	scaling = glm::scale(glm::mat4(), mScale);
	model = translation * rotation * scaling;
	MVP = mProjection * mView * model;
	
	//Use our fluid shader program
	glUseProgram(mPrograms[Programs::FLUID]);
	glEnableVertexAttribArray(1); //Attribute 1 is normals, attribute 0 is still vertices.
	//Send the uniforms to shader, the cubemap texture, MVP matrix, model matrix,and camera position.
	glUniform1i(mUniforms[Uniforms::FLUID_CUBE_MAP], 0);
	glUniformMatrix4fv(mUniforms[Uniforms::FLUID_MVP], 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(mUniforms[Uniforms::FLUID_MODEL], 1, GL_FALSE, &model[0][0]);
	GLint location = glGetUniformLocation(mPrograms[Programs::FLUID], "cameraPos");
	glUniform3f(location, 10.0f, 15.0f, -30.0f);

	//bind VBO for fluid vertices
	glBindBuffer(GL_ARRAY_BUFFER, mBufferObjects[BufferObjects::FLUID]); 
	//Set attrib pointers for vertices and normals
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6, nullptr);  //vertices
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (GLvoid*)(sizeof(GLfloat) * 3)); //normals

	//Render fluid
	glDrawArrays(GL_TRIANGLES, 0, nVertices * 2);

	//print any errors to console
	GLenum err = glGetError();
	while (err != GL_NO_ERROR)
	{
		std::cerr << err << std::endl;
		err = glGetError();
	}

}

void FluidSim::genScalarField(){
	//Render to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glViewport(0, 0, SIM_WIDTH + 1, SIM_HEIGHT + 1);

	//Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT);

	//use the scalar field shader program
	glUseProgram(mPrograms[Programs::SFIELD]);

	//set uniforms
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTextures[Textures::PARTICLES]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mSim.markerParticles().size(), 1, GL_RGB, GL_FLOAT, &mSim.markerParticles()[0]);
	glUniform1i(mUniforms[Uniforms::SFIELD_PARTICLES], 0);
	glUniform1i(mUniforms[Uniforms::SFIELD_NUM_PARTICLES], mSim.markerParticles().size());
	glUniform1f(mUniforms[Uniforms::SFIELD_RADIUS_SQUARED], 1.0f);

	//draw
	glDrawArrays(GL_POINTS, 0, SIM_DEPTH + 1);

	//unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, mWidth, mHeight);

}

GLuint FluidSim::genTriangleList(){
	//disable rasterizer
	glEnable(GL_RASTERIZER_DISCARD);

	//use the list triangles program
	glUseProgram(mPrograms[Programs::LIST_TRIANGLES]);
	
	//set uniforms
	glUniform3i(mUniforms[Uniforms::LIST_TRIANGLES_DIMENSIONS], SIM_WIDTH, SIM_HEIGHT, SIM_DEPTH); //dimensions
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mTextures[Textures::SFIELD]);
	glUniform1i(mUniforms[Uniforms::LIST_TRIANGLES_SFIELD], 0);  //3d texture containing the scalar field
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_BUFFER, mTextures[Textures::TRI_TABLE]);
	glUniform1i(mUniforms[Uniforms::LIST_TRIANGLES_TRI_TABLE], 1);

	//bind transform feedback buffer object. Only one tbo so we just use index 0. We are writing
	//the list of triangles to the triangle list array buffer object
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mBufferObjects[BufferObjects::TRIANGLE_LIST]);

	//Begin query for # of primitives written and begin transform feedback
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, mQuery);
	glBeginTransformFeedback(GL_POINTS);

	//draw 
	glDrawArrays(GL_POINTS, 0, (SIM_WIDTH) * (SIM_HEIGHT) * (SIM_DEPTH));

	//end transform feedback and the query
	glEndTransformFeedback();
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	//Get the number of primitives
	GLuint nPrimitives;
	glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &nPrimitives);

	//Flush and renable rasterization before returning the number of primitives drawn
	glFlush();
	glDisable(GL_RASTERIZER_DISCARD);

	return nPrimitives;

}

GLuint FluidSim::genVertices(GLuint in_nPrimitives){
	//disable rasterizer
	glEnable(GL_RASTERIZER_DISCARD);

	//Use the fluid vertice generation program
	glUseProgram(mPrograms[Programs::GEN_VERTICES]);

	//set uniforms
	glUniform3i(mUniforms[Uniforms::GEN_VERTICES_DIMENSIONS], SIM_WIDTH + 1, SIM_HEIGHT + 1, SIM_DEPTH + 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mTextures[Textures::SFIELD]);
	glUniform1i(mUniforms[Uniforms::GEN_VERTICES_SFIELD], 0);  //3d texture containing the scalar field

	//bind transform feedback buffer object. Only one tbo so we just use index 0. 
	//We are writing the fluid vertices and normals to the fluid array buffer object.
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mBufferObjects[BufferObjects::FLUID]);

	//enable attribs
	glEnableVertexAttribArray(0);

	//Bind our triangle list array buffer and set attrib 0 to point to it. 
	glBindBuffer(GL_ARRAY_BUFFER, mBufferObjects[BufferObjects::TRIANGLE_LIST]);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT,  0, 0);

	//Begin transform feedback
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, mQuery);
	glBeginTransformFeedback(GL_POINTS);

	//draw
	glDrawArrays(GL_POINTS, 0, in_nPrimitives);

	//End transform feedback
	glEndTransformFeedback();
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	//Get number of output primitives
	GLuint out_nPrimitives;
	glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &out_nPrimitives);

	//Flush and renable rasterization before returning the number of primitives drawn
	glFlush();
	glDisable(GL_RASTERIZER_DISCARD);

	return out_nPrimitives;

}
void FluidSim::handleKeyDownEvent(SDL_Keycode key){
	switch (key)
	{
	case SDLK_x: //increase scale of fluid
		if (mScale.x < 3.0f)
		{
			mScale.x += 0.1f;
			mScale.y += 0.1f;
			mScale.z += 0.1f;
		}
		break;
	case SDLK_z: //decrease scale of fluid
		if (mScale.x > 0.2f)
		{
			mScale.x -= 0.1f;
			mScale.y -= 0.1f;
			mScale.z -= 0.1f;
		}
		break;
	case SDLK_n:   //update simulation
		if(!mAutoRun && !mThreadSimRunning){
			mThreadSimRunning = true;
			mThreadSim = std::thread(&CFDSimulation::update, &mSim, TIMESTEP);
			GLuint nPrimitives = genTriangleList();
			nVertices = genVertices(nPrimitives);
		}
		break;
	case SDLK_m:
		mAutoRun = !mAutoRun;
		if (!mAutoRun) std::cout << counter << std::endl;
	
	case SDLK_q:
		glBindTexture(GL_TEXTURE_2D, mTextures[Textures::PARTICLES]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mSim.markerParticles().size(), 1, GL_RGB, GL_FLOAT, &mSim.markerParticles()[0]);

		genScalarField();
		SDL_GL_SwapWindow(mWindow);
		break;
	}

}
void FluidSim::handleMouseButtonDownEvent(Uint8 button){
	if (button == SDL_BUTTON_LEFT)
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	}
}

void FluidSim::handleMouseButtonUpEvent(Uint8 button){
	if (button == SDL_BUTTON_LEFT)
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);
	}
}


void FluidSim::handleMouseMotionEvent(Sint32 xrel, Sint32 yrel){
	glm::quat rot(1.0f, yrel*0.01f, xrel*0.01f, 0.0f);
	mQuat = rot * mQuat;
	mQuat = glm::normalize(mQuat);
}


void FluidSim::runSim(){
	//Initialize the program in a try/catch block. If an exception occurs, print the message to std::cerr and throw it again.
	try
	{
		init();
	}
	catch (OpenGLException e)
	{
		std::cerr << e.what() << std::endl;
		throw e;
	}
	catch (SDLException e)
	{
		std::cerr << e.what() << std::endl;
		throw e;
	}

	//Main loop flag
	bool quit = false;

	//Event handler
	SDL_Event e;

	//While application is running
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				handleKeyDownEvent(e.key.keysym.sym);
			case SDL_MOUSEBUTTONDOWN:
				handleMouseButtonDownEvent(e.button.button);
				break;
			case SDL_MOUSEBUTTONUP:
				handleMouseButtonUpEvent(e.button.button);
				break;
			case SDL_MOUSEMOTION:
				handleMouseMotionEvent(e.motion.xrel, e.motion.yrel);
				break;
			default:
				;
			}
		}
		//Render
		render();
		//Swap buffers
		SDL_GL_SwapWindow(mWindow);
		if (mAutoRun && !mThreadSimRunning) {
			mThreadSimRunning = true;
			mThreadSim = std::thread(&CFDSimulation::update, &mSim, TIMESTEP);
			GLuint nPrimitives = genTriangleList();
			nVertices = genVertices(nPrimitives);
		}
		//Check if our simulation has finished updating. Join the thread if it is. 
		if (mThreadSim.joinable()){

			mThreadSim.join();
			counter++;
			mThreadSimRunning = false;
			genScalarField();

		}
	}
}



