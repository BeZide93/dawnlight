// Adapted from BeZide93/dawnlight-twilit-essentials, commit 805fac000711135ee2d85e086ab6100d767717bd.
// See docs/bossrush-twilit-essentials.md for provenance and integration details.
#pragma once

#include "global.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "m_Do/m_Do_ext.h"
#include "SSystem/SComponent/c_xyz.h"
#include "SSystem/SComponent/c_sxyz.h"

int loadObjectArchive(const char* arcName);

void unloadObjectArchive(const char* arcName);
J3DModel* loadBmdFromArc(const char* arcName, const char* bmdName, cXyz scale = cXyz(1.0f, 1.0f, 1.0f));
mDoExt_bckAnm* loadBckFromArc(const char* arcName, const char* bckName, int playMode = 2, f32 rate = 1.0f);
mDoExt_bckAnm* loadBckFromArcIdx(const char* arcName, int resIndex, int playMode = 2, f32 rate = 1.0f);
J3DModel* loadBmdFromArcIdx(const char* arcName, int resIndex, cXyz scale = cXyz(1.0f, 1.0f, 1.0f));

mDoExt_brkAnm* loadBrkFromArc(const char* arcName, const char* brkName, J3DModelData* modelData, int playMode = 2, f32 rate = 1.0f);
mDoExt_btkAnm* loadBtkFromArc(const char* arcName, const char* btkName, J3DModelData* modelData, int playMode = 2, f32 rate = 1.0f);
void renderModelAt(J3DModel* model, const cXyz& pos, const csXyz& angle = csXyz(0, 0, 0), const cXyz& scale = cXyz(1.0f, 1.0f, 1.0f), mDoExt_bckAnm* bck = nullptr);

void renderModelAtMtx(J3DModel* model, MtxP mtx, mDoExt_bckAnm* bck = nullptr);

void release_boss_gallery_archives();
