#ifndef JOLT_H
#define JOLT_H

#include <cglm/cglm.h>

#include "numtypes.h"

void joltInit();
void joltUpdate(f32 deltaTime, mat4 m);
void joltFireCube(vec3 pos, vec3 dir);
void joltQuit();

#endif