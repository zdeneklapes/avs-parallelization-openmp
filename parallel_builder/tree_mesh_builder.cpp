/**
 * @file    tree_mesh_builder.cpp
 *
 * @author  FULL NAME <xlogin00@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP tasks + octree early elimination
 *
 * @date    DATE
 **/

#include <iostream>
#include <math.h>
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

    unsigned trianglesCount = 0;
#ifndef DEBUG
#pragma omp parallel default(none) shared(trianglesCount, field)
#pragma omp single nowait
#endif
    trianglesCount = decomposeSpace(mGridSize, Vec3_t<float>(), field);

    return trianglesCount;
}

auto TreeMeshBuilder::decomposeSpace(
        const unsigned gridSize,
        const Vec3_t<float> &cubeOffset,
        const ParametricScalarField &field
) -> unsigned {
    const auto edgeLength = float(gridSize);
    if (isBlockEmpty(edgeLength, cubeOffset, field)) {
        return 0;
    }

    if (gridSize <= GRID_CUT_OFF) {
        return buildCube(cubeOffset, field);
    }

    unsigned totalTrianglesCount = 0;
    const unsigned newGridSize = gridSize / 2;
    const auto newEdgeLength = float(newGridSize);

    for (const Vec3_t<float> vertexNormPos: sc_vertexNormPos) {
#ifndef DEBUG
#pragma omp task default(none) firstprivate(vertexNormPos) shared(cubeOffset, newEdgeLength, newGridSize, field, totalTrianglesCount)
#endif
        {
            const Vec3_t<float> newCubeOffset(
                    cubeOffset.x + vertexNormPos.x * newEdgeLength,
                    cubeOffset.y + vertexNormPos.y * newEdgeLength,
                    cubeOffset.z + vertexNormPos.z * newEdgeLength
            );
            const unsigned trianglesCount =
                    decomposeSpace(newGridSize, newCubeOffset, field);

#ifndef DEBUG
#pragma omp atomic update
#endif
            totalTrianglesCount += trianglesCount;
        }
    }

#ifdef DEBUG
#pragma omp taskwait
#endif
    return totalTrianglesCount;
}

auto TreeMeshBuilder::isBlockEmpty(
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
    static const float expr = sqrtf(3.F) / 2.F;

    return evaluateFieldAt(midPoint, field) > mIsoLevel + expr * resEdgeLength;
}


float TreeMeshBuilder::evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field) {
    float minDistanceSquared = std::numeric_limits<float>::max();

    for (const Vec3_t<float> point: field.getPoints()) {
        const float distanceSquared = (pos.x - point.x) * (pos.x - point.x)
                                      + (pos.y - point.y) * (pos.y - point.y)
                                      + (pos.z - point.z) * (pos.z - point.z);
        minDistanceSquared = std::min(minDistanceSquared, distanceSquared);
    }

    return sqrtf(minDistanceSquared);
}

void TreeMeshBuilder::emitTriangle(const BaseMeshBuilder::Triangle_t &triangle) {
#ifndef DEBUG
#pragma omp critical(tree_emitTriangle)
#endif
    triangles.push_back(triangle);
}
