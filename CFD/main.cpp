#include "FluidSim.h"

#pragma comment(lib, "GLEW/glew32.lib")
#pragma comment(lib, "SDL/x64/sdl2.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "libpng/zlib.lib")
#pragma comment(lib, "libpng/libpng16.lib")

int main(){
	FluidSim sim;
	sim.runSim();
}