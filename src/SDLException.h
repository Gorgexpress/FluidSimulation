#ifndef _SDLEXCEPTION_H_
#define _SDLEXCEPTION_H_

#include <stdexcept>
#include <exception>
#include <string>
class SDLException : public std::runtime_error{
public:
	SDLException() :runtime_error("SDL exception"){}
	SDLException(const std::string& msg) :runtime_error(msg.c_str()){}
};
#endif
