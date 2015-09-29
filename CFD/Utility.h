#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <GL/glew.h>
#include <vector>
#include <libpng/png.h>

namespace util{

	//structure used to contain with and height of an image
	struct dimensions2D{
		int width, height;
		dimensions2D(int width, int height){
			this->width = width;
			this->height = height;
		}
	};
	/*
	Compiles a shader
	@param srcPath the path to the shader file
	@param shaderType the type of shader
	@return the ID of the shader
	*/
	GLuint compileShader(const char* srcPath, GLenum shaderType);

	/*
	Creates a program and links it up with shaders. Also cleans up the shaders once we are done linking them to the program.
	@param vertexShaderID id of vertex shader (required)
	@param geometryShaderID id of the fragment shader (0 for no geometry shader
	@param fragmentShaderID id of fragment shader (0 for no fragment shader)
	@param varyings the const GLchar* array containing the varyings to be captured if transform feedback is used
	@param varyingsCount the number of varyings(if 0, the varyings parameter will be ignored)
	@return the ID of the program
	*/
	GLuint linkProgram(GLuint vertexShaderID, GLuint geometryShaderID, GLuint fragmentShaderID, const GLchar* varyings[], int varyingsCount);
	
	//Similar to above, but does not use varyings.
	GLuint linkProgram(GLuint vertexShaderID, GLuint geometryShaderID, GLuint fragmentShaderID);

	//Similar to above, but does not use a geometry shader. 
	GLuint linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID);

	/*
	Load image using libpng
	@param path the path to the png file
	@param imageData the vector of png_bytes to store the image data. Any data already existing in the
	vector will be cleared out.
	@return a pair of ints containing the width and height of the image respectively.
	If there was an error, first will be set to -1.
	*/
	dimensions2D loadImage(const char* path, std::vector<png_byte>& imageData);
}
#endif