#!/bin/bash
## -V: create SPIR-V binary
## -x: save binary output as text-based 32-bit hexadecimal numbers
## -o: output file
glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert

read -p "Press key to continue.. " -n1 -s