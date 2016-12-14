#ifndef _CFDSIMULATION_H_
#define _CFDSIMULATION_H_

#include <vector>
#include <math.h>
#include <glm/glm.hpp>

const int DEFAULT_FLUID_WIDTH = 25;
const int DEFAULT_FLUID_HEIGHT = 25;
const int DEFAULT_FLUID_DEPTH = 25;
const unsigned int DEFAULT_FLUID_VOLUME = DEFAULT_FLUID_WIDTH * DEFAULT_FLUID_HEIGHT * DEFAULT_FLUID_DEPTH;

class CFDSimulation{
	

public:
	CFDSimulation();
	CFDSimulation(int width, int height, int depth);
	void update(float dt);
	void resize(int width, int height, int depth);
	const std::vector<glm::vec3>& markerParticles() const { return mMarkerParticles; }
private:
	std::vector<glm::vec3> mVelocity, mVelocityBuffer;
	std::vector<float> mPressure, mBuffer, mBuffer2;
	std::vector<glm::vec3> mMarkerParticles;
	std::vector<bool> mFluid;
	int mWidth, mHeight, mDepth;

	//void resize(int width, int height, int depth);
	void updateParticles(float dt);
	void advectVelocity(float dt);
	void diffuseVelocity();
	void project();
	void setBoundariesVelocity(std::vector<glm::vec3>& velocity);
	void setBoundariesPressure(std::vector<float>& pressure);
	void advectQuantity(std::vector<float>& q, float dt);
	void diffuseQuantity(std::vector<float>& q);
	void determineFluidCells();
	void applyForces(float dt);
	inline unsigned int Index(int x, int y, int z){
		return x + y * mDepth + z * mWidth * mHeight;
	}

};


#endif

/*
	template <typename T>
	struct vec3{
		T x, y, z;
		inline T sum(){
			return x + y + z;
		}
		inline T dot(const vec3& rhs){
			return x * rhs.x + y * rhs.y + z * rhs.z;
		}
		inline void round(){
			x = std::round(x);
			y = std::round(y);
			z = std::round(z);
		}
	};
	template <typename T>
	inline friend vec3<T> operator+(vec3<T> lhs, const vec3<T>& rhs){
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
		return lhs;
	}
	template <typename T>
	inline friend vec3<T> operator-(vec3<T> lhs, const vec3<T>& rhs){
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
		return lhs;
	}
	template <typename T>
	inline friend vec3<T> operator/(vec3<T> lhs, const float& scalar){
		lhs.x /= scalar;
		lhs.y /= scalar;
		lhs.z /= scalar;
		return lhs;
	}
	template <typename T>
	inline friend vec3<T> operator*(vec3<T> lhs, const float& scalar){
		lhs.x *= scalar;
		lhs.y *= scalar;
		lhs.z *= scalar;
		return lhs;
	}
*/