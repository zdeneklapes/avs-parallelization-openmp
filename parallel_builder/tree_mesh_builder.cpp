/**
 * @file    tree_mesh_builder.cpp
 *
 * @author  Zdenek Lapes <xlapes02@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP tasks + octree early elimination
 *
 * @date    2023-12-08
 **/

#include <iostream>
#include <cmath>
#include <limits>

#include "tree_mesh_builder.h"

TreeMeshBuilder::TreeMeshBuilder(unsigned gridEdgeSize)
        : BaseMeshBuilder(gridEdgeSize, "Octree") {

}

unsigned TreeMeshBuilder::marchCubes(const ParametricScalarField &field) {
    // Suggested approach to tackle this problem is to add new method to
    // this class. This method will call itself to process the children.
    // It is also strongly suggested to first implement Octree as sequential
    // code and only when that works add OpenMP tasks to achieve parallelism.
    unsigned int totalTriangles = 0;
#ifndef DEBUG
#pragma omp parallel default(none) shared(totalTriangles, field)
//#pragma omp parallel default(none) shared(totalTriangles, field) num_threads(8)
    {
#pragma omp master
//#pragma omp single nowait
        {
#endif
            totalTriangles = decomposeCube(Vec3_t<float>(), mGridSize, field);
#ifndef DEBUG
        }
    }
#endif


    return totalTriangles;
}

float TreeMeshBuilder::evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field) {
    // NOTE: This method is called from "buildCube(...)"!

    // 1. Store pointer to and number of 3D points in the field
    //    (to avoid "data()" and "size()" call in the loop).
    const Vec3_t<float> *pPoints = field.getPoints().data();
    const auto count = unsigned(field.getPoints().size());

    float value = std::numeric_limits<float>::max();

    // 2. Find minimum square distance from points "pos" to any point in the
    //    field.
    for (unsigned i = 0; i < count; ++i) {
        float distanceSquared = (pos.x - pPoints[i].x) * (pos.x - pPoints[i].x);
        distanceSquared += (pos.y - pPoints[i].y) * (pos.y - pPoints[i].y);
        distanceSquared += (pos.z - pPoints[i].z) * (pos.z - pPoints[i].z);

        // Comparing squares instead of real distance to avoid unnecessary
        // "sqrt"s in the loop.
        value = std::min(value, distanceSquared);
    }

    // 3. Finally take square root of the minimal square distance to get the real distance
    return sqrt(value);
}

void TreeMeshBuilder::emitTriangle(const BaseMeshBuilder::Triangle_t &triangle) {
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


auto TreeMeshBuilder::decomposeCube(const Vec3_t<float> &cubeOffset,
                                    const unsigned int gridSize,
                                    const ParametricScalarField &field) -> unsigned int {
    const auto edgeLen = float(gridSize);

    if (isSurfaceInBlock(edgeLen, cubeOffset, field)) { return 0; }
    if (gridSize <= GRID_CUT_OFF) { return buildCube(cubeOffset, field); }

    unsigned int totalTriangles = 0;
    const unsigned int nextGridSize = gridSize / 2;

//#ifndef DEBUG
//#pragma omp for
//#endif
//#ifndef DEBUG
//#pragma omp task default(none) shared(cubeOffset, nextGridSize, field, totalTriangles)
//#endif
//    for (unsigned int i = 0; i < 8; ++i) {
    for (unsigned int i = 0; i < 8; ++i) {
#ifndef DEBUG
#pragma omp task default(none) firstprivate(i) shared(cubeOffset, nextGridSize, field, totalTriangles)
#endif
        {
            const Vec3_t<float> nextCubeOffset(
                    cubeOffset.x + sc_vertexNormPos[i].x * float(nextGridSize),
                    cubeOffset.y + sc_vertexNormPos[i].y * float(nextGridSize),
                    cubeOffset.z + sc_vertexNormPos[i].z * float(nextGridSize)
            );
            const unsigned int trianglesCount = decomposeCube(nextCubeOffset, nextGridSize, field);
#ifndef DEBUG
#pragma omp atomic update
#endif
            totalTriangles += trianglesCount;
        }
    }

#ifndef DEBUG
#pragma omp taskwait
#endif
    return totalTriangles;
}

auto TreeMeshBuilder::isSurfaceInBlock(
        const float edgeLength,
        const Vec3_t<float> &cubeOffset,
        const ParametricScalarField &field
) -> bool {
    const float resEdgeLength = edgeLength * mGridResolution;
    const float halfEdgeLength = resEdgeLength / 2.F;
    const Vec3_t<float> midPoint(
            cubeOffset.x * mGridResolution + halfEdgeLength,
            cubeOffset.y * mGridResolution + halfEdgeLength,
            cubeOffset.z * mGridResolution + halfEdgeLength
    );
    auto block = evaluateFieldAt(midPoint, field);
    auto circle = mIsoLevel + sqrtf(3.F) / 2.F * resEdgeLength;
    bool isBlockEmpty = block > circle;
    return isBlockEmpty;
}


