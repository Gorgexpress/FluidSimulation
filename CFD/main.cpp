#include <SDL/SDL.h>
#undef main
#include <GL/glew.h>
#include <SDL/SDL_opengl.h>
#include <GL/GLU.h>
#include "SDLException.h"
#include "OpenGLException.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>
#include "MarchingCubes.h"
#include "CFDSimulation.h"
#pragma comment(lib, "GLEW/glew32.lib")
#pragma comment(lib, "SDL/x64/sdl2.lib")
#pragma comment(lib, "opengl32.lib")

//Forward declarations of functions:

//Initializes SDL, GLEW, and creates the window before calling initGL(). 
void init();

//Initializes the rendering program
void initGL();

//Compiles a given shader of a given type
GLuint compileShader(const char* srcPath, GLenum shaderType);

//Creates and links a program with a vertex shader and fragment shader
GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID);

//Main render method
void render();

//Event handling functions
void handleKeyDownEvent(SDL_Keycode key);
void handleMouseButtonDownEvent(Uint8 button);
void handleMouseButtonUpEvent(Uint8 button);
void handleMouseMotionEvent(Sint32 xrel, Sint32 yrel);

//Deallocates resources used by SDL and OpenGL
void shutdown();

//Constants for default window width and height
const int DEFAULT_WINDOW_WIDTH = 640;
const int DEFAULT_WINDOW_HEIGHT = 480;

//Constant for file paths to shaders
const char* VERTEX_SHADER_PATH = "shaders/cube_150.glslv";
const char* FRAGMENT_SHADER_PATH = "shaders/cube_150.glslf";

//Constant for vertices
const GLfloat gVertexData[] = {
	-1.0f, -1.0f, -1.0f, // triangle 1 : begin
	-1.0f, -1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, // triangle 1 : end
	1.0f, 1.0f, -1.0f, // triangle 2 : begin
	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f, // triangle 2 : end
	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, -1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, -1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, -1.0f, 1.0f
};

//The window we'll be rendering too
SDL_Window* gWindow = nullptr;

//OpenGL context
SDL_GLContext gContext;

//Shader Program
GLuint gProgramID = 0;

//Vertex Array Object
GLuint gVAO;

//Vertex Buffer Object
GLuint gVBO;
GLuint gIBO;


//matrices
glm::mat4 gProjection, gView;
//rotation quaternion
glm::quat gQuat;
//scaling vector
glm::vec3 gScale;

//vector of triangles
std::vector<TRIANGLE> gTriangles;

//Simulation
CFDSimulation gSim;

void init(){
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
	gWindow = SDL_CreateWindow("A Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
		, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (gWindow == nullptr)
	{
		std::string errorMessage = "Window could not be created! SDL Error: ";
		errorMessage += SDL_GetError();
		throw SDLException(errorMessage);
	}
	//Create Context
	gContext = SDL_GL_CreateContext(gWindow);
	if (gContext == nullptr)
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

	//Reserve space for our vector of triangles
	gTriangles.reserve(5000);
	//Initialize OpenGL
	initGL();

	//Disable polling of mouse motion events by default
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
}

void initGL(){
	//Compile the vertex and fragment shaders
	GLuint vertexShaderID = compileShader(VERTEX_SHADER_PATH, GL_VERTEX_SHADER);
	GLuint fragmentShaderID = compileShader(FRAGMENT_SHADER_PATH, GL_FRAGMENT_SHADER);

	//Create the program and link the shaders to it
	gProgramID = linkProgram(vertexShaderID, fragmentShaderID);

	//Create a vertex array object
	glGenVertexArrays(1, &gVAO);
	glBindVertexArray(gVAO);

	//Create Vertex Buffer Object
	glGenBuffers(1, &gVBO);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE)*5000, &gTriangles[0], GL_DYNAMIC_DRAW);

	gQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	//remove this later
	gProjection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	gView = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 5.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
		);
	gScale = glm::vec3(1.0f, 1.0f, 1.0f);

	glEnable(GL_DEPTH);
}

GLuint compileShader(const char* srcPath, GLenum shaderType){
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

GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID){
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

void render(){
	//Set background color to black and clear the screen
	glm::mat4 translation = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 rotation = glm::mat4_cast(gQuat);
	glm::mat4 scaling = glm::scale(glm::mat4(), gScale);
	glm::mat4 model = translation * rotation * scaling;
	glm::mat4 MVP = gProjection * gView * model;
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	//Use our shader program
	glUseProgram(gProgramID);

	GLint location = glGetUniformLocation(gProgramID, "MVP");
	glUniformMatrix4fv(location, 1, GL_FALSE, &MVP[0][0]);
	//Attribute 0: vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//Render
	glDrawArrays(GL_TRIANGLES, 0, 3*12);

	//cleanup
	glDisableVertexAttribArray(0);
	glUseProgram(NULL);
}

void handleKeyDownEvent(SDL_Keycode key){
	switch (key)
	{
	case SDLK_x:
		if (gScale.x < 3.0f)
		{
			gScale.x += 0.1f;
			gScale.y += 0.1f;
			gScale.z += 0.1f;
		}
		break;
	case SDLK_z:
		if (gScale.x > 0.2f)
		{
			gScale.x -= 0.1f;
			gScale.y -= 0.1f;
			gScale.z -= 0.1f;
		}
		break;
	}
}
void handleMouseButtonDownEvent(Uint8 button){
	if (button == SDL_BUTTON_LEFT)
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
	}
}

void handleMouseButtonUpEvent(Uint8 button){
	if (button == SDL_BUTTON_LEFT)
	{
		SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);
	}
}


void handleMouseMotionEvent(Sint32 xrel, Sint32 yrel){
	glm::quat rot(1.0f, yrel*0.01f, xrel*0.01f, 0.0f);
	gQuat = rot * gQuat;
	gQuat = glm::normalize(gQuat);
}

void shutdown(){
	//Deallocate shader program
	glDeleteProgram(gProgramID);

	//Destroy the window
	SDL_DestroyWindow(gWindow);
	gWindow = nullptr;

	//Quit SDL
	SDL_Quit();
}

int main(){
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
		SDL_GL_SwapWindow(gWindow);
	}

	//Free resources before ending the program
	shutdown();

	return 0;
}