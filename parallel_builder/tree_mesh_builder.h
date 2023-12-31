/**
 * @file    tree_mesh_builder.h
 *
 * @author  Zdenek Lapes <xlapes02@stud.fit.vutbr.cz>
 *
 * @brief   Parallel Marching Cubes implementation using OpenMP tasks + octree early elimination
 *
 * @date    2023-12-08
 **/

#ifndef TREE_MESH_BUILDER_H
#define TREE_MESH_BUILDER_H

#include "base_mesh_builder.h"

class TreeMeshBuilder : public BaseMeshBuilder {
public:
    TreeMeshBuilder(unsigned gridEdgeSize);

protected:
    unsigned marchCubes(const ParametricScalarField &field);

    float evaluateFieldAt(const Vec3_t<float> &pos, const ParametricScalarField &field);

    void emitTriangle(const Triangle_t &triangle);

    const Triangle_t *getTrianglesArray() const { return mTriangles.data(); }

    auto decomposeCube(const Vec3_t<float> &cubeOffset, unsigned int gridSize,
                       const ParametricScalarField &field) -> unsigned int;

    auto isSurfaceInBlock(
            const float edgeLength,
            const Vec3_t<float> &cubeOffset,
            const ParametricScalarField &field
    ) -> bool;


    static const unsigned GRID_CUT_OFF = 1;
    std::vector<Triangle_t> mTriangles; ///< Temporary array of triangles
};

#endif // TREE_MESH_BUILDER_H
