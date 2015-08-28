#include "MarchingCubes.h"
#include <cmath>

MarchingCubes::MarchingCubes(){
	;
}
MarchingCubes::~MarchingCubes(){
	;
}
/*
Given a grid cell and an isolevel, calculate the triangular
facets required to represent the isosurface through the cell.
Return the number of triangular facets, the array "triangles"
will be loaded up with the vertices at most 5 triangular facets.
0 will be returned if the grid cell is either totally above
of totally below the isolevel.
*/
int MarchingCubes::Polygonise(GRIDCELL& grid, double isolevel, std::vector<TRIANGLE> &triangles)
{
	int i, ntriang;
	int cubeindex;
	glm::vec3 vertlist[12];

	

	/*
	Determine the index into the edge table which
	tells us which vertices are inside of the surface
	*/
	cubeindex = 0;
	if (grid.val[0] < isolevel) cubeindex |= 1;
	if (grid.val[1] < isolevel) cubeindex |= 2;
	if (grid.val[2] < isolevel) cubeindex |= 4;
	if (grid.val[3] < isolevel) cubeindex |= 8;
	if (grid.val[4] < isolevel) cubeindex |= 16;
	if (grid.val[5] < isolevel) cubeindex |= 32;
	if (grid.val[6] < isolevel) cubeindex |= 64;
	if (grid.val[7] < isolevel) cubeindex |= 128;

	/* Cube is entirely in/out of the surface */
	if (edgeTable[cubeindex] == 0)
		return(0);

	/* Find the vertices where the surface intersects the cube */
	if (edgeTable[cubeindex] & 1)
		vertlist[0] =
		VertexInterp(isolevel, grid.p[0], grid.p[1], grid.val[0], grid.val[1]);
	if (edgeTable[cubeindex] & 2)
		vertlist[1] =
		VertexInterp(isolevel, grid.p[1], grid.p[2], grid.val[1], grid.val[2]);
	if (edgeTable[cubeindex] & 4)
		vertlist[2] =
		VertexInterp(isolevel, grid.p[2], grid.p[3], grid.val[2], grid.val[3]);
	if (edgeTable[cubeindex] & 8)
		vertlist[3] =
		VertexInterp(isolevel, grid.p[3], grid.p[0], grid.val[3], grid.val[0]);
	if (edgeTable[cubeindex] & 16)
		vertlist[4] =
		VertexInterp(isolevel, grid.p[4], grid.p[5], grid.val[4], grid.val[5]);
	if (edgeTable[cubeindex] & 32)
		vertlist[5] =
		VertexInterp(isolevel, grid.p[5], grid.p[6], grid.val[5], grid.val[6]);
	if (edgeTable[cubeindex] & 64)
		vertlist[6] =
		VertexInterp(isolevel, grid.p[6], grid.p[7], grid.val[6], grid.val[7]);
	if (edgeTable[cubeindex] & 128)
		vertlist[7] =
		VertexInterp(isolevel, grid.p[7], grid.p[4], grid.val[7], grid.val[4]);
	if (edgeTable[cubeindex] & 256)
		vertlist[8] =
		VertexInterp(isolevel, grid.p[0], grid.p[4], grid.val[0], grid.val[4]);
	if (edgeTable[cubeindex] & 512)
		vertlist[9] =
		VertexInterp(isolevel, grid.p[1], grid.p[5], grid.val[1], grid.val[5]);
	if (edgeTable[cubeindex] & 1024)
		vertlist[10] =
		VertexInterp(isolevel, grid.p[2], grid.p[6], grid.val[2], grid.val[6]);
	if (edgeTable[cubeindex] & 2048)
		vertlist[11] =
		VertexInterp(isolevel, grid.p[3], grid.p[7], grid.val[3], grid.val[7]);

	/* Create the triangle */
	ntriang = 0;
	for (i = 0; triTable[cubeindex][i] != -1; i += 3) {
		TRIANGLE triangle;
		triangle.p[0] = vertlist[triTable[cubeindex][i]];
		triangle.p[1] = vertlist[triTable[cubeindex][i + 1]];
		triangle.p[2] = vertlist[triTable[cubeindex][i + 2]];
		triangles.push_back(triangle);
		ntriang++;
	}

	return(ntriang);
}

/*
Linearly interpolate the position where an isosurface cuts
an edge between two vertices, each with their own scalar value
*/
glm::vec3 MarchingCubes::VertexInterp(double isolevel, glm::vec3 p1, glm::vec3 p2, double valp1, double valp2)
{
	double mu;
	glm::vec3 p;

	if (abs(isolevel - valp1) < 0.00001)
		return(p1);
	if (abs(isolevel - valp2) < 0.00001)
		return(p2);
	if (abs(valp1 - valp2) < 0.00001)
		return(p1);
	mu = (isolevel - valp1) / (valp2 - valp1);
	p.x = p1.x + mu * (p2.x - p1.x);
	p.y = p1.y + mu * (p2.y - p1.y);
	p.z = p1.z + mu * (p2.z - p1.z);

	return(p);
}

void MarchingCubes::genField(const std::vector<glm::vec3>& particles, float radius, std::vector<TRIANGLE>& triangles){
	std::map<glm::vec3, double, vecComparators> grid;
	int lowestX = 100000, highestX = 0, lowestY = 100000, highestY = 0, lowestZ = 1000000, highestZ = 0;
	for (auto it = particles.begin(); it != particles.end(); ++it){
		glm::vec3 p = (*it);
		int lowerX = floor(p.x - BLOB_RADIUS);
		int higherX = ceil(p.x + BLOB_RADIUS);
		int lowerY = floor(p.y - BLOB_RADIUS);
		int higherY = ceil(p.y + BLOB_RADIUS);
		int lowerZ = floor(p.z - BLOB_RADIUS);
		int higherZ = ceil(p.z + BLOB_RADIUS);
		glm::vec3 point;
		for (int x = lowerX; x <= higherX; ++x){
			point.x = x;
			for (int y = lowerY; y <= higherY; ++y){
				point.y = y;
				for (int z = lowerZ; z <= higherZ; ++z){
					point.z = z;
					setFieldValue(grid, p, point);

				}
			}
		}
		if (lowerX < lowestX) lowestX = lowerX;
		if (higherX > highestX) highestX = higherX;
		if (lowerY < lowestY) lowestY = lowerY;
		if (higherY > highestY) highestY = higherY;
		if (lowerZ < lowestZ) lowestZ = lowerZ;
		if (higherZ > highestZ) highestZ = higherZ;
	}
	for (float x = lowestX; x < highestX; x += VOXEL_SIZE)
		for (float y = lowestY; y < highestY; y += VOXEL_SIZE)
			for (float z = lowestZ; z < highestZ; z += VOXEL_SIZE){
				GRIDCELL cell;
				setGridCellValue(grid, cell, 0, x, y, z + VOXEL_SIZE);
				setGridCellValue(grid, cell, 1, x + VOXEL_SIZE, y, z + VOXEL_SIZE);
				setGridCellValue(grid, cell, 2, x + VOXEL_SIZE, y, z);
				setGridCellValue(grid, cell, 3, x, y, z);
				setGridCellValue(grid, cell, 4, x, y + VOXEL_SIZE, z + VOXEL_SIZE);
				setGridCellValue(grid, cell, 5, x + VOXEL_SIZE, y + VOXEL_SIZE, z + VOXEL_SIZE);
				setGridCellValue(grid, cell, 6, x + VOXEL_SIZE, y + VOXEL_SIZE, z);
				setGridCellValue(grid, cell, 7, x, y + VOXEL_SIZE, z);
				Polygonise(cell, 0.0001, triangles);
			}
	int i = 0;
}

inline void MarchingCubes::setFieldValue(std::map<glm::vec3, double, vecComparators>& grid, glm::vec3& particle, glm::vec3& point){
	double distance = (pow(point.x - particle.x, 2) + pow(point.y - particle.y, 2) + pow(point.z - particle.z, 2));
	if (distance > BLOB_RADIUS_SQUARED) return;
	double value = pow(1 - distance, 2);
	std::pair<std::map<glm::vec3, double, vecComparators>::iterator, bool> ret = grid.insert(std::pair<glm::vec3, double>(point, value));
	if (!ret.second)
		(*ret.first).second += value;

}




inline void MarchingCubes::setGridCellValue(std::map<glm::vec3, double, vecComparators>& grid, GRIDCELL& cell, int index, float x, float y, float z){
	cell.p[index].x = x;
	cell.p[index].y = y;
	cell.p[index].z = z;
	auto it = grid.find(cell.p[index]);
	cell.val[index] = it != grid.end() ? (*it).second : 0.0;
}