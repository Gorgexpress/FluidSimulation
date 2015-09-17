#include "FluidSim.h"
#include "SDLException.h"
#include "OpenGLException.h"
#include <GL/GLU.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include "LUTs.h"

//Constants for default window width and height
const int DEFAULT_WINDOW_WIDTH = 640;
const int DEFAULT_WINDOW_HEIGHT = 480;

//constants for fluid simulation dimensions
const int SIM_WIDTH = 25;
const int SIM_HEIGHT = 25;
const int SIM_DEPTH = 25;

//Constant for file paths to shaders
const char* STATIC_VERTEX_SHADER_PATH = "shaders/static.glslv";
const char* STATIC_FRAGMENT_SHADER_PATH = "shaders/static.glslf";
const char* FLUID_VERTEX_SHADER_PATH = "shaders/cube_150.glslv";
const char* FLUID_FRAGMENT_SHADER_PATH = "shaders/cube_150.glslf";
const char* DENSITY_VERTEX_SHADER_PATH = "shaders/density.glslv";
const char* DENSITY_FRAGMENT_SHADER_PATH = "shaders/density.glslf";
const char* DENSITY_GEOMETRY_SHADER_PATH = "shaders/density.glslg";
const char* TEST_VERTEX_SHADER_PATH = "shaders/test.glslv";
const char* TEST_FRAGMENT_SHADER_PATH = "shaders/test.glslf";



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
	//cube
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
	//we already call a shutdown method before the sim ends so we dont need anything here right now
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

	
	//Initialize our sim, we arent gonna update it yet so just get whatever info we need right away
	genField(mSim.markerParticles(), 0.5f, mTriangles);
	//Initialize OpenGL
	initGL();

	
	//Disable polling of mouse motion events by default
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
}

void FluidSim::initGL(){
	//Compile the vertex and fragment shaders for the static shader program
	GLuint vertexShaderID = compileShader(STATIC_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	GLuint fragmentShaderID = compileShader(STATIC_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);

	//Create the program and link the shaders to it
	mProgramStatic = linkProgram(vertexShaderID, fragmentShaderID, 0);
	//Compile the vertex and fragment shaders for the fluid shader program
	vertexShaderID = compileShader(FLUID_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	fragmentShaderID = compileShader(FLUID_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);

	//Create the program and link the shaders to it
	mProgramFluid = linkProgram(vertexShaderID, fragmentShaderID, 0);

	//geometry
	vertexShaderID = compileShader(DENSITY_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	fragmentShaderID = compileShader(DENSITY_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);
	GLuint geometryShaderID = compileShader(DENSITY_GEOMETRY_SHADER_PATH, GL_GEOMETRY_SHADER);
	//Create the program and link the shaders to it
	mProgramDensity = linkProgram(vertexShaderID, fragmentShaderID, geometryShaderID);

	
	//Compile the vertex and fragment shaders for the fluid shader program
	vertexShaderID = compileShader(TEST_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	fragmentShaderID = compileShader(TEST_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);
	//Create the program and link the shaders to it
	mProgramTest = linkProgram(vertexShaderID, fragmentShaderID, 0);
	initGLTextures();
	initGLObjects();

	getAllUniformLocations();

	//initialize our quaternion
	mQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	//remove this later
	mProjection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 1000.0f);
	mView = glm::lookAt(
		CAMERA_POS,
		glm::vec3(10.0f, 5.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
		);
	mScale = glm::vec3(1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

}

void FluidSim::initGLObjects(){
	//Create a vertex array object
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);

	//Initialize VBO for static vertices(wall and floor)
	glGenBuffers(1, &mVBOs);
	glBindBuffer(GL_ARRAY_BUFFER, mVBOs);
	glBufferData(GL_ARRAY_BUFFER, sizeof(STATIC_VERTICES), STATIC_VERTICES, GL_STATIC_DRAW);

	//Initialize VBO for indices for static vertices
	glGenBuffers(1, &mIBOs);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOs);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(STATIC_INDICES), STATIC_INDICES, GL_STATIC_DRAW);

	//Initialize VBO for fluid vertices
	glGenBuffers(1, &mVBOf);
	glBindBuffer(GL_ARRAY_BUFFER, mVBOf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE) * 300000, nullptr, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TRIANGLE) *mTriangles.size(), &mTriangles[0]);

	//Initialize transform feedback vbo
	glGenBuffers(1, &mTBO);
	glBindBuffer(GL_ARRAY_BUFFER, mTBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE) * 100000, nullptr, GL_DYNAMIC_COPY);

	//Initialize Uniform Buffer
	glGenBuffers(1, &mUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(GLint) * 4096 + 256, nullptr, GL_STATIC_DRAW);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GLint) * 256, LUTS::edgeTable); 
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(GLint) * 256, sizeof(GLint) * 4096, LUTS::triTable);
	GLint location = glGetUniformBlockIndex(mProgramListTriangles, "luts");
	glUniformBlockBinding(mProgramListTriangles, location, 0);

	//initialize framebuffer and give its output texture to it
	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mScalarFieldTexture, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw  OpenGLException("fewfwfwef");
	//set the list of draw buffers
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FluidSim::initGLTextures(){
	//Load floor texture
	std::vector<png_byte> imageData;
	std::pair<int, int> widthAndHeight = loadImage(TEXTURE_PATH, imageData);

	//Initialize cube map
	glGenTextures(1, &mCubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mCubeMap);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);

	//load texture for walls and ceiling, then send it to the rest of the cube map
	widthAndHeight = loadImage(TEXTURE_WALL_PATH, imageData);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//Initialize texture to store particle positions
	glGenTextures(1, &mParticleTexture);
	glBindTexture(GL_TEXTURE_2D, mParticleTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, 10000, 1, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Initialize texture to store scalar field
	glGenTextures(1, &mScalarFieldTexture);
	glBindTexture(GL_TEXTURE_3D, mScalarFieldTexture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, SIM_WIDTH + 1, SIM_HEIGHT + 1, SIM_DEPTH + 1, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

}

void FluidSim::getAllUniformLocations(){
	mUniformLocMVPs = glGetUniformLocation(mProgramStatic, "MVP");
	mUniformLocMVPf = glGetUniformLocation(mProgramFluid, "MVP");
	mUniformLocM = glGetUniformLocation(mProgramFluid, "M");
	mUniformLocCubeMaps = glGetUniformLocation(mProgramStatic, "cubeMap");
	mUniformLocCubeMapf = glGetUniformLocation(mProgramFluid, "cubeMap");
	mUniformParticles = glGetUniformLocation(mProgramDensity, "particles");
	mUniformDimensions = glGetUniformLocation(mProgramDensity, "dimensions");
	mUniformnParticles = glGetUniformLocation(mProgramDensity, "nParticles");
	mUniformRadiusSquared = glGetUniformLocation(mProgramDensity, "radiusSquared");
	mUniformSField = glGetUniformLocation(mProgramListTriangles, "sField");
	mUniformDimensions3D = glGetUniformLocation(mProgramListTriangles, "dimensions");
}
GLuint FluidSim::compileShader(const char* srcPath, GLenum shaderType){
	//Create the shader
	GLuint shaderID = glCreateShader(shaderType);
	//Read the shader source code from the file
	std::ifstream in(srcPath);
	if (!in)
	{
		std::string errorMessage = "Could not open file shader file at \"";
		errorMessage += srcPath;
		errorMessage += "\".";
		throw OpenGLException(errorMessage);
	}
	std::string srcString = static_cast<std::ostringstream&>(std::ostringstream{} << in.rdbuf()).str();
	//Compile shader
	GLchar const * src = srcString.c_str();
	glShaderSource(shaderID, 1, &src, nullptr);
	glCompileShader(shaderID);

	//Check compile status
	GLint status, infoLogLength;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);

	//If compilation failed, get the error message and throw an exception
	if (status != GL_TRUE)
	{
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<char> errorMessage(infoLogLength);
		glGetShaderInfoLog(shaderID, infoLogLength, nullptr, &errorMessage[0]);
		throw OpenGLException(&errorMessage[0]);
	}

	//return the successfuly compiled shader ID;
	return shaderID;
}

GLuint FluidSim::linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID, GLuint geometryShaderID){
	//Create the program
	GLuint programID = glCreateProgram();

	//Attach the vertex and fragment shaders
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	if (geometryShaderID) glAttachShader(programID, geometryShaderID);
	//Link the program
	glLinkProgram(programID);

	//Check the link status
	GLint status, infoLogLength;
	glGetProgramiv(programID, GL_LINK_STATUS, &status);

	//If the link failed, get the error message and throw an exception
	if (status != GL_TRUE)
	{
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<char> errorMessage(infoLogLength);
		glGetProgramInfoLog(programID, infoLogLength, nullptr, &errorMessage[0]);
		throw OpenGLException(&errorMessage[0]);
	}

	//Cleanup the shaders
	glDetachShader(programID, vertexShaderID);
	glDeleteShader(vertexShaderID);
	glDetachShader(programID, fragmentShaderID);
	glDeleteShader(fragmentShaderID);
	if (geometryShaderID)
	{
		glDetachShader(programID, geometryShaderID);
		glDeleteShader(geometryShaderID);
	}

	//Return the successfully linked program
	return programID;
}

void FluidSim::render(){

	//Generate MVP matrix for the room.
	glm::mat4 translation = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 rotation = glm::mat4(1.0f);
	glm::mat4 scaling = glm::scale(glm::mat4(), glm::vec3(50.0f, 50.0f, 50.0f)); 
	glm::mat4 model = translation * rotation * scaling;
	glm::mat4 MVP = mProjection * mView * model;


	//use our static shader program
	glUseProgram(mProgramStatic);
	//Clear Buffer
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Send the shader our MVP matrix
	glUniformMatrix4fv(mUniformLocMVPs, 1, GL_FALSE, &MVP[0][0]);

	//Bind the cube map texture then send it to the shader
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mCubeMap);
	glUniform1i(mUniformLocCubeMaps, 0);

	//Enable vertex attrib array 0, vertices, and bind the static vbo.
	glEnableVertexAttribArray(0); 
	glBindBuffer(GL_ARRAY_BUFFER, mVBOs);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); // vertices

	//Bind IBO and draw the room
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBOs); 
	glDrawElements(GL_TRIANGLES, sizeof(STATIC_INDICES) / sizeof(GLushort), GL_UNSIGNED_SHORT, nullptr);

	//calculate MVP matrix for fluid
	translation = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
	rotation = glm::mat4_cast(mQuat);
	scaling = glm::scale(glm::mat4(), mScale);
	model = translation * rotation * scaling;
	MVP = mProjection * mView * model;
	
	//Use our fluid shader program
	glUseProgram(mProgramFluid);
	glEnableVertexAttribArray(1); //Attribute 1 is normals, attribute 0 is still vertices.
	//Send the uniforms to shader, the cubemap texture, MVP matrix, model matrix,and camera position.
	glUniform1i(mUniformLocCubeMapf, 0);
	glUniformMatrix4fv(mUniformLocMVPf, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(mUniformLocM, 1, GL_FALSE, &model[0][0]);
	GLint location = glGetUniformLocation(mProgramFluid, "cameraPos");
	glUniform3f(location, 10.0f, 15.0f, -30.0f);

	glBindBuffer(GL_ARRAY_BUFFER, mVBOf); //bind VBO for fluid vertices

	//Set attrib pointers for vertices and normals
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6, nullptr); 
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 6, (GLvoid*)(sizeof(GLfloat) * 3));

	//Render fluid
	glDrawArrays(GL_TRIANGLES, 0, 3 * mTriangles.size());

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
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
	glViewport(0, 0, SIM_WIDTH + 1, SIM_HEIGHT + 1);
	glClear(GL_COLOR_BUFFER_BIT);
	//use density shader program
	glUseProgram(mProgramDensity);

	//set uniforms
	std::vector<glm::vec3> morestuff(10000);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mParticleTexture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, &morestuff[0]);
	glUniform1i(mUniformParticles, 0);

	glUniform1i(mUniformnParticles, mSim.markerParticles().size());
	glUniform2i(mUniformDimensions, SIM_WIDTH + 1, SIM_HEIGHT + 1); 
	glUniform1f(mUniformRadiusSquared, 1.0f);

	//draw
	glDrawArrays(GL_POINTS, 0, SIM_DEPTH + 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, mWidth, mHeight);
}

void FluidSim::genTriangleList(){
	//disable rasterizer
	glEnable(GL_RASTERIZER_DISCARD);

	//Set uniforms
	

	//set transform feedback varyings
	const GLchar* feedbackVaryings[] = { "z6_y6_x6_edge1_edge2_edge3" };
	glTransformFeedbackVaryings(mProgramListTriangles, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	
	//set uniforms
	glUniform3i(mUniformDimensions3D, SIM_WIDTH, SIM_HEIGHT, SIM_DEPTH); //dimensions
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, mScalarFieldTexture);
	glUniform1i(mUniformSField, 0);  //3d texture containing the scalar field

	//bind transform feedback buffer object. Only one tbo so we just use index 0.
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTBO);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, mUBO);

	//bind Uniform Buffer Object. Only one ubo so again we can just use index 0.
	//end transform feedback mode using points
	glBeginTransformFeedback(GL_POINTS);

	//Draw call to write data into transform feedback buffer.
	//Don't need input, just set the uniforms. One point for each position in scalar field grid.
	glDrawArrays(GL_POINTS, 0, (SIM_WIDTH) * (SIM_HEIGHT) * (SIM_DEPTH));

	//end transform feedback, flush, and renable rasterization.
	glEndTransformFeedback();
	glFlush();
	glDisable(GL_RASTERIZER_DISCARD);;

}

void FluidSim::genVertices(){

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
	case SDLK_n: //update simulation
		mSim.update(0.5f);
		mTriangles.clear();
		genField(mSim.markerParticles(), 0.5f, mTriangles);
		glBindBuffer(GL_ARRAY_BUFFER, mVBOf);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TRIANGLE)*mTriangles.size(), &mTriangles[0]);
		break;
	case SDLK_q:
		glBindTexture(GL_TEXTURE_2D, mParticleTexture);
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

void FluidSim::shutdown(){
	//Deallocate shader programs
	glDeleteProgram(mProgramFluid);
	glDeleteProgram(mProgramStatic);
	//Delete the cubemap texture
	glDeleteTextures(1, &mCubeMap);
	//Delete any opengl objects
	glDeleteBuffers(1, &mVBOs);
	glDeleteBuffers(1, &mVBOf);
	glDeleteBuffers(1, &mIBOs);
	glDeleteVertexArrays(1, &mVAO);
	//Destroy the window
	SDL_DestroyWindow(mWindow);
	mWindow = nullptr;

	//Quit SDL
	SDL_Quit();
}

void FluidSim::runSim(){
	//Initialize the program
	init();

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
			//User requests quit
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
		//render();

		//Swap buffers
		//SDL_GL_SwapWindow(mWindow);
	}

	//Free resources before ending the program
	shutdown();
}

std::pair<int, int> FluidSim::loadImage(const char* path, std::vector<png_byte>& imageData){
	
	imageData.clear();

	
	std::ifstream in(path, std::ios::in | std::ios::binary);
	if (!in)
	{
		std::cerr << "Could not open file at " << path << std::endl;
		return std::pair<int, int>(-1, 0);
	}

	png_byte header[8];

	in.read(reinterpret_cast<char*>(header), 8);

	if (png_sig_cmp(header, 0, 8))
	{
		std::cerr << "File at " << path << "is not a PNG" << std::endl;
		in.close();
		return std::pair<int, int>(-1, 1);
	}

	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr)
	{
		std::cerr << "png_create_read_struct returned 0" << std::endl;
		in.close();
		return std::pair<int, int>(-1, 2);
	}

	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
	{
		std::cerr << "png_create_info_struct returned 0" << std::endl;
		png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
		in.close();
		return std::pair<int, int>(-1, 3);
	}
	png_infop endInfo = png_create_info_struct(pngPtr);
	if (!endInfo)
	{
		std::cerr << "png_create_info_struct returned 0" << std::endl;
		png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);
		in.close();
		return std::pair<int, int>(-1, 4);
	}

	if (setjmp(png_jmpbuf(pngPtr)))
	{
		std::cerr << "error from libpng" << std::endl;
		png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
		in.close();
		return std::pair<int, int>(-1, 5);
	}

	//define and set a custom read function, since pnglib doesnt know how to handle c++ structures.
	auto readFunction = [](png_structp pngPtr, png_bytep data, png_size_t length) {
		png_voidp ioPtr = png_get_io_ptr(pngPtr);
		reinterpret_cast<std::istream*>(ioPtr)->read(reinterpret_cast<char*>(data), length);
	};
	png_set_read_fn(pngPtr, reinterpret_cast<png_voidp>(&in), readFunction);

	//let libpng know you already read the first 8 bytes
	png_set_sig_bytes(pngPtr, 8);

	//read all the info up to the image data
	png_read_info(pngPtr, infoPtr);

	//variables to pass to get info
	int bitDepth, colorType;
	png_uint_32 tempWidth, tempHeight;

	//get info about the png
	png_get_IHDR(pngPtr, infoPtr, &tempWidth, &tempHeight, &bitDepth, &colorType, NULL, NULL, NULL);

	//update png info struct
	png_read_update_info(pngPtr, infoPtr);

	//row size in bytes
	int rowBytes = png_get_rowbytes(pngPtr, infoPtr);

	//glTexImage2d requires rows to be 4-byte aligned
	rowBytes += 3 - ((rowBytes - 1) % 4);

	//Allocate the image data as a big block to be given to opengl
	imageData.resize(rowBytes * tempHeight + 15);

	//row pointers is for pointing to image data for reading the png with libpng
	std::vector<png_bytep> rowPointers(tempHeight);

	//Set the individual row pointers to point at the correct offsets of the image data
	for (int i = 0; i < tempHeight; ++i)
		rowPointers[tempHeight - 1 - i] = &imageData[i * rowBytes];

	//Read the png into the image data through the row pointers
	png_read_image(pngPtr, &rowPointers[0]);

	png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
	in.close();
	return std::pair<int, int>(tempWidth, tempHeight);
}


