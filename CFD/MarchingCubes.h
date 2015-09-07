/*
This is 99% copy pasted from a rushed school project so it's going to be a mess.
I know there are some annoying bugs but I don't completely remember how it works.
Will just have to rewrite the parts of the code I wrote, which involves converting
the particle positions to an isosurface. 
*/

#ifndef _MARCHINGCUBES_H_
#define _MARCHINGCUBES_H_
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>


struct TRIANGLE {
	glm::vec3 p[3];
};

struct GRIDCELL {
	glm::vec3 p[8];
	double val[8];
};

struct vec3Hash{
	std::size_t operator()(const glm::vec3 key) const{
		size_t ix = static_cast<int>(floorf(key.x) / 0.5f);
		size_t iy = static_cast<int>(floorf(key.y) / 0.5f);
		size_t iz = static_cast<int>(floorf(key.z) / 0.5f);

		return ((ix * 11113) ^ (iy * 12979) ^ (iz * 13513)) % 15625;
	}
};

int Polygonise(std::unordered_map<glm::vec3, double, vec3Hash>& isoSurface, GRIDCELL& grid, double isolevel, std::vector<TRIANGLE> &triangles);
glm::vec3 VertexInterp(double isolevel, glm::vec3 p1, glm::vec3 p2, double valp1, double valp2);
void genField(const std::vector<glm::vec3>& particles, float radius, std::vector<TRIANGLE>& triangles);
glm::vec3 gradient(std::unordered_map<glm::vec3, double, vec3Hash>& grid, glm::vec3& p);
inline void setFieldValue(std::unordered_map<glm::vec3, double, vec3Hash>& grid, glm::vec3& particle, glm::vec3& point);
inline void setGridCellValue(std::unordered_map<glm::vec3, double, vec3Hash>& grid, GRIDCELL& cell, int index, float x, float y, float z);


#endif