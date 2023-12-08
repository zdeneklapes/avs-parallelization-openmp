/**
 * @file    loop_mesh_builder.cpp
 *
 * @author  Zdenek Lapes <xlapes02@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP loops
 *
 * @date    2023-12-08
 **/

#include <iostream>
#include <math.h>
#include <limits>

#include "loop_mesh_builder.h"

#define SCHEDULE_TYPE dynamic
#define CHUNK_SIZE 32

LoopMeshBuilder::LoopMeshBuilder(
        unsigned gridEdgeSize
) : BaseMeshBuilder(gridEdgeSize, "OpenMP Loop") {

}

unsigned LoopMeshBuilder::marchCubes(const ParametricScalarField &field) {
    // 1. Compute total number of cubes in the grid.
    size_t totalCubesCount = mGridSize * mGridSize * mGridSize;

    unsigned totalTriangles = 0;

    // 2. Loop over each coordinate in the 3D grid.
#ifndef DEBUG
#pragma omp parallel default(none) reduction(+:totalTriangles) shared(totalCubesCount, field)
    {
#pragma omp for schedule(SCHEDULE_TYPE, CHUNK_SIZE)
#endif
        for (size_t i = 0; i < totalCubesCount; ++i) {
            // 3. Compute 3D position in the grid.
            Vec3_t<float> cubeOffset(
                    i % mGridSize, // x
                    (i / mGridSize) % mGridSize, // y
                    i / (mGridSize * mGridSize) // z
            );

            // 4. Evaluate "Marching Cube" at given position in the grid and
            //    store the number of triangles generated.
            totalTriangles += buildCube(cubeOffset, field);
        }
#ifndef DEBUG
    }
#endif
    // 5. Return total number of triangles generated.
    return totalTriangles;
}

float LoopMeshBuilder::evaluateFieldAt(const Vec3_t<float> &pos,
                                       const ParametricScalarField &field) {
    // NOTE: This method is called from "buildCube(...)"!

    // 1. Store pointer to and number of 3D points in the field
    //    (to avoid "data()" and "size()" call in the loop).
    const Vec3_t<float> *pPoints = field.getPoints().data();
    const unsigned count = unsigned(field.getPoints().size());

    float value = std::numeric_limits<float>::max();

    // 2. Find minimum square distance from points "pos" to any point in the
    //    field.
#ifndef DEBUG
//#pragma omp parallel default(none) shared(pPoints, count, pos) reduction(min:value)
    {
//#pragma omp for schedule(SCHEDULE_TYPE, CHUNK_SIZE)
#endif
        for (unsigned i = 0; i < count; ++i) {
            float distanceSquared = (pos.x - pPoints[i].x) * (pos.x - pPoints[i].x);
            distanceSquared += (pos.y - pPoints[i].y) * (pos.y - pPoints[i].y);
            distanceSquared += (pos.z - pPoints[i].z) * (pos.z - pPoints[i].z);

            // Comparing squares instead of real distance to avoid unnecessary
            // "sqrt"s in the loop.
            value = std::min(value, distanceSquared);
        }
#ifndef DEBUG
    }
#endif

    // 3. Finally take square root of the minimal square distance to get the real distance
    return sqrt(value);
}

void LoopMeshBuilder::emitTriangle(const BaseMeshBuilder::Triangle_t &triangle) {
    // NOTE: This method is called from "buildCube(...)"!

    // Store generated triangle into vector (array) of generated triangles.
    // The pointer to data in this array is return by "getTrianglesArray(...)" call
    // after "marchCubes(...)" call ends.

#ifndef DEBUG
#pragma omp critical(triangle)
#endif
    {
        mTriangles.push_back(triangle);
    }
}
