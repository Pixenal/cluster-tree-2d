/* 
SPDX-FileCopyrightText: 2025 Caleb Dawson
SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <float.h>

#include <pixenals_alloc_utils.h>
#include <pixenals_math_utils.h>

#ifdef WIN32
#define CLUTRE_FORCE_INLINE __forceinline
#else
#define CLUTRE_FORCE_INLINE __attribute__((always_inline)) static inline
#endif

#define CLUTRE_PATTERN_GRID

#ifdef CLUTRE_PATTERN_GRID
#define CLUTRE_POINT_RES 2
#define CLUTRE_POINT_COUNT (CLUTRE_POINT_RES * CLUTRE_POINT_RES)
#define CLUTRE_NOISE_SCALE_MAX CLUTRE_POINT_RES
#else
#define CLUTRE_POINT_COUNT 9
#define CLUTRE_NOISE_SCALE_MAX 4
#endif

#define CLUTRE_DF_RES 64
#define CLUTRE_STACK_SIZE 16
#define CLUTRE_FACE_COUNT_MIN 32
#define CLUTRE_VALID_MIN (1.0f / 3.0f)

typedef struct ClutreBb {
	PixtyV2_F32 min;
	PixtyV2_F32 max;
} ClutreBb;

typedef struct ClutreIBb {
	PixtyV2_I32 min;
	PixtyV2_I32 max;
} ClutreIBb;

typedef struct ClutreClutreFaceCorner {
	int32_t face;
	int32_t corner;
} ClutreClutreFaceCorner;

typedef struct ClutreBorder {
	ClutreClutreFaceCorner *pArr;
	int32_t size;
	int32_t count;
} ClutreBorder;

typedef struct ClutreNode {
	struct ClutreNode *pChildren;
	ClutreBorder border;
	PixtyRange faces;
	int32_t childCount;
	ClutreBb bb;
	int32_t point;
	int32_t idx;
} ClutreNode;

typedef struct ClutreTree {
	ClutreNode *pRoot;
	PixalcFPtrs alloc;
	int32_t *pFaces;
	PixalcLinAlloc nodeAlloc;
	int32_t faceCount;
	bool valid;
} ClutreTree;

typedef struct ClutreMesh {
	void *pUserData;
	PixtyRange (*fpFaceRange)(const void *, int32_t);
	int32_t (*fpVert)(const void *, int32_t);
	PixtyV2_F32 (*fpPos)(const void *, int32_t);
	int32_t faceCount;
} ClutreMesh;

typedef enum ClutreIntersect {
	CLUTRE_NONE,
	CLUTRE_INTERSECT,
	CLUTRE_ENCLOSED,
	CLUTRE_ENCLOSING,
	CLUTRE_NO_INTERSECT
} ClutreIntersect;

typedef struct ClutreArr {
	void *pUserData;
	PixErr (*fpAdd)(const PixalcFPtrs *, void *, int32_t, ClutreIntersect, PixtyV2_I32);
} ClutreArr;

typedef struct ClutreFace {
	void *pUserData;
	PixtyV2_F32 (*fpPos)(struct ClutreSample *, int32_t);
	int32_t size;
} ClutreFace;

typedef struct ClutreNoisePoint {
	PixtyV2_F32 pos;
} ClutreNoisePoint;

typedef struct ClutreDfPoint {
	float layers[CLUTRE_POINT_COUNT];
	int32_t nearest;
} ClutreDfPoint;

typedef struct ClutreNoise {
	ClutreNoisePoint points[CLUTRE_POINT_COUNT];
	ClutreDfPoint *pDf; 
} ClutreNoise;

typedef struct ClutreStackEntry {
	ClutreNode *pNode;
	int32_t nextChild;
} ClutreStackEntry;

typedef struct ClutreStack {
	ClutreStackEntry stack[CLUTRE_STACK_SIZE];
	int32_t ptr;
} ClutreStack;

typedef struct ClutreLoopFunc {
	PixErr (*func)(ClutreStack *pStack, void *, bool *);
	void *pArgs;
} ClutreLoopFunc;

typedef struct ClutreFaceRange {
	const int32_t *pArr;
	int32_t size;
} ClutreFaceRange;

#ifdef CLUTRE_DEBUG_VIS
typedef struct ClutreImg {
	float *pData;
	int32_t width;
	int32_t height;
} ClutreImg;
#endif

typedef struct ClutreBuildLoopArgs {
	ClutreTree *pTree;
	const ClutreMesh *pMesh;
	const ClutreNoise *pNoise;
	int8_t *pClusterBuf;
	PixtyI32Arr *pFaceBuf;
#ifdef CLUTRE_DEBUG_VIS
	ClutreImg *pImgArr;
#endif
} ClutreBuildLoopArgs;

typedef struct ClutreSampleLoopArgs {
	const ClutreTree *pTree;
	ClutreArr *pClutreArr;
	const PixtyV2_F32 *pPos;
	const ClutreBb *pFaceBb;
	PixtyV2_I32 tile;
	int32_t faceSize;
	bool enclosed;
	bool added;
#ifdef CLUTRE_DEBUG_VIS
	const ClutreMesh *pMesh;
	ClutreImg *pImg;
#endif
} ClutreSampleLoopArgs;

#ifdef CLUTRE_DEBUG_VIS
void clutreDumpSampleImg(
	const ClutreTree *pTree,
	const ClutreMesh *pMesh,
	ClutreImg *pImg,
	const ClutreBb *pFaceBb,
	const ClutreNode *pCluster,
	PixtyV2_I32 tile
);
#endif
PixErr clutreMeshValidate(const ClutreMesh *pMesh);
bool clutreBbValidate(const ClutreBb *pBb);
void clutreNoiseGen(const PixalcFPtrs *pAlloc, ClutreNoise *pNoise);
const ClutreDfPoint *clutreNoiseSample(
	const ClutreNoise *pNoise,
	PixtyV2_F32 pos,
	ClutreBb bb
);
ClutreBb clutreBbScale(ClutreBb bb, float scale);
void clutreContribFaceToBb(ClutreBb *pBb, const ClutreBb *pFaceBb);
PixErr clutreInitChildren(
	ClutreTree *pTree,
	ClutreNode *pCluster,
	int32_t pointCount,
	ClutreNode **ppChildRedir,
	int8_t *pClusterBuf,
	PixtyI32Arr *pFaceBuf,
	ClutreBb *pBbBuf
);
PixErr clutreReorderFaces(ClutreTree *pTree, ClutreNode *pCluster, PixtyI32Arr *pFaceBuf);
void clutreTreeMemInit(const PixalcFPtrs *pAlloc, ClutreTree *pTree, const ClutreMesh *pMesh);
void clutreBuildCleanup(
	const ClutreTree *pTree,
	ClutreNoise *pNoise,
	int8_t *pClusterBuf,
	PixtyI32Arr *pFaceBuf
);

CLUTRE_FORCE_INLINE
PixErr clutreSampleAdd(
	const PixalcFPtrs *pAlloc,
	ClutreArr *pArr,
	int32_t idx,
	ClutreIntersect status,
	PixtyV2_I32 tile
) {
	PixErr err = PIX_ERR_SUCCESS;
	PIX_ERR_ASSERT("", status > CLUTRE_NONE && status < CLUTRE_NO_INTERSECT);
	err = pArr->fpAdd(pAlloc, pArr->pUserData, idx, status, tile);
	PIX_ERR_RETURN_IFNOT(err, "");
	return err;
}

static inline
void clutreBbCmp(ClutreBb *pBb, PixtyV2_F32 pos) {
	pBb->min.d[0] = pos.d[0] < pBb->min.d[0] ? pos.d[0] : pBb->min.d[0];
	pBb->min.d[1] = pos.d[1] < pBb->min.d[1] ? pos.d[1] : pBb->min.d[1];
	pBb->max.d[0] = pos.d[0] > pBb->max.d[0] ? pos.d[0] : pBb->max.d[0];
	pBb->max.d[1] = pos.d[1] > pBb->max.d[1] ? pos.d[1] : pBb->max.d[1];
}

CLUTRE_FORCE_INLINE
void clutreCmpBbWithVert(ClutreBb *pBb, const ClutreMesh *pMesh, int32_t vert) {
	clutreBbCmp(pBb, pMesh->fpPos(pMesh->pUserData, vert));
}

CLUTRE_FORCE_INLINE
ClutreBb clutreFaceBbGet(const ClutreTree *pTree, const ClutreMesh *pMesh, int32_t face) {
	ClutreBb bb = {.min = {.d = {FLT_MAX, FLT_MAX}}, .max = {.d = {-FLT_MAX, -FLT_MAX}}};
	int32_t faceIdx = pTree->pFaces[face];
	PixtyRange faceRange = pMesh->fpFaceRange(pMesh->pUserData, faceIdx);
	for (int32_t i = faceRange.start; i < faceRange.end; ++i) {
		clutreCmpBbWithVert(&bb, pMesh, pMesh->fpVert(pMesh->pUserData, i));
	}
	return bb;
}

#ifdef CLUTRE_DEBUG_VIS
static PixtyV3_F32 clutreCol[CLUTRE_POINT_COUNT] = {
	{.d = {1.0f, .0f, .0f}},
	{.d = {.0f, 1.0f, .0f}},
	{.d = {.0f, .0f, 1.0f}},
	{.d = {.5f, .5f, .0f}}
	/*
	{.d = {.0f, .5f, .5f}},
	{.d = {.5f, .0f, .5f}},
	{.d = {.75f, .25f, .25f}},
	{.d = {.25f, .75f, .25f}},
	{.d = {.25f, .25f, .75f}},
	{.d = {1.0f, .5f, .0f}},
	{.d = {.0f, 1.0f, .5f}},
	{.d = {.5f, .0f, 1.0f}},
	{.d = {.9f, .8f, .33f}},
	{.d = {.33f, .9f, .8f}},
	{.d = {.8f, .33f, .9f}},
	{.d = {1.0f, 1.0f, .75f}}
	*/
};

static inline
void clutreDumpSampleImg(
	const ClutreTree *pTree,
	const ClutreMesh *pMesh,
	ClutreImg *pImg,
	const ClutreBb *pFaceBb,
	const ClutreNode *pCluster,
	PixtyV2_I32 tile
) {
	int32_t imgRes = 2048;
	float fImgRes = (float)imgRes;
	if (!pImg->pData) {
		*pImg = (ClutreImg) {
			.width = imgRes,
			.height = imgRes,
		};
		pImg->pData =
			pTree->alloc.fpCalloc(3 * pImg->width * pImg->height, sizeof(float));
	}
	ClutreBb imgBb = {
		.min = {floorf(pFaceBb->min.d[0]), floorf(pFaceBb->min.d[1])},
		.max = {ceilf(pFaceBb->max.d[0]), ceilf(pFaceBb->max.d[1])}
	};
	PixtyV2_F32 imgSize = {
		imgBb.max.d[0] - imgBb.min.d[0],
		imgBb.max.d[1] - imgBb.min.d[1]
	};
	float imgSizeMax = imgSize.d[0] > imgSize.d[1] ? imgSize.d[0] : imgSize.d[1];
	PixtyV2_F32 fTile = {(float)tile.d[0], (float)tile.d[1]};
	ClutreBb region = {
		.min = _(_(_(pCluster->bb.min V2ADD fTile) V2SUB imgBb.min) V2DIVS imgSizeMax),
		.max = _(_(_(pCluster->bb.max V2ADD fTile) V2SUB imgBb.min) V2DIVS imgSizeMax)
	};
	PixtyV2_I32 start = {
		(int32_t)(region.min.d[0] * fImgRes),
		(int32_t)(region.min.d[1] * fImgRes)
	};
	PixtyV2_I32 end = {
		(int32_t)(region.max.d[0] * fImgRes),
		(int32_t)(region.max.d[1] * fImgRes)
	};
	start.d[0] = start.d[0] < 0 ? 0 : start.d[0];
	start.d[1] = start.d[1] < 0 ? 0 : start.d[1];
	end.d[0] = end.d[0] >= imgRes ? imgRes - 1 : end.d[0];
	end.d[1] = end.d[1] >= imgRes ? imgRes - 1 : end.d[1];
	for (int32_t i = start.d[1]; i <= end.d[1]; ++i) {
		for (int32_t j = start.d[0]; j <= end.d[0]; ++j) {
			PixtyV2_F32 pos = {.d = {(float)j, (float)i}};
			pos = _(pos V2DIVS fImgRes);
			pos = _(_(_(pos V2MULS imgSizeMax) V2ADD imgBb.min) V2SUB fTile);
			if (_(pos V2LESS pCluster->bb.min) || _(pos V2GREAT pCluster->bb.max)) {
				continue;
			}
			for (int32_t k = pCluster->faces.start; k < pCluster->faces.end; ++k) {
				ClutreBb faceBb = clutreFaceBbGet(pTree, pMesh, k);
				if (_(pos V2GREATEQL faceBb.min) && _(pos V2LESSEQL faceBb.max)) {
					PixtyV3_F32 pixelCol = _(clutreCol[pCluster->point] V3MULS .33f);
					int32_t linIdx = 3 * ((imgRes - 1 - i) * imgRes + j);
					_((PixtyV3_F32 *)&pImg->pData[linIdx] V3ADDEQL pixelCol);
					goto nextPixel;
				}
			}
nextPixel:
			;
		}
	}
}
#endif

static inline
ClutreNode *clutreStackTop(ClutreStack *pStack) {
	return pStack->stack[pStack->ptr].pNode;
}

static inline
int32_t clutreStackNextChild(const ClutreStack *pStack) {
	return pStack->stack[pStack->ptr].nextChild;
}

static inline
void clutreStackPush(ClutreStack *pStack, ClutreNode *pCluster) {
	++pStack->ptr;
	PIX_ERR_ASSERT("max stack depth hit", pStack->ptr < CLUTRE_STACK_SIZE);
	pStack->stack[pStack->ptr] = (ClutreStackEntry){.pNode = pCluster};
}

static inline
void clutreStackPushNextChild(ClutreStack *pStack) {
	ClutreStackEntry *pEntry = pStack->stack + pStack->ptr;
	clutreStackPush(pStack, pEntry->pNode->pChildren + pEntry->nextChild);
	++pEntry->nextChild;
}

static inline
void clutreStackPop(ClutreStack *pStack) {
	--pStack->ptr;
}

CLUTRE_FORCE_INLINE
const ClutreDfPoint *clutreNoiseSampleAtFace(
	const ClutreTree *pTree,
	const ClutreMesh *pMesh,
	const ClutreNoise *pNoise,
	const ClutreNode *pCluster,
	int32_t face,
	float scale,
	ClutreBb *pFaceBb
) {
	ClutreBb faceBb = clutreFaceBbGet(pTree, pMesh, face);
	if (pFaceBb) {
		*pFaceBb = faceBb;
	}
	PixtyV2_F32 faceCentre = _(_(faceBb.min V2ADD faceBb.max) V2DIVS 2.0f);
	const ClutreDfPoint *pSample =
		clutreNoiseSample(pNoise, faceCentre, clutreBbScale(pCluster->bb, scale));
	return pSample;
}

CLUTRE_FORCE_INLINE
PixErr clutreAssignFacesToPoints(
	const ClutreTree *pTree,
	const ClutreMesh *pMesh,
	const ClutreNoise *pNoise,
	ClutreBb *pBbBuf,
	int8_t *pClusterBuf,
	ClutreStack *pStack,
	float scale,
	int32_t *pPointCount,
	bool *pRetry
) {
	PixErr err = PIX_ERR_SUCCESS;
	ClutreNode *pCluster = clutreStackTop(pStack);
	for (int32_t i = 0; i < CLUTRE_POINT_COUNT; ++i) {
		pBbBuf[i] = (ClutreBb){
			.min = {.d = {FLT_MAX, FLT_MAX}},
			.max = {.d = {-FLT_MAX, -FLT_MAX}}
		};
	}
	int32_t perCluster[CLUTRE_POINT_COUNT] = {0};
	PixtyRange faces = pCluster->faces;
	for (int32_t i = faces.start; i < faces.end; ++i) {
		ClutreBb faceBb = {0};
		const ClutreDfPoint *pSample =
			clutreNoiseSampleAtFace(pTree, pMesh, pNoise, pCluster, i, scale, &faceBb);
		clutreContribFaceToBb(pBbBuf + pSample->nearest, &faceBb);
		pClusterBuf[i] = pSample->nearest;
		++perCluster[pSample->nearest];
	}
	int32_t pointCount = 0;
	int32_t validCount = 0;
	for (int32_t i = 0; i < CLUTRE_POINT_COUNT; ++i) {
		if (!perCluster[i]) {
			continue;
		}
		++pointCount;
		if (perCluster[i] >= CLUTRE_FACE_COUNT_MIN && clutreBbValidate(pBbBuf + i)) {
			++validCount;
		}
	}
	if (pRetry && (float)validCount / (float)pointCount < CLUTRE_VALID_MIN) {
		*pRetry = true;
		return err;
	}
	for (int32_t i = faces.start; i < faces.end; ++i) {
		int32_t faceCount = perCluster[pClusterBuf[i]];
		if (faceCount >= CLUTRE_FACE_COUNT_MIN) {
			continue;
		}
		PIX_ERR_ASSERT("", faceCount > 0);
		ClutreBb faceBb = {0};
		const ClutreDfPoint *pSample =
			clutreNoiseSampleAtFace(pTree, pMesh, pNoise, pCluster, i, scale, &faceBb);
		int32_t nearest = -1;
		for (int32_t j = 0; j < CLUTRE_POINT_COUNT; ++j) {
			if (perCluster[j] >= CLUTRE_FACE_COUNT_MIN &&
			    (nearest == -1 || pSample->layers[j] < pSample->layers[nearest])
			) {
				nearest = j;
			}
		}
		if (nearest == -1) {
			PIX_ERR_ASSERT("", pRetry);
			*pRetry = true;
			return err;
		}
		clutreContribFaceToBb(pBbBuf + nearest, &faceBb);
		pClusterBuf[i] = nearest;
		++perCluster[nearest];
	}
	if (pRetry) {
		*pRetry = false;
	}
	*pPointCount = validCount;
	return err;
}

CLUTRE_FORCE_INLINE
PixErr clutreDivide(
	ClutreTree *pTree,
	const ClutreMesh *pMesh,
	const ClutreNoise *pNoise,
	int8_t *pClusterBuf,
	PixtyI32Arr *pFaceBuf,
	ClutreStack *pStack
) {
	PixErr err = PIX_ERR_SUCCESS;
	ClutreNode *pCluster = clutreStackTop(pStack);
	if (pCluster->pChildren ||
	    (pCluster->faces.end - pCluster->faces.start) <= CLUTRE_FACE_COUNT_MIN
	) {
		return err;
	}
	int32_t pointCount = 0;
	ClutreBb bbBuf[CLUTRE_POINT_COUNT] = {0};
	for (int32_t i = 0; i < CLUTRE_NOISE_SCALE_MAX; ++i) {
		#ifdef CLUTRE_PATTERN_GRID
		//CLUTRE_NOISE_SCALE_MAX == CLUTRE_POINT_RES if CLUTRE_PATTERN_GRID is defined
		float scale = (float)CLUTRE_POINT_RES / (float)(CLUTRE_POINT_RES - i);
		#else
		float scale *= 2.0f * (float)(i + 1) / 2.0f;
		#endif
		bool retry = false;
		err = clutreAssignFacesToPoints(
			pTree,
			pMesh,
			pNoise,
			bbBuf,
			pClusterBuf,
			pStack,
			scale,
			&pointCount,
			i == CLUTRE_NOISE_SCALE_MAX - 1 ? NULL : &retry//last attempt cannot retry
		);
		PIX_ERR_RETURN_IFNOT(err, "");
		if (!retry) {
			break;
		}
	}
	if (pointCount <= 1) {
		return err;
	}
	ClutreNode *childRedir[CLUTRE_POINT_COUNT] = {0};
	err = clutreInitChildren(
		pTree,
		pCluster,
		pointCount,
		childRedir,
		pClusterBuf,
		pFaceBuf,
		bbBuf
	);
	PIX_ERR_RETURN_IFNOT(err, "");
	err = clutreReorderFaces(pTree, pCluster, pFaceBuf);
	PIX_ERR_RETURN_IFNOT(err, "");
	return err;
}

CLUTRE_FORCE_INLINE
PixErr clutreCallDivide(ClutreStack *pStack, void *pArgsRaw, bool *pAddChildren) {
	ClutreBuildLoopArgs *pArgs = pArgsRaw;
	return clutreDivide(
		pArgs->pTree,
		pArgs->pMesh,
		pArgs->pNoise,
		pArgs->pClusterBuf,
		pArgs->pFaceBuf,
		pStack
	);
}

CLUTRE_FORCE_INLINE
PixErr clutreLoopBody(
	ClutreStack *pStack,
	ClutreLoopFunc *pFunc
) {
	PixErr err = PIX_ERR_SUCCESS;
	bool addChildren = true;
	I32 nextChild = clutreStackNextChild(pStack);
	if (!nextChild) {
		err = pFunc->func(pStack, pFunc->pArgs, &addChildren);
		PIX_ERR_RETURN_IFNOT(err, "");
	}
	if (addChildren && nextChild < clutreStackTop(pStack)->childCount) {
		clutreStackPushNextChild(pStack);
	}
	else {
		clutreStackPop(pStack);
	}
	return err;
}

static inline
void clutreTreeDestroy(ClutreTree *pTree) {
	if (pTree->nodeAlloc.valid) {
		pixalcLinAllocDestroy(&pTree->nodeAlloc);
	}
	if (pTree->pFaces) {
		pTree->alloc.fpFree(pTree->pFaces);
	}
	*pTree = (ClutreTree){0};
}

CLUTRE_FORCE_INLINE
PixErr clutreTreeInit(
	const PixalcFPtrs *pAlloc,
	const ClutreMesh *pMesh,
	ClutreTree *pTree
) {
	PixErr err = PIX_ERR_SUCCESS;
	err = clutreMeshValidate(pMesh);
	PIX_ERR_RETURN_IFNOT(err, "invalid mesh");
	clutreTreeMemInit(pAlloc, pTree, pMesh);
	ClutreNoise noise = {0};
	clutreNoiseGen(pAlloc, &noise);
	ClutreStack stack = {.ptr = -1};
	clutreStackPush(&stack, pTree->pRoot);
	int8_t *pClusterBuf = pAlloc->fpMalloc(pMesh->faceCount);
	PixtyI32Arr faceBuf[CLUTRE_POINT_COUNT] = {0};
#ifdef CLUTRE_DEBUG_VIS
	ClutreImg imgArr[CLUTRE_STACK_SIZE] = {0};
#endif
	ClutreBuildLoopArgs loopArgs = {
		.pTree = pTree,
		.pMesh = pMesh,
		.pNoise = &noise,
		.pClusterBuf = pClusterBuf,
		.pFaceBuf = faceBuf
#ifdef CLUTRE_DEBUG_VIS
		,.pImgArr = imgArr
#endif
	};
	do {
		err = clutreLoopBody(
			&stack,
			&(ClutreLoopFunc){.func = clutreCallDivide, .pArgs = &loopArgs}
		);
		PIX_ERR_THROW_IFNOT(err, "", 0);
	} while(stack.ptr >= 0);
	pTree->valid = true;

	PIX_ERR_CATCH(0, err,
		clutreTreeDestroy(pTree);
	;);
#ifdef CLUTRE_DEBUG_VIS
	for (I32 i = 0; i < CLUTRE_STACK_SIZE; ++i) {
		if (imgArr[i].pData) {
			pAlloc->fpFree(imgArr[i].pData);
		}
	}
#endif
	clutreBuildCleanup(pTree, &noise, pClusterBuf, faceBuf);
	return err;
}

static inline
float clutreIntersect(PixtyV2_F32 a, PixtyV2_F32 ab, float c, int32_t axis) {
	return (c - a.d[axis]) / ab.d[axis];
}

static inline
PixtyV2_F32 clutreSlabSeg(PixtyV2_F32 a, PixtyV2_F32 ab, const ClutreBb *pBb, int32_t axis) {
	PixtyV2_F32 t = {
		clutreIntersect(a, ab, pBb->min.d[axis], axis),
		clutreIntersect(a, ab, pBb->max.d[axis], axis)
	};
	if (!isfinite(t.d[0]) || !isfinite(t.d[1])) {
		PixtyV2_F32 b = _(a V2ADD ab);
		if (a.d[axis] >= pBb->min.d[axis] && a.d[axis] <= pBb->max.d[axis] ||
		    b.d[axis] >= pBb->min.d[axis] && b.d[axis] <= pBb->max.d[axis]
		) {
			return (PixtyV2_F32){.0f, 1.0f};
		}
		return (PixtyV2_F32){-2.0f, 2.0f};
	}
	t.d[0] = t.d[0] < .0f ? .0f : t.d[0] > 1.0f ? 1.0f : t.d[0];
	t.d[1] = t.d[1] < .0f ? .0f : t.d[1] > 1.0f ? 1.0f : t.d[1];
	if (t.d[0] < t.d[1]) {
		return t;
	}
	return (PixtyV2_F32){t.d[1], t.d[0]};
}

static inline
void clutreMarkSide(
	PixtyV2_F32 seg,
	PixtyV2_F32 a,
	PixtyV2_F32 ab,
	const ClutreBb *pBb,
	bool *pSides,
	I32 axis
) {
	float pos = a.d[!axis] + ab.d[!axis] * seg.d[!axis];
	bool side = pos >= pBb->max.d[!axis];
	if(side || pos <= pBb->min.d[!axis]) {
		pSides[2 * axis + side] = true;
	}
}

static inline
ClutreIntersect clutreSlabTest(
	PixtyV2_F32 a,
	PixtyV2_F32 b,
	const ClutreBb *pBb,
	bool *pSides,
	PixtyV2_F32 *pAlphas
) {
	PixtyV2_F32 ab = _(b V2SUB a);
	PixtyV2_F32 segX = clutreSlabSeg(a, ab, pBb, 0);
	PixtyV2_F32 segY = clutreSlabSeg(a, ab, pBb, 1);
	if (!segX.d[0] && !segY.d[0] && segX.d[1] == 1.0f && segY.d[1] == 1.0f) {
		return CLUTRE_ENCLOSED;
	}
	bool inf = segX.d[0] == -2.0f || segY.d[0] == -2.0f;
	bool hitX = segX.d[0] < segX.d[1];
	bool hitY = segY.d[0] < segY.d[1];
	if (!inf && hitX && hitY &&
	    segX.d[0] <= segY.d[1] && segX.d[1] >= segY.d[0]
	) {
		if (pAlphas) {
			*pAlphas = (PixtyV2_F32){
				segX.d[0] > segY.d[0] ? segX.d[0] : segY.d[0],
				segX.d[1] < segY.d[1] ? segX.d[1] : segY.d[1]
			};
		}
		return CLUTRE_INTERSECT;
	}
	if (hitX) {
		clutreMarkSide(segX, a, ab, pBb, pSides, 0);
	}
	if (hitY) {
		clutreMarkSide(segY, a, ab, pBb, pSides, 1);
	}
	return CLUTRE_NO_INTERSECT;
}

//TODO this and above intersect funcs can be moved out of header file
static inline
ClutreIntersect clutreBbFaceIntersect(
	const ClutreBb *pBb,
	I32 faceSize,
	const PixtyV2_F32 *pPos,
	const ClutreBb *pFaceBb,
	PixtyV2_I32 tile
) {
	ClutreIntersect status = CLUTRE_NO_INTERSECT;
	if (pFaceBb->min.d[0] > pBb->max.d[0] || pFaceBb->max.d[0] < pBb->min.d[0] ||
	    pFaceBb->min.d[1] > pBb->max.d[1] || pFaceBb->max.d[1] < pBb->min.d[1]
	) {
		return status;
	}
	PixtyV2_F32 fTile = {(float)tile.d[0], (float)tile.d[1]};
	bool sides[4] = {0};
	for (int32_t i = 0; i < faceSize; ++i) {
		I32 iNext = (i + 1) % faceSize;
		PixtyV2_F32 a = _(pPos[i] V2SUB fTile);
		PixtyV2_F32 b = _(pPos[iNext] V2SUB fTile);
		switch (clutreSlabTest(a, b, pBb, sides, NULL)) {
			case CLUTRE_INTERSECT:
				return CLUTRE_INTERSECT;
			case CLUTRE_ENCLOSED:
				status = CLUTRE_ENCLOSED;
			default:
				;
		}
	}
	if (status != CLUTRE_NO_INTERSECT) {
		return status;
	}
	return sides[0] && sides[1] && sides[2] && sides[3] ? CLUTRE_ENCLOSING : status;
}

#define CLUTRE_FACE_MAX_SIZE 4

static inline
PixErr clutreSampleCluster(
	const ClutreTree *pTree,
	ClutreStack *pStack,
	ClutreArr *pClutreArr,
	const PixtyV2_F32 *pPos,
	const ClutreBb *pFaceBb,
	PixtyV2_I32 tile,
	I32 faceSize,
	bool enclosed,
	bool *pAdded,
	bool *pAddChildren
#ifdef CLUTRE_DEBUG_VIS
	,const ClutreMesh *pMesh,
	ClutreImg *pImg
#endif
) {
	PixErr err = PIX_ERR_SUCCESS;
	ClutreNode *pCluster = clutreStackTop(pStack);
	ClutreIntersect status =
		clutreBbFaceIntersect(&pCluster->bb, faceSize, pPos, pFaceBb, tile);
	if (enclosed && status != CLUTRE_ENCLOSED) {
		*pAdded = *pAddChildren = false;
		return err;
	}
	bool add = false;
	switch (status) {
		case CLUTRE_ENCLOSING:
			add = true;
			//v fallthrough v
		case CLUTRE_NO_INTERSECT:
			*pAddChildren = false;
			break;
		default:
			add = !pCluster->pChildren;
	}
	if (add) {
		err = clutreSampleAdd(&pTree->alloc, pClutreArr, pCluster->idx, status, tile);
		PIX_ERR_RETURN_IFNOT(err, "");
		#ifdef CLUTRE_DEBUG_VIS
			clutreDumpSampleImg(pTree, pMesh, pImg, pFaceBb, pCluster, tile);
		#endif
	}
	*pAdded = add;
	return err;
}

static inline
PixErr clutreCallSampleCluster(ClutreStack *pStack, void *pArgsRaw, bool *pAddChildren) {
	ClutreSampleLoopArgs *pArgs = pArgsRaw;
	return clutreSampleCluster(
		pArgs->pTree,
		pStack,
		pArgs->pClutreArr,
		pArgs->pPos,
		pArgs->pFaceBb,
		pArgs->tile,
		pArgs->faceSize,
		pArgs->enclosed,
		&pArgs->added,
		pAddChildren
#ifdef CLUTRE_DEBUG_VIS
		,pArgs->pMesh,
		pArgs->pImg
#endif
	);
}

typedef struct ClutreValidIdx {
	uint32_t idx : 31;
	uint32_t valid : 1;
} ClutreValidIdx;

typedef struct ClutreValidIdxArr {
	ClutreValidIdx *pArr;
	I32 size;
} ClutreValidIdxArr;

typedef struct ClutreStart {
	ClutreValidIdxArr arr;
	PixtyV2_I32 start;
	PixtyV2_I32 end;
} ClutreStart;

static
bool bbCropToTile(const ClutreFace *pFace, PixtyV2_I32 tile, ClutreBb *pBb) {
	*pBb = (ClutreBb){.min = {FLT_MAX, FLT_MAX}, .max = {-FLT_MAX, -FLT_MAX}};
	PixtyV2_F32 fTile = {(float)tile.d[0], (float)tile.d[1]};
	bool sides[4] = {0};
	for (int32_t i = 0; i < pFace->size; ++i) {
		I32 iNext = (i + 1) % pFace->size;
		PixtyV2_F32 a = pFace->fpPos(pFace->pUserData, i);
		PixtyV2_F32 b = pFace->fpPos(pFace->pUserData, iNext);
		PixtyV2_F32 alphas = {0};
		ClutreIntersect status = clutreSlabTest(
			a, b,
			&(ClutreBb){.min = fTile, .max = _(fTile V2ADDS 1.0f)},
			sides,
			&alphas
		);
		switch (status) {
			case CLUTRE_ENCLOSED:
				clutreBbCmp(pBb, a);
				break;
			case CLUTRE_INTERSECT: {
				PixtyV2_F32 ab = _(b V2SUB a);
				clutreBbCmp(pBb, _(a V2ADD _(ab V2MULS alphas.d[0])));
				clutreBbCmp(pBb, _(a V2ADD _(ab V2MULS alphas.d[1])));
				break;
			}
			default:
				;
		}
	}
	if (_(pBb->min V2LESSEQL _(fTile V2ADDS 1.0f)) && _(pBb->max V2GREATEQL fTile)) {
		return true;
	}
	if (sides[0] && sides[1] && sides[2] && sides[3]) {
		*pBb = (ClutreBb){.min = fTile, .max = _(fTile V2ADDS 1.0f)};//enclosing
		return true;
	}
	return false;
}

CLUTRE_FORCE_INLINE
PixErr clutreSampleForTile(
	const ClutreTree *pTree,
	const ClutreFace *pFace,
	const ClutreStart *pStart,
	ClutreArr *pArr,
	bool enclosed,
	ClutreBb faceBb,
	PixtyV2_F32 *pPos,
	PixtyV2_I32 tile
) {
	PixErr err = PIX_ERR_SUCCESS;
	const ClutreNode *pRoot;
	if (pStart) {
		//TODO implement this with a callback, rather than with a set struct
		ClutreValidIdx startIdx = pStart->arr.pArr[
			(tile.d[1] - pStart->start.d[1]) * (pStart->end.d[0] - pStart->start.d[0]) +
			tile.d[0] - pStart->start.d[0]
		];
		if (!startIdx.valid) {
			return err;
		}
		pRoot = pixalcLinAllocIdxConst(&pTree->nodeAlloc, startIdx.idx);
	}
	else {
		pRoot = pTree->pRoot;
	}
	I32 faceSize;
	ClutreBb bb;
	if (enclosed) {
		if (!bbCropToTile(pFace, tile, &bb)) {
			return err;
		}
		for (I32 i = 0; i < 4; ++i) {
			pPos[i] = (PixtyV2_F32){
				i % 3 ? bb.min.d[0] : bb.max.d[0],
				i / 2 ? bb.min.d[1] : bb.max.d[1]
			};
		}
		faceSize = 4;
	}
	else {
		faceSize = pFace->size;
		bb = faceBb;
	}
	ClutreBb tileBb = {
		.min = {(float)tile.d[0], (float)tile.d[1]},
		.max = {(float)(tile.d[0] + 1), (float)(tile.d[1] + 1)}
	};
	ClutreIntersect status = clutreBbFaceIntersect(
		&tileBb,
		faceSize,
		pPos,
		&bb,
		(PixtyV2_I32){0}
	);
	switch (status) {
		case CLUTRE_ENCLOSING:
			err = clutreSampleAdd(&pTree->alloc, pArr, pRoot->idx, status, tile);
			PIX_ERR_RETURN_IFNOT(err, "");
#ifdef CLUTRE_DEBUG_VIS
			clutreDumpSampleImg(pTree, pMesh, &img, &faceBb, pTree->pRoot, tile);
#endif
			//v fallthrough v
		case CLUTRE_NO_INTERSECT:
			return err;
		default:
			PIX_ERR_ASSERT("intersect status not set", status != CLUTRE_NONE);
	}
	ClutreStack stack = {.ptr = -1};
	//TODO temp fix, pStart loses const qualifier here
	clutreStackPush(&stack, (ClutreNode *)pStart);
	ClutreSampleLoopArgs loopArgs = {
		.pTree = pTree,
		.pClutreArr = pArr,
		.enclosed = enclosed,
		.tile = tile,
		.pPos = pPos,
		.pFaceBb = &bb
#ifdef CLUTRE_DEBUG_VIS
		,.pMesh = pMesh,
		.pImg = &img
#endif
	};
	do {
		err = clutreLoopBody(
			&stack,
			&(ClutreLoopFunc){.func = clutreCallSampleCluster, .pArgs = &loopArgs}
		);
		PIX_ERR_RETURN_IFNOT(err, "");
		if (enclosed) {
			if (loopArgs.added) {
				break;
			}
			const ClutreNode *pCluster = clutreStackTop(&stack);
			PIX_ERR_ASSERT("", clutreStackNextChild(&stack) <= pCluster->childCount);
			if (clutreStackNextChild(&stack) == pCluster->childCount
			) {
				err = clutreSampleAdd(
					&pTree->alloc,
					pArr,
					pCluster->idx,
					CLUTRE_ENCLOSED,
					tile
				);
				PIX_ERR_RETURN_IFNOT(err, "");
				break;
			}
		}
	} while(stack.ptr >= 0);
	return err;
}

//TODO replace pix err return with throw where appropriate
CLUTRE_FORCE_INLINE
PixErr clutreSampleForFace(
	const ClutreTree *pTree,
	const ClutreStart *pStart,
	const ClutreFace *pFace,
	ClutreArr *pArr,
	bool enclosed
#ifdef CLUTRE_DEBUG_VIS
	,const ClutreMesh *pMesh
#endif
) {
	PixErr err = PIX_ERR_SUCCESS;
	PIX_ERR_RETURN_IFNOT_COND(
		err,
		pFace->size > 0 && pFace->size <= CLUTRE_FACE_MAX_SIZE,
		""
	);
	ClutreBb faceBb = {.min = {FLT_MAX, FLT_MAX}, .max = {-FLT_MAX, -FLT_MAX}};
	PixtyV2_F32 pos[CLUTRE_FACE_MAX_SIZE] = {0};
	if (!enclosed) {
		for (int32_t i = 0; i < pFace->size; ++i) {
			pos[i] = pFace->fpPos(pFace->pUserData, i);
			clutreBbCmp(&faceBb, pos[i]);
		}
	}
	ClutreIBb iFaceBb = {
		.min = {(int32_t)faceBb.min.d[0], (int32_t)faceBb.min.d[1]},
		.max = {(int32_t)faceBb.max.d[0], (int32_t)faceBb.max.d[1]}
	};
#ifdef CLUTRE_DEBUG_VIS
	ClutreImg img = {0};
#endif
	for (int32_t i = iFaceBb.min.d[1]; i <= iFaceBb.max.d[1]; ++i) {
		for (int32_t j = iFaceBb.min.d[0]; j <= iFaceBb.max.d[0]; ++j) {
			PixtyV2_I32 tile = {j, i};
			clutreSampleForTile(
				pTree,
				pFace,
				pStart,
				pArr,
				enclosed,
				faceBb,
				pos,
				tile
			);
		}
	}
#ifdef CLUTRE_DEBUG_VIS
	if (img.pData) {
		pTree->alloc.fpFree(img.pData);
	}
#endif
	return err;
}

static inline
PixErr clutreIdx(const ClutreTree *pTree, int32_t idx, const ClutreNode **ppNode) {
	PixErr err = PIX_ERR_SUCCESS;
	PIX_ERR_RETURN_IFNOT_COND(err, pTree->valid && pTree->nodeAlloc.valid, "");
	*ppNode = pixalcLinAllocIdxConst(&pTree->nodeAlloc, idx);
	return err;
}

static inline
PixErr clutreFaceRangeGet(const ClutreTree *pTree, int32_t idx, PixtyRange *pRange) {
	PixErr err = PIX_ERR_SUCCESS;
	const ClutreNode *pCluster = NULL;
	err = clutreIdx(pTree, idx, &pCluster);
	PIX_ERR_RETURN_IFNOT(err, "");
	*pRange = pCluster->faces;
	return err;
}

static inline
PixErr clutreFacesGet(
	const ClutreTree *pTree,
	const ClutreNode *pCluster,
	ClutreFaceRange *pFaces
) {
	PixErr err = PIX_ERR_SUCCESS;
	PIX_ERR_RETURN_IFNOT_COND(err, pTree->valid, "");
	*pFaces = (ClutreFaceRange){
		.pArr = pTree->pFaces + pCluster->faces.start,
		.size = pCluster->faces.end - pCluster->faces.start
	};
	return err;
}

static inline
int32_t clutreTreeCount(const ClutreTree *pTree) {
	PIX_ERR_ASSERT("", pTree->valid);
	return pixalcLinAllocGetCount(&pTree->nodeAlloc);
}