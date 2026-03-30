/* 
SPDX-FileCopyrightText: 2025 Caleb Dawson
SPDX-License-Identifier: Apache-2.0
*/

#include <cluster_tree_2d.h>

typedef int32_t I32;
typedef uint32_t U32;
typedef int8_t I8;
typedef float F32;

PixErr clutreMeshValidate(const ClutreMesh *pMesh) {
	PixErr err = PIX_ERR_SUCCESS;
	PIX_ERR_RETURN_IFNOT_COND(err, pMesh->faceCount > 0, "");
	PIX_ERR_RETURN_IFNOT_COND(
		err,
		pMesh->fpFaceRange && pMesh->fpVert && pMesh->fpPos,
		""
	);
	return err;
}

static
PixtyV2_F32 bbSizeGet(const ClutreBb *pBb) {
	return _(pBb->max V2SUB pBb->min);
}

bool clutreBbValidate(const ClutreBb *pBb) {
	PixtyV2_F32 bbSize = bbSizeGet(pBb);
	return bbSize.d[0] > .0f && bbSize.d[1] > .0f; 
}

static
float radicalInvVdc(U32 i) {
	i = (i << 16u) | (i >> 16u);
	i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
	i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
	i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
	i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
	return (float)i * 2.3283064365386963e-10f;
}

// *see below cit for above func aswell
//Hammersley set, see https://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// *hemisphere part is unused here
static
PixtyV2_F32 hammersley2d(U32 i, U32 num) {
	return (PixtyV2_F32){.d = {(float)i / (float)num, radicalInvVdc(i)}};
}

void clutreNoiseGen(const PixalcFPtrs *pAlloc, ClutreNoise *pNoise) {
	I32 posLin = 0;
	for (U32 i = 0; i < CLUTRE_POINT_COUNT; ++i) {
#ifdef CLUTRE_PATTERN_GRID
		PixtyV2_I32 iPos = {.d = {
			posLin % CLUTRE_POINT_RES,
			posLin / CLUTRE_POINT_RES
		}};
		pNoise->points[i].pos = (PixtyV2_F32){.d = {
			(F32)(posLin % CLUTRE_POINT_RES) / (F32)CLUTRE_POINT_RES,
			(F32)(posLin / CLUTRE_POINT_RES) / (F32)CLUTRE_POINT_RES
		}};
		_(&pNoise->points[i].pos V2ADDEQLS 1.0f / CLUTRE_POINT_RES / 2.0f);
		++posLin;
#else
		pNoise->points[i].pos = hammersley2d(i, CLUTRE_POINT_COUNT);
#endif
	}
	pNoise->pDf = pAlloc->fpMalloc(sizeof(ClutreDfPoint) * CLUTRE_DF_RES * CLUTRE_DF_RES);
	for (I32 i = 0; i < CLUTRE_DF_RES; ++i) {
		for (I32 j = 0; j < CLUTRE_DF_RES; ++j) {
			I32 idx = CLUTRE_DF_RES * i + j;
			PixtyV2_F32 pos = (PixtyV2_F32){.d = {(F32)j, (F32)i}};
			_(&pos V2DIVSEQL (F32)CLUTRE_DF_RES);
			F32 nearest = FLT_MAX;
			pNoise->pDf[idx] = (ClutreDfPoint){0};
			for (U32 k = 0; k < CLUTRE_POINT_COUNT; ++k) {
				pNoise->pDf[idx].layers[k] =
					pixmV2F32Len(_(pos V2SUB pNoise->points[k].pos));
				if (pNoise->pDf[idx].layers[k] < nearest) {
					pNoise->pDf[idx].nearest = k;
					nearest = pNoise->pDf[idx].layers[k];
				}
			}
		}
	}
}

const ClutreDfPoint *clutreNoiseSample(
	const ClutreNoise *pNoise,
	PixtyV2_F32 pos,
	ClutreBb bb
) {
	PixtyV2_F32 bbSize = bbSizeGet(&bb);
	PixtyV2_F32 posNorm = _(_(pos V2SUB bb.min) V2DIV bbSize);
	_(&posNorm V2MULSEQL (F32)CLUTRE_DF_RES);
	I32 dfAxisMax = CLUTRE_DF_RES - 1; 
	PixtyV2_I32 iPos = {.d = {
		(I32)posNorm.d[0] >= dfAxisMax ?
			dfAxisMax : posNorm.d[0] < .0f ? 0 : (I32)roundf(posNorm.d[0]),
		(I32)posNorm.d[1] >= dfAxisMax ?
			dfAxisMax : posNorm.d[1] < .0f ? 0 : (I32)roundf(posNorm.d[1])
	}};
	I32 idx = CLUTRE_DF_RES * iPos.d[1] + iPos.d[0];
	return pNoise->pDf + idx;
}

ClutreBb clutreBbScale(ClutreBb bb, F32 scale) {
	//PixtyV2_F32 centre = _(_(bb.min V2ADD bb.max) V2DIVS 2.0f);
	return (ClutreBb){
		.min = bb.min,
		.max = _(_(_(bb.max V2SUB bb.min) V2MULS scale) V2ADD bb.min)
	};
}

void clutreContribFaceToBb(ClutreBb *pBb, const ClutreBb *pFaceBb) {
	pBb->min.d[0] = pFaceBb->min.d[0] < pBb->min.d[0] ? pFaceBb->min.d[0] : pBb->min.d[0];
	pBb->min.d[1] = pFaceBb->min.d[1] < pBb->min.d[1] ? pFaceBb->min.d[1] : pBb->min.d[1];
	pBb->max.d[0] = pFaceBb->max.d[0] > pBb->max.d[0] ? pFaceBb->max.d[0] : pBb->max.d[0];
	pBb->max.d[1] = pFaceBb->max.d[1] > pBb->max.d[1] ? pFaceBb->max.d[1] : pBb->max.d[1];
}

PixErr clutreInitChildren(
	ClutreTree *pTree,
	ClutreNode *pCluster,
	I32 pointCount,
	ClutreNode **ppChildRedir,
	I8 *pClusterBuf,
	PixtyI32Arr *pFaceBuf,
	ClutreBb *pBbBuf
) {
	PixErr err = PIX_ERR_SUCCESS;
	I32 allocIdx = pixalcLinAlloc(&pTree->nodeAlloc, &pCluster->pChildren, pointCount);
	for (I32 i = 0; i < CLUTRE_POINT_COUNT; ++i) {
		pFaceBuf[i].count = 0;
	}
	for (I32 i = pCluster->faces.start; i < pCluster->faces.end; ++i) {
		I32 point = pClusterBuf[i];
		{
			I32 newIdx = 0;
			PIXALC_DYN_ARR_ADD(I32, &pTree->alloc, pFaceBuf + point, newIdx);
			pFaceBuf[point].pArr[newIdx] = pTree->pFaces[i];
		}
		if (!ppChildRedir[point]) {
			PIX_ERR_ASSERT("", pCluster->childCount < pointCount);
			ppChildRedir[point] = pCluster->pChildren + pCluster->childCount;
			PixtyV2_F32 bbSize = bbSizeGet(pBbBuf + point);
			if (bbSize.d[0] == .0f || bbSize.d[1] == .0f) {
				bool side = bbSize.d[0] > .0f;
				PIX_ERR_ASSERT("", side || bbSize.d[1] > .0f);
				F32 offset = FLT_EPSILON * 2.0f;
				pBbBuf[point].min.d[side] -= offset;
				pBbBuf[point].max.d[side] += offset;
			}
			*ppChildRedir[point] = (ClutreNode){
				.bb = pBbBuf[point],
				.point = point,
				.idx = allocIdx + pCluster->childCount
			};
			++pCluster->childCount;
		}
	}
	return err;
}

PixErr clutreReorderFaces(ClutreTree *pTree, ClutreNode *pCluster, PixtyI32Arr *pFaceBuf) {
	PixErr err = PIX_ERR_SUCCESS;
	I32 offset = pCluster->faces.start;
	for (I32 i = 0; i < pCluster->childCount; ++i) {
		ClutreNode *pChild = pCluster->pChildren + i;
		if (pFaceBuf[pChild->point].count) {
			pChild->faces = (PixtyRange){
				.start = offset,
				.end = offset + pFaceBuf[pChild->point].count
			};
			memcpy(
				pTree->pFaces + offset,
				pFaceBuf[pChild->point].pArr,
				sizeof(I32) * pFaceBuf[pChild->point].count
			);
			offset += pFaceBuf[pChild->point].count;
		}
	}
	PIX_ERR_ASSERT("", offset == pCluster->faces.end);
	return err;
}

void clutreTreeMemInit(const PixalcFPtrs *pAlloc, ClutreTree *pTree, const ClutreMesh *pMesh) {
	*pTree = (ClutreTree){.alloc = *pAlloc};
	pixalcLinAllocInit(pAlloc, &pTree->nodeAlloc, sizeof(ClutreNode), 5, false);
	pixalcLinAlloc(&pTree->nodeAlloc, &pTree->pRoot, 1);
	pTree->pRoot->faces = (PixtyRange){.start = 0, .end = pMesh->faceCount};
	pTree->pRoot->bb = (ClutreBb){.min = {.d = {.0f, .0f}}, .max = {.d = {1.0f, 1.0f}}};
	pTree->pFaces = pAlloc->fpMalloc(sizeof(I32) * pMesh->faceCount);
	for (I32 i = 0; i < pMesh->faceCount; ++i) {
		pTree->pFaces[i] = i;
	}
}

void clutreBuildCleanup(
	const ClutreTree *pTree,
	ClutreNoise *pNoise,
	I8 *pClusterBuf,
	PixtyI32Arr *pFaceBuf
) {
	const PixalcFPtrs *pAlloc = &pTree->alloc;
	if (pNoise->pDf) {
		pAlloc->fpFree(pNoise->pDf);
	}
	pAlloc->fpFree(pClusterBuf);
	for (I32 i = 0; i < CLUTRE_POINT_COUNT; ++i) {
		if (pFaceBuf[i].pArr) {
			pAlloc->fpFree(pFaceBuf[i].pArr);
		}
	}
}
