/**
 * @file    tree_mesh_builder.h
 *
 * @author  FULL NAME <xlogin00@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP tasks + octree early elimination
 *
 * @date    DATE
 **/

#ifndef TREE_MESH_BUILDER_H
#define TREE_MESH_BUILDER_H

#include "base_mesh_builder.h"

class TreeMeshBuilder : public BaseMeshBuilder {
public:
    TreeMeshBuilder(unsigned gridEdgeSize);

protected:
    unsigned marchCubes(const ParametricScalarField &field);

    auto decomposeSpace(
            unsigned gridSize,
            const Vec3_t<float> &cubeOffset,
            const ParametricScalarField &field
    ) -> unsigned;

    auto isBlockEmpty(
            float edgeLength,
            const Vec3_t<float> &cubeOffset,
            const ParametricScalarField &field
    ) -> bool;

    float evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field);

    void emitTriangle(const Triangle_t &triangle);

    const Triangle_t *getTrianglesArray() const { return nullptr; }

    static const unsigned GRID_CUT_OFF = 1;
    std::vector<Triangle_t> triangles;
};

#endif // TREE_MESH_BUILDER_H
