# FluidSimulation

Simple fluid simulation using SDL and OpenGL. VS2013 project.
Only setup to run when compiled as a 64 bit exe right now, in debug mode.

Currently just sets up the data structures for the simulation with some initial values, and renders it using marching cubes.
Updating the simulation is disabled.
The simulation is a very basic one using semi-lagrangian advection and does not interact with solids. A more
complex one is needed, as it is hard to see changes in the fluid with the current simulation.

The techniques used for the GPU implementation of marching cubes were described in
GPU Gems 3 chapter 1 "Generating Complex Procedural Terrains Using the GPU" by Ryan Geiss.

CPU implemention of marching cubes (which is no longer used, but still contained in MarchingCubes.h and Marching Cubes.cpp)
uses the implementation from Paul Bourke's Polygonising a scalar field, modified to support generating normals. 

![alt tag](https://raw.github.com/Gorgexpress/FluidSimulation/master/images/cubeofwater.png)
