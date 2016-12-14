#include "CFDSimulation.h"
#include <iostream>
CFDSimulation::CFDSimulation(){
	//Initialize the simulation using the default constants defined in the header
	resize(DEFAULT_FLUID_WIDTH, DEFAULT_FLUID_HEIGHT, DEFAULT_FLUID_DEPTH);

	//Remove this later
	//Hard code setting up the marker particles for testing
	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < 15; ++y)
			for (int z = 1; z < mDepth - 1; ++z)
			{
				mFluid[Index(x, y, z)] = true;
				mMarkerParticles.push_back(glm::vec3(x + 0.5f , y + 0.5f, z + 0.5f));
				mVelocity[Index(x, y, z)] = glm::vec3(0.0f, 200.0f, 0.0f);
				
			}



	


	
}

CFDSimulation::CFDSimulation(int width, int height, int depth){
	//Initialize the simulation using the given width, height, and depth
	resize(width, height, depth);
}

void CFDSimulation::update(float dt){
	/*
	updateParticles(dt);
	diffuseVelocity();
	advectVelocity(dt);
	diffuseVelocity();
	project();
	*/
	determineFluidCells();
	applyForces(dt);
	diffuseVelocity();
	project();
	advectVelocity(dt);
	project();
	updateParticles(dt);
}

void CFDSimulation::updateParticles(float dt)
{
	for (auto &p : mMarkerParticles)
	{
		//no safety checks for now, nothing should even be moving
		glm::vec3 newPosition = p + mVelocity[(Index(static_cast<int>(p.x), static_cast<int>(p.y), static_cast<int>(p.z)))] * dt;
		p = newPosition;

	}
}
void CFDSimulation::resize(int width, int height, int depth){
	//Set the width, height, and depth
	mWidth = width;
	mHeight = height;
	mDepth = depth;
	//Compute the total volume of the fluid
	unsigned int volume = mWidth * mHeight * mDepth;
	//Then reallocate new data structures of the required size with default values.
	mVelocity = std::vector<glm::vec3>(volume, glm::vec3(0.0f, 0.0f, 0.0f));
	mVelocityBuffer = std::vector<glm::vec3>(volume, glm::vec3(0.0f, 0.0f, 0.0f));
	mPressure = std::vector<float>(volume, 0.0f);
	mBuffer = std::vector<float>(volume, 0.0f);
	mBuffer2 = std::vector<float>(volume, 0.0f);
	mFluid = std::vector<bool>(volume, false);
}

void CFDSimulation::advectVelocity(float dt){
	//for every voxel
	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < mHeight - 1; ++y)
			for (int z = 1; z < mDepth - 1; ++z)
			{
				//Predict where the current voxel of fluid was last time step 
				//by using (previous position) = (current position) - velocity * dt.
				glm::vec3 prevPosition = glm::vec3(x, y, z) - mVelocity[Index(x,y,z)] * dt;

				//Get a rounded version of this position
				glm::ivec3 iPrevPosition = glm::round(prevPosition);

				//get the 6 different x, y, and z positions we will need to use as indices for the 8 nieghbors of this position
				int x1 = iPrevPosition.x + 1;
				int y1 = iPrevPosition.y + 1;
				int z1 = iPrevPosition.z + 1;
				int x0 = iPrevPosition.x - 1;
				int y0 = iPrevPosition.y - 1;
				int z0 = iPrevPosition.z - 1;

				//Get the weights to be used for trilinear interpolation. 
				//positive direction(right, above, forward)
				float wx1 = x1 - prevPosition.x;
				float wy1 = y1 - prevPosition.y;
				float wz1 = z1 - prevPosition.z;

				//negative direction is just 1.0 - positive direction weight in this case
				float wx0 = 1.0f - wx1;
				float wy0 = 1.0f - wy1;
				float wz0 = 1.0f - wz1;

				//Do trilinear interpolation
				mVelocityBuffer[Index(x, y, z)] = (wx1 * wy1 * wz1 * mVelocity[Index(x1, y1, z1)] + wx1 * wy1 * wz0 * mVelocity[Index(x1, y1, z0)] +
					wx1 * wy0 * wz1 * mVelocity[Index(x1, y0, z1)] + wx0 * wy1 * wz1 * mVelocity[Index(x0, y1, z1)] +
					wx1 * wy0 * wz0 * mVelocity[Index(x1, y0, z0)] + wx0 * wy1 * wz0 * mVelocity[Index(x0, y1, z0)] +
					wx0 * wy0 * wz1 * mVelocity[Index(x0, y0, z1)] + wx0 * wy0 * wz0 * mVelocity[Index(x0, y0, z0)]);

			
			
			}
	mVelocity.swap(mVelocityBuffer);
	
}

void CFDSimulation::diffuseVelocity(){
	for (int it = 20; it > 0; --it)
	{
		for (int x = 1; x < mWidth - 1; ++x)
			for (int y = 1; y < mHeight - 1; ++y)
				for (int z = 1; z < mDepth - 1; ++z)
				{
					//calculate the divergence of the velocity for this cell
					float divergence = ((mVelocity[Index(x + 1, y, z)].x - mVelocity[Index(x - 1, y, z)].x)
						+ (mVelocity[Index(x, y + 1, z)].y - mVelocity[Index(x, y - 1, z)].y)
						+ (mVelocity[Index(x, y, z + 1)].z - mVelocity[Index(x, y, z - 1)].z)) / 2.0f;
					//Calculate the updated velocity and place it in the buffer.
					mVelocity[Index(x, y, z)] = (mVelocity[Index(x + 1, y, z)] + mVelocity[Index(x - 1, y, z)]
						+ mVelocity[Index(x, y + 1, z)] + mVelocity[Index(x, y - 1, z)]
						+ mVelocity[Index(x, y, z + 1)] + mVelocity[Index(x, y, z - 1)]
						- divergence) / 6.0f;
					

				}
		//swap with buffer
		//mVelocity.swap(mVelocityBuffer);
		
		//Enforce boundary conditions on velocity
		setBoundariesVelocity(mVelocity);
	}
}

void CFDSimulation::advectQuantity(std::vector<float>& q, float dt){
	//for every voxel
	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < mHeight - 1; ++y)
			for (int z = 1; z < mDepth - 1; ++z)
			{
				//Predict where the current voxel of fluid was last time step 
				//by using (previous position) = (current position) - velocity * dt.
				glm::vec3 prevPosition = glm::vec3(x, y, z) - mVelocity[Index(x, y, z)] * dt;

				//Get a rounded version of this position
				glm::ivec3 iPrevPosition = glm::round(prevPosition);

				//get the 6 different x, y, and z positions we will need to use as indices for the 8 nieghbors of this position
				int x1 = iPrevPosition.x + 1;
				int y1 = iPrevPosition.y + 1;
				int z1 = iPrevPosition.z + 1;
				int x0 = iPrevPosition.x - 1;
				int y0 = iPrevPosition.y - 1;
				int z0 = iPrevPosition.z - 1;

				//Get the weights to be used for trilinear interpolation. 
				//positive direction(right, above, forward)
				float wx1 = x1 - prevPosition.x;
				float wy1 = y1 - prevPosition.y;
				float wz1 = z1 - prevPosition.z;

				//negative direction is just 1.0 - positive direction weight in this case
				float wx0 = 1.0f - wx1;
				float wy0 = 1.0f - wy1;
				float wz0 = 1.0f - wz1;

		
				//Do trilinear interpolation of the quantity
				q[Index(x, y, z)] = (wx1 * wy1 * wz1 * q[Index(x1, y1, z1)] + wx1 * wy1 * wz0 * q[Index(x1, y1, z0)] +
					wx1 * wy0 * wz1 * q[Index(x1, y0, z1)] + wx0 * wy1 * wz1 * q[Index(x0, y1, z1)] +
					wx1 * wy0 * wz0 * q[Index(x1, y0, z0)] + wx0 * wy1 * wz0 * q[Index(x0, y1, z0)] +
					wx0 * wy0 * wz1 * q[Index(x0, y0, z1)] + wx0 * wy0 * wz0 * q[Index(x0, y0, z0)]);
				
			}
}

void CFDSimulation::diffuseQuantity(std::vector<float>& q){
	for (int it = 20; it > 0; --it)
	{
		for (int x = 1; x < mWidth - 1; ++x)
			for (int y = 1; y < mHeight - 1; ++y)
				for (int z = 1; z < mDepth - 1; ++z)
				{
					//calculate the divergence of the velocity for this cell
					float divergence = ((mVelocity[Index(x + 1, y, z)].x - mVelocity[Index(x - 1, y, z)].x)
						+ (mVelocity[Index(x, y + 1, z)].y - mVelocity[Index(x, y - 1, z)].y)
						+ (mVelocity[Index(x, y, z + 1)].z - mVelocity[Index(x, y, z - 1)].z)) / 2.0f;
					//Calculate the updated value of the quantity and place it in the buffer.
					mBuffer[Index(x, y, z)] = (q[Index(x + 1, y, z)] + q[Index(x - 1, y, z)]
						+ q[Index(x, y + 1, z)] + q[Index(x, y - 1, z)]
						+ q[Index(x, y, z + 1)] + q[Index(x, y, z - 1)]
						- divergence) / 6.0f;

				}
		//swap with buffer
		q.swap(mBuffer);
	}
}




void CFDSimulation::project(){
	//calculate the divergence of velocity for every cell, store it in a buffer
	//We also set the pressure for each cell to 0, which is needed for the next step.
	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < mHeight - 1; ++y)
			for (int z = 1; z < mDepth - 1; ++z)
			{
				mBuffer[Index(x, y, z)] = ((mVelocity[Index(x + 1, y, z)].x - mVelocity[Index(x - 1, y, z)].x)
					+ (mVelocity[Index(x, y + 1, z)].y - mVelocity[Index(x, y - 1, z)].y)
					+ (mVelocity[Index(x, y, z + 1)].z - mVelocity[Index(x, y, z - 1)].z)) / 2.0f;
				mPressure[Index(x, y, z)] = 0.0f;
			}
	//Enforce boundary conditions on the divergence values in the buffer as well as the pressure
	setBoundariesPressure(mBuffer);
	setBoundariesPressure(mPressure);
	//Do jacobi iteration over the pressure field. Store output in second float buffer, swapping after each iteration.
	for (int it = 20; it > 0; --it)
	{
		for (int x = 1; x < mWidth - 1; ++x)
			for (int y = 1; y < mHeight - 1; ++y)
				for (int z = 1; z < mDepth - 1; ++z)
				{
					mPressure[Index(x, y, z)] = (mPressure[Index(x + 1, y, z)] + mPressure[Index(x - 1, y, z)]
						+ mPressure[Index(x, y + 1, z)] + mPressure[Index(x, y - 1, z)]
						+ mPressure[Index(x, y, z + 1)] + mPressure[Index(x, y, z - 1)]
						- mBuffer[Index(x, y, z)]) / 6.0f;
				}
		//mPressure.swap(mBuffer2);
		setBoundariesPressure(mPressure);
	}

	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < mHeight - 1; ++y)
			for (int z = 1; z < mDepth - 1; ++z)
			{
				//Get the gradient of the pressure
				glm::vec3 gradient = glm::vec3(mPressure[Index(x + 1, y, z)] - mPressure[Index(x - 1, y, z)],
					mPressure[Index(x, y + 1, z)] - mPressure[Index(x, y - 1, z)],
					mPressure[Index(x, y, z + 1)] - mPressure[Index(x, y, z - 1)]) / 2.0f;
				//Find the new velocity by subtracting the current velocity by the gradient of the pressure
				mVelocity[Index(x, y, z)] -= gradient;
			}
	//Set boundaries for velocity values
	setBoundariesVelocity(mVelocity);
}

void CFDSimulation::setBoundariesVelocity(std::vector<glm::vec3>& velocity){
	/*Cells on the boundary can be connected to either 0 or 1 non boundary cells. 
	We can ignore boundary cells of the former kind.
	For the others, we need to set the boundary cell velocity to the negation of the connected non-boundary cell's velocity.
	This results in a net velocity of 0 between the 2 cells.
	*/

	//Set left and right boundaries
	for (int y = 1; y < mHeight - 1; ++y)
		for (int z = 1; z < mDepth - 1; ++z)
		{
			//Left boundary
			velocity[Index(0, y, z)] = -velocity[Index(1, y, z)];
			//Right boundary
			velocity[Index(mWidth - 1, y, z)] = -velocity[Index(mWidth - 2, y, z)];
		}
	//Set top and bottom boundaries
	for (int x = 1; x < mWidth - 1; ++x)
		for (int z = 1; z < mDepth - 1; ++z)
		{
			//Bottom boundary
			velocity[Index(x, 0, z)] = -velocity[Index(x, 1, z)];
			//Top boundary
			velocity[Index(x, mHeight - 1, z)] = -velocity[Index(x, mHeight - 2, z)];
		}
	//Front and Back boundaries
	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < mHeight - 1; ++y)
		{
			//Back boundary
			velocity[Index(x, y, 0)] = -velocity[Index(x, y, 1)];
			//Front boundary
			velocity[Index(x, y, mDepth - 1)] = -velocity[Index(x, y, mDepth - 2)];
		}

	//Set corners
	velocity[Index(0, 0, 0)] = (velocity[Index(1, 0, 0)] + velocity[Index(0, 1, 0)] + velocity[Index(0, 0, 1)]) / 3.0f;
	velocity[Index(mWidth - 1, 0, 0)] = (velocity[Index(mWidth - 2, 0, 0)] + velocity[Index(mWidth - 1, 1, 0)] + velocity[Index(mWidth - 1, 0, 1)]) / 3.0f;
	velocity[Index(0, mHeight - 1, 0)] = (velocity[Index(1, mHeight - 1, 0)] + velocity[Index(0, mHeight - 2, 0)] + velocity[Index(0, mHeight - 1, 1)]) / 3.0f;
	velocity[Index(0, 0, mDepth - 1)] = (velocity[Index(1, 0, mDepth -1)] + velocity[Index(0, 1, mDepth - 1)] + velocity[Index(0, 0, mDepth - 2)]) / 3.0f;
	velocity[Index(mWidth - 1, mHeight - 1, 0)] = (velocity[Index(mWidth - 2, mHeight - 1, 0)] + velocity[Index(mWidth - 1, mHeight - 2, 0)] + velocity[Index(mWidth - 1, mHeight - 1, 1)]) / 3.0f;
	velocity[Index(mWidth - 1, 0, mDepth - 1)] = (velocity[Index(mWidth - 2, 0, mDepth - 1)] + velocity[Index(mWidth - 1, 1, mDepth - 1)] + velocity[Index(mWidth - 1, 0, mWidth - 2)]) / 3.0f;
	velocity[Index(0, mHeight - 1, mDepth - 1)] = (velocity[Index(1, mHeight - 1, mDepth - 1)] + velocity[Index(0, mHeight - 2, mDepth - 1)] + velocity[Index(0, mHeight - 1, mDepth - 2)]) / 3.0f;
	velocity[Index(mWidth - 1, mHeight - 1, mDepth - 1)] = (velocity[Index(mWidth - 2, mHeight - 1, mDepth - 1)] + velocity[Index(mWidth - 1, mHeight - 2, mDepth - 1)] + velocity[Index(mWidth - 1, mHeight - 1, mDepth - 2)]) / 3.0f;
}

void CFDSimulation::setBoundariesPressure(std::vector<float>& pressure){
	/*Cells on the boundary can be connected to either 0 or 1 non boundary cells.
	We can ignore boundary cells of the former kind.
	For the others, we need to set the boundary cell pressure to equal the connected non-boundary cell's pressure.
	*/

	//Set left and right boundaries
	for (int y = 1; y < mHeight - 1; ++y)
		for (int z = 1; z < mDepth - 1; ++z)
		{
			//Left boundary
			pressure[Index(0, y, z)] = pressure[Index(1, y, z)];
			//Right boundary
			pressure[Index(mWidth - 1, y, z)] = pressure[Index(mWidth - 2, y, z)];
		}
	//Set top and bottom boundaries
	for (int x = 1; x < mWidth - 1; ++x)
		for (int z = 1; z < mDepth - 1; ++z)
		{
			//Bottom boundary
			pressure[Index(x, 0, z)] = pressure[Index(x, 1, z)];
			//Top boundary
			pressure[Index(x, mHeight - 1, z)] = pressure[Index(x, mHeight - 2, z)];
		}
	//Front and Back boundaries
	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < mHeight - 1; ++y)
		{
			//Back boundary
			pressure[Index(x, y, 0)] = pressure[Index(x, y, 1)];
			//Front boundary
			pressure[Index(x, y, mDepth - 1)] = pressure[Index(x, y, mDepth - 2)];
		}

	pressure[Index(0, 0, 0)] = (pressure[Index(1, 0, 0)] + pressure[Index(0, 1, 0)] + pressure[Index(0, 0, 1)]) / 3.0f;
	pressure[Index(mWidth - 1, 0, 0)] = (pressure[Index(mWidth - 2, 0, 0)] + pressure[Index(mWidth - 1, 1, 0)] + pressure[Index(mWidth - 1, 0, 1)]) / 3.0f;
	pressure[Index(0, mHeight - 1, 0)] = (pressure[Index(1, mHeight - 1, 0)] + pressure[Index(0, mHeight - 2, 0)] + pressure[Index(0, mHeight - 1, 1)]) / 3.0f;
	pressure[Index(0, 0, mDepth - 1)] = (pressure[Index(1, 0, mDepth - 1)] + pressure[Index(0, 1, mDepth - 1)] + pressure[Index(0, 0, mDepth - 2)]) / 3.0f;
	pressure[Index(mWidth - 1, mHeight - 1, 0)] = (pressure[Index(mWidth - 2, mHeight - 1, 0)] + pressure[Index(mWidth - 1, mHeight - 2, 0)] + pressure[Index(mWidth - 1, mHeight - 1, 1)]) / 3.0f;
	pressure[Index(mWidth - 1, 0, mDepth - 1)] = (pressure[Index(mWidth - 2, 0, mDepth - 1)] + pressure[Index(mWidth - 1, 1, mDepth - 1)] + pressure[Index(mWidth - 1, 0, mWidth - 2)]) / 3.0f;
	pressure[Index(0, mHeight - 1, mDepth - 1)] = (pressure[Index(1, mHeight - 1, mDepth - 1)] + pressure[Index(0, mHeight - 2, mDepth - 1)] + pressure[Index(0, mHeight - 1, mDepth - 2)]) / 3.0f;
	pressure[Index(mWidth - 1, mHeight - 1, mDepth - 1)] = (pressure[Index(mWidth - 2, mHeight - 1, mDepth - 1)] + pressure[Index(mWidth - 1, mHeight - 2, mDepth - 1)] + pressure[Index(mWidth - 1, mHeight - 1, mDepth - 2)]) / 3.0f;
}

void CFDSimulation::determineFluidCells() {
	mFluid.assign(mFluid.size(), false);
	for (auto &p : mMarkerParticles)
	{
		mFluid[Index(p.x, p.y, p.z)] = true;

	}
}

void CFDSimulation::applyForces(float dt) {
	for (int x = 1; x < mWidth - 1; ++x)
		for (int y = 1; y < mHeight; ++y)
			for (int z = 1; z < mDepth - 1; ++z)
				if(mFluid[Index(x, y, z)]) 
					mVelocity[Index(x, y, z)].y += dt * -9.8f;
}

