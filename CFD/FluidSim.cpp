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


//Constants for default window width and height
const int DEFAULT_WINDOW_WIDTH = 640;
const int DEFAULT_WINDOW_HEIGHT = 480;

//Constant for file paths to shaders
const char* STATIC_VERTEX_SHADER_PATH = "shaders/static.glslv";
const char* STATIC_FRAGMENT_SHADER_PATH = "shaders/static.glslf";
const char* FLUID_VERTEX_SHADER_PATH = "shaders/cube_150.glslv";
const char* FLUID_FRAGMENT_SHADER_PATH = "shaders/cube_150.glslf";


//Path to texture
const char* TEXTURE_PATH = "floor_diamond_tileIII.png";
const char* TEXTURE_WALL_PATH = "stone wall 3.png";

//Constants for VBO array indices
const int VBO_STATIC_VERTICES = 0; //Will be used for the floor, walls, anything that doesn't change
const int VBO_STATIC_INDICES = 1;
const int VBO_FLUID_VERTICES = 2;

//Constants for ProgramID array indices
const int STATIC_PROGRAM = 0;
const int FLUID_PROGRAM = 1;

const GLfloat STATIC_VERTICES[] = {
	//floor
	-50.0f, 0.0f, -50.0f, 0.0f, 0.0f, //back left: 0
	-50.0f, 0.0f, 50.0f, 0.0f, 1.0f, //front left: 1
	50.0f, 0.0f, -50.0f, 1.0f, 0.0f, //back right: 2
	50.0f, 0.0f, 50.0f, 1.0f, 1.0f, //front right: 3
	//back wall
	-50.0f, 50.0f, -50.0f, 0.0f, 1.0f, //back left: 4
	50.0f, 50.0f, -50.0f, 1.0f, 1.0f, //back right: 5
	//left wall
	-50.0f, 0.0f, 50.0f, 1.0f, 0.0f, // lower front left: 6
	-50.0f, 50.0f, 50.0f, 1.0f, 1.0f, // front left: 7
	//right wall
	50.0f, 0.0f, 50.0f, 0.0f, 0.0f, //lower front right: 8
	50.0f, 50.0f, 50.0f, 0.0f, 1.0f, // upper front right: 9
	//front wall
};

const GLushort STATIC_INDICES[] = {
	0, 1, 2, 3, 2, 1,
	0, 2, 4, 5, 4, 2,
	4, 6, 0, 6, 4, 7,
	8, 9, 2, 5, 2, 9,
	8, 6, 9, 7, 9, 6

};
FluidSim::FluidSim(){

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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
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

	//Initialize our sim, we arent gonna update it yet so just get whatever info we need right away
	MarchingCubes mcubes;
	mcubes.genField(mSim.markerParticles(), 0.5f, mTriangles);

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
	mProgramID[STATIC_PROGRAM] = linkProgram(vertexShaderID, fragmentShaderID);

	//Compile the vertex and fragment shaders for the fluid shader program
	vertexShaderID = compileShader(FLUID_VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	fragmentShaderID = compileShader(FLUID_FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);

	//Create the program and link the shaders to it
	mProgramID[FLUID_PROGRAM] = linkProgram(vertexShaderID, fragmentShaderID);

	//Create a vertex array object
	glGenVertexArrays(1, &mVAO);
	glBindVertexArray(mVAO);

	//Create Vertex Buffer Object
	glGenBuffers(3, &mVBOs[0]);

	//Bind static vertices buffer
	glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBO_STATIC_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(STATIC_VERTICES), STATIC_VERTICES, GL_STATIC_DRAW);

	//Bind static indices buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBOs[VBO_STATIC_INDICES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(STATIC_INDICES), STATIC_INDICES, GL_STATIC_DRAW);


	//Bind Fluid Vertices buffer
	glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBO_FLUID_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE) * 100000, nullptr, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TRIANGLE) *mTriangles.size(), &mTriangles[0]);

	//Load the png we are using for a texture
	std::vector<png_byte> imageData;
	std::pair<int, int> widthAndHeight = loadImage(TEXTURE_PATH, imageData);

	//Generate the OpenGL texture 
	glGenTextures(1, &mTexture);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//Load the png we are using for a texture
	widthAndHeight = loadImage(TEXTURE_WALL_PATH, imageData);

	//Generate the OpenGL texture 
	glGenTextures(1, &mTexture2);
	glBindTexture(GL_TEXTURE_2D, mTexture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widthAndHeight.first, widthAndHeight.second, 0, GL_RGB, GL_UNSIGNED_BYTE, &imageData[0]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//initialize our quaternion
	mQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	//remove this later
	mProjection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	mView = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 50.0f),
		glm::vec3(10.0f, 10.0f, 10.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
		);
	mScale = glm::vec3(1.0f, 1.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

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

GLuint FluidSim::linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID){
	//Create the program
	GLuint programID = glCreateProgram();

	//Attach the vertex and fragment shaders
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);

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

	//Return the successfully linked program
	return programID;
}

std::pair<int,int> FluidSim::loadImage(const char* path, std::vector<png_byte>& imageData){
	
	imageData.clear();

	//pnglib is a C library, so it's easier to stick to C structures. Can change this later if
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

	//set read function
	png_set_read_fn(pngPtr, reinterpret_cast<png_voidp>(&in), FluidSim::readFunction);

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
	//rowBytes += 3 - ((rowBytes - 1) % 4);

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
void FluidSim::render(){

	//Set background color to black and clear the screen
	glm::mat4 translation = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 rotation = glm::mat4_cast(mQuat);
	glm::mat4 scaling = glm::scale(glm::mat4(), mScale);
	glm::mat4 model = translation * rotation * scaling;
	glm::mat4 MVP = mProjection * mView * model;
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//DRAW THE FLOOR
	//use our static shader program
	glUseProgram(mProgramID[STATIC_PROGRAM]);

	GLint location = glGetUniformLocation(mProgramID[STATIC_PROGRAM], "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, &MVP[0][0]);
	location = glGetUniformLocation(mProgramID[STATIC_PROGRAM], "myTextureSampler");
	//Set our active texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mTexture2);
	glUniform1i(location, 0);
	glEnableVertexAttribArray(0); //Attribute 0: vertices
	glEnableVertexAttribArray(1); //Attribute 1: texture coords
	glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBO_STATIC_VERTICES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(sizeof(GLfloat) * 3));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBOs[VBO_STATIC_INDICES]);
	//Draw floor
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
	glUniform1i(location, 1);
	glDrawElements(GL_TRIANGLES, (sizeof(STATIC_INDICES) / sizeof(GLushort)) - 6, GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * 6));

	glDisableVertexAttribArray(1);
	//Use our shader program
	glUseProgram(mProgramID[FLUID_PROGRAM]);

	location = glGetUniformLocation(mProgramID[FLUID_PROGRAM], "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, &MVP[0][0]);
	//Attribute 0: vertices
	glBindBuffer(GL_ARRAY_BUFFER, mVBOs[VBO_FLUID_VERTICES]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//Render
	glDrawArrays(GL_TRIANGLES, 0, 3 * mTriangles.size());

	//cleanup
	glDisableVertexAttribArray(0);
	glUseProgram(NULL);
	GLenum err = glGetError();
	while (err != GL_NO_ERROR)
	{
		std::cerr << err << std::endl;
		err = glGetError();
	}
}

void FluidSim::handleKeyDownEvent(SDL_Keycode key){
	switch (key)
	{
	case SDLK_x:
		if (mScale.x < 3.0f)
		{
			mScale.x += 0.1f;
			mScale.y += 0.1f;
			mScale.z += 0.1f;
		}
		break;
	case SDLK_z:
		if (mScale.x > 0.2f)
		{
			mScale.x -= 0.1f;
			mScale.y -= 0.1f;
			mScale.z -= 0.1f;
		}
		break;
	case SDLK_n:
		mSim.update(0.2f);
		MarchingCubes cubes;
		mTriangles.clear();
		cubes.genField(mSim.markerParticles(), 0.5f, mTriangles);
		glBindBuffer(GL_ARRAY_BUFFER, mVBOs[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TRIANGLE)*mTriangles.size(), &mTriangles[0]);
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
	for (auto programID : mProgramID)
		glDeleteProgram(programID);

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
		render();

		//Swap buffers
		SDL_GL_SwapWindow(mWindow);
	}

	//Free resources before ending the program
	shutdown();
}

void FluidSim::readFunction(png_structp pngPtr, png_bytep data, png_size_t length){
	png_voidp ioPtr = png_get_io_ptr(pngPtr);
	reinterpret_cast<std::istream*>(ioPtr)->read(reinterpret_cast<char*>(data), length);
}