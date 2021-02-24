#pragma once

#include <fbxsdk.h>

FbxAMatrix GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, const FbxPose* pPose = nullptr, const FbxAMatrix* pParentGlobalPosition = nullptr);

void ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxVector4* pVertexArray);

void ComputeLinearDeformation(FbxAMatrix& pGlobalPosition,
    FbxMesh* pMesh,
    FbxTime& pTime,
    FbxVector4* pVertexArray,
    FbxPose* pPose);

void ComputeClusterDeformation(FbxAMatrix& pGlobalPosition,
    const FbxMesh* pMesh,
    FbxCluster* pCluster,
    FbxAMatrix& pVertexTransformMatrix,
    FbxTime pTime,
    const FbxPose* pPose);

// Scale all the elements of a matrix.
inline void MatrixScale(FbxAMatrix& pMatrix, double pValue)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            pMatrix[i][j] *= pValue;
        }
    }
}


// Add a value to all the elements in the diagonal of the matrix.
inline void MatrixAddToDiagonal(FbxAMatrix& pMatrix, double pValue)
{
    pMatrix[0][0] += pValue;
    pMatrix[1][1] += pValue;
    pMatrix[2][2] += pValue;
    pMatrix[3][3] += pValue;
}


// Sum two matrices element by element.
inline void MatrixAdd(FbxAMatrix& pDstMatrix, const FbxAMatrix& pSrcMatrix)
{
    int i, j;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            pDstMatrix[i][j] += pSrcMatrix[i][j];
        }
    }
}

// Get the matrix of the given pose
inline FbxAMatrix GetPoseMatrix(const FbxPose* pPose, int pNodeIndex)
{
    FbxAMatrix lPoseMatrix;
    FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

    memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

    return lPoseMatrix;
}

// Get the geometry offset to a node. It is never inherited by the children.
inline FbxAMatrix GetGeometry(const FbxNode* pNode)
{
    const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
    const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

    return FbxAMatrix(lT, lR, lS);
}