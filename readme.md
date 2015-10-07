# FluidSimulation

Simple fluid simulation using SDL and OpenGL. 
Only setup to run when compiled as a 64 bit exe right now. 

Currently just sets up the data structures for the simulation with some initial values, and renders it using marching cubes. 
Can't actually interact with the fluid yet. 

The techniques used for the GPU implementation of marching cubes was described in
GPU Gems 3 chapter 1 "Generating Complex Procedural Terrains Using the GPU" by Ryan Geiss.

CPU implemention of marching cubes (which is no longer used, but still contained in MarchingCubes.h and Marching Cubes.cpp)
uses the implementation from Paul Bourke's Polygonising a scalar field, modified to support generating normals. 

![alt tag](https://raw.github.com/Gorgexpress/FluidSimulation/master/images/cubeofwater.png)