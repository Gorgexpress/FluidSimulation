#include "Utility.h"
#include "OpenGLException.h"
#include "SDLException.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

GLuint util::compileShader(const char* srcPath, GLenum shaderType){
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

GLuint util::linkProgram(GLuint vertexShaderID, GLuint geometryShaderID, GLuint fragmentShaderID, const GLchar* varyings[], int varyingsCount){
	//Create the program
	GLuint programID = glCreateProgram();

	//Attach the vertex and fragment shaders
	glAttachShader(programID, vertexShaderID);
	if (fragmentShaderID) glAttachShader(programID, fragmentShaderID);
	if (geometryShaderID) glAttachShader(programID, geometryShaderID);

	//setup varyings if there are any
	if (varyingsCount > 0)
		glTransformFeedbackVaryings(programID, varyingsCount, varyings, GL_INTERLEAVED_ATTRIBS);


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
	if (fragmentShaderID)
	{
		glDetachShader(programID, fragmentShaderID);
		glDeleteShader(fragmentShaderID);
	}
	if (geometryShaderID)
	{
		glDetachShader(programID, geometryShaderID);
		glDeleteShader(geometryShaderID);
	}

	//Return the successfully linked program
	return programID;
}

GLuint util::linkProgram(GLuint vertexShaderID, GLuint geometryShaderID, GLuint fragmentShaderID){
	return linkProgram(vertexShaderID, fragmentShaderID, geometryShaderID, nullptr, 0);
}

GLuint util::linkProgram(GLuint vertexShaderID, GLuint fragmentShaderID){
	return linkProgram(vertexShaderID, fragmentShaderID, 0, nullptr, 0);
}

util::dimensions2D util::loadImage(const char* path, std::vector<png_byte>& imageData){

	imageData.clear();

	std::ifstream in(path, std::ios::in | std::ios::binary);
	if (!in)
	{
		std::cerr << "Could not open file at " << path << std::endl;
		return dimensions2D(-1, 0);
	}

	png_byte header[8];

	in.read(reinterpret_cast<char*>(header), 8);

	if (png_sig_cmp(header, 0, 8))
	{
		std::cerr << "File at " << path << "is not a PNG" << std::endl;
		in.close();
		return dimensions2D(-1, 1);
	}

	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr)
	{
		std::cerr << "png_create_read_struct returned 0" << std::endl;
		in.close();
		return dimensions2D(-1, 2);
	}

	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr)
	{
		std::cerr << "png_create_info_struct returned 0" << std::endl;
		png_destroy_read_struct(&pngPtr, (png_infopp)NULL, (png_infopp)NULL);
		in.close();
		return dimensions2D(-1, 3);
	}
	png_infop endInfo = png_create_info_struct(pngPtr);
	if (!endInfo)
	{
		std::cerr << "png_create_info_struct returned 0" << std::endl;
		png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)NULL);
		in.close();
		return dimensions2D(-1, 4);
	}

	if (setjmp(png_jmpbuf(pngPtr)))
	{
		std::cerr << "error from libpng" << std::endl;
		png_destroy_read_struct(&pngPtr, &infoPtr, &endInfo);
		in.close();
		return dimensions2D(-1, 5);
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
	return dimensions2D(tempWidth, tempHeight);
}

uint64_t util::GetTimeMs64() {
	#ifdef _WIN32
		/* Windows */
		FILETIME ft;
		LARGE_INTEGER li;

		/* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
		* to a LARGE_INTEGER structure. */
		GetSystemTimeAsFileTime(&ft);
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;

		uint64_t ret = li.QuadPart;
		ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
		ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

		return ret;
	#else
		/* Linux */
		struct timeval tv;

		gettimeofday(&tv, NULL);

		uint64_t ret = tv.tv_usec;
		/* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
		ret /= 1000;

		/* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
		ret += (tv.tv_sec * 1000);

		return ret;
#endif
	
}


