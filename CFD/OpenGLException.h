#ifndef _OPENGLEXCEPTION_H_
#define _OPENGLEXCEPTION_H_
#include <stdexcept>
#include <exception>
class OpenGLException : public std::runtime_error{
public:
	OpenGLException() :runtime_error("OpenGL exception"){}
	OpenGLException(const std::string& msg) :runtime_error(msg.c_str()){}
};
#endif