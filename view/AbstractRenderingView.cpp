/*
 * AbstractRenderingView.cpp
 *
 * Copyright (C) 2010 by VISUS (Universitaet Stuttgart). 
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include "AbstractRenderingView.h"
#include "AbstractNamedObject.h"
#include "vislib/String.h"
#include <GL/gl.h>
#include "param/BoolParam.h"
#include "param/StringParam.h"
#include "utility/ColourParser.h"
#include "view/special/TitleRenderer.h"
#include "vislib/sysfunctions.h"

using namespace megamol::core;


/*
 * view::AbstractRenderingView::AbstractTitleRenderer::AbstractTitleRenderer
 */
view::AbstractRenderingView::AbstractTitleRenderer::AbstractTitleRenderer(void) {
    // intentionally empty
}


/*
 * view::AbstractRenderingView::AbstractTitleRenderer::~AbstractTitleRenderer
 */
view::AbstractRenderingView::AbstractTitleRenderer::~AbstractTitleRenderer(void) {
    // intentionally empty
}


/*
 * view::AbstractRenderingView::EmptyTitleRenderer::EmptyTitleRenderer
 */
view::AbstractRenderingView::EmptyTitleRenderer::EmptyTitleRenderer(void) {
    // intentionally empty
}


/*
 * view::AbstractRenderingView::EmptyTitleRenderer::~EmptyTitleRenderer
 */
view::AbstractRenderingView::EmptyTitleRenderer::~EmptyTitleRenderer(void) {
    // intentionally empty
}


/*
 * view::AbstractRenderingView::EmptyTitleRenderer::Create
 */
bool view::AbstractRenderingView::EmptyTitleRenderer::Create(void) {
    // intentionally empty
    return true;
}


/*
 * view::AbstractRenderingView::EmptyTitleRenderer::Render
 */
void view::AbstractRenderingView::EmptyTitleRenderer::Render(
        float tileX, float tileY, float tileW, float tileH,
        float virtW, float virtH, bool stereo, bool leftEye, double instTime,
        class ::megamol::core::CoreInstance *core) {
    ::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    ::glClear(GL_COLOR_BUFFER_BIT);
}


/*
 * view::AbstractRenderingView::EmptyTitleRenderer::Release
 */
void view::AbstractRenderingView::EmptyTitleRenderer::Release(void){
    // intentionally empty
}


/*
 * view::AbstractRenderingView::AbstractRenderingView
 */
view::AbstractRenderingView::AbstractRenderingView(void) : AbstractView(),
        overrideBkgndCol(NULL), overrideViewport(NULL),
        bkgndColSlot("backCol", "The views background colour"),
        softCursor(false), softCursorSlot("softCursor", "Bool flag to activate software cursor rendering"),
        titleRenderer(NULL), fpsCounter(10), fpsOutputTimer(0) {

    this->bkgndCol[0] = 0.0f;
    this->bkgndCol[1] = 0.0f;
    this->bkgndCol[2] = 0.125f;

    this->bkgndColSlot << new param::StringParam(utility::ColourParser::ToString(
        this->bkgndCol[0], this->bkgndCol[1], this->bkgndCol[2]));
    this->MakeSlotAvailable(&this->bkgndColSlot);

    this->softCursorSlot << new param::BoolParam(this->softCursor);
    this->MakeSlotAvailable(&this->softCursorSlot);

}


/*
 * view::AbstractRenderingView::~AbstractRenderingView
 */
view::AbstractRenderingView::~AbstractRenderingView(void) {
    this->removeTitleRenderer();
    this->overrideBkgndCol = NULL; // DO NOT DELETE
    this->overrideViewport = NULL; // DO NOT DELETE
}


/*
 * view::AbstractRenderingView::beginFrame
 */
void view::AbstractRenderingView::beginFrame(void) {
    //this->fpsCounter.FrameBegin(); // TODO: Enable multi-thread
}


/*
 * view::AbstractRenderingView::endFrame
 */
void view::AbstractRenderingView::endFrame(bool abort) {
    if (!abort) {
        unsigned int ticks = vislib::sys::GetTicksOfDay();
        if ((ticks < this->fpsOutputTimer) || (ticks >= this->fpsOutputTimer + 1000)) {
            this->fpsOutputTimer = ticks;
            vislib::StringA name("UNKNOWN");
            AbstractNamedObject *ano = dynamic_cast<AbstractNamedObject*>(this);
            if (ano != NULL) {
                name = ano->FullName();
            }
            // okey, does not make any sense when multiple windows are rendering, but better than nothing
            printf("%s FPS: %f\n", name.PeekBuffer(), this->fpsCounter.FPS());
            fflush(stdout); // grr
        }
    }
    //this->fpsCounter.FrameEnd(); // TODO: Enable multi-thread
}


/*
 * view::AbstractRenderingView::lastFrameTime
 */
double view::AbstractRenderingView::lastFrameTime(void) const {
    return this->fpsCounter.LastFrameTime();
}


/*
 * view::AbstractRenderingView::bkgndColour
 */
const float *view::AbstractRenderingView::bkgndColour(void) const {
    if (this->bkgndColSlot.IsDirty()) {
        this->bkgndColSlot.ResetDirty();
        utility::ColourParser::FromString(
            this->bkgndColSlot.Param<param::StringParam>()->Value(),
            this->bkgndCol[0], this->bkgndCol[1], this->bkgndCol[2]);
    }
    return this->bkgndCol;
}


/*
 * view::AbstractRenderingView::showSoftCursor
 */
bool view::AbstractRenderingView::showSoftCursor(void) const {
    if (this->softCursorSlot.IsDirty()) {
        this->softCursorSlot.ResetDirty();
        this->softCursor = this->softCursorSlot.Param<param::BoolParam>()->Value();
    }
    return this->softCursor;
}


/*
 * view::AbstractRenderingView::renderTitle
 */
void view::AbstractRenderingView::renderTitle(
        float tileX, float tileY, float tileW, float tileH,
        float virtW, float virtH, bool stereo, bool leftEye, double instTime) const {
    if (!this->titleRenderer) {
        this->titleRenderer = new special::TitleRenderer();
        if (!this->titleRenderer->Create()) {
            delete this->titleRenderer;
            this->titleRenderer = new EmptyTitleRenderer();
            ASSERT(this->titleRenderer->Create());
        }
    }

    this->titleRenderer->Render(tileX, tileY, tileW, tileH,
        virtW, virtH, stereo, leftEye, instTime, this->GetCoreInstance());

}


/*
 * view::AbstractRenderingView::removeTitleRenderer
 */
void view::AbstractRenderingView::removeTitleRenderer(void) const {
    if (this->titleRenderer) {
        this->titleRenderer->Release();
        delete this->titleRenderer;
        this->titleRenderer = NULL;
    }
}


/*
 * view::AbstractRenderingView::toggleSoftCurse
 */
void view::AbstractRenderingView::toggleSoftCurse(void) {
    this->softCursorSlot.Param<param::BoolParam>()->SetValue(!this->softCursor);
}
