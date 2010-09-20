/*
 * View3D.cpp
 *
 * Copyright (C) 2008 - 2010 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include "View3D.h"
#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */
#include <GL/gl.h>
#include "view/CallRenderView.h"
#include "view/CameraParamOverride.h"
#include "param/BoolParam.h"
#include "param/ButtonParam.h"
#include "param/EnumParam.h"
#include "param/FloatParam.h"
#include "param/StringParam.h"
#include "param/Vector3fParam.h"
#include "CallRender3D.h"
#include "CoreInstance.h"
#include "utility/ColourParser.h"
#include "vislib/CameraParamsStore.h"
#include "vislib/Exception.h"
#include "vislib/Log.h"
#include "vislib/mathfunctions.h"
#include "vislib/Point.h"
#include "vislib/Quaternion.h"
#include "vislib/String.h"
#include "vislib/StringSerialiser.h"
#include "vislib/sysfunctions.h"
#ifdef ENABLE_KEYBOARD_VIEW_CONTROL
#include "vislib/KeyCode.h"
#endif /* ENABLE_KEYBOARD_VIEW_CONTROL */
#include "vislib/Trace.h"
#include "vislib/Vector.h"

using namespace megamol::core;


/*
 * view::View3D::View3D
 */
view::View3D::View3D(void) : view::AbstractView3D(), cam(), camParams(),
        camOverrides(), cursor2d(), modkeys(), rotator1(),
        rotator2(), zoomer1(), zoomer2(), lookAtDist(),
        rendererSlot("rendering", "Connects the view to a Renderer"),
        lightDir(0.5f, -1.0f, -1.0f), isCamLight(true), bboxs(),
        animPlaySlot("anim::play", "Bool parameter to play/stop the animation"),
        animSpeedSlot("anim::speed", "Float parameter of animation speed in time frames per second"),
        animTimeSlot("anim::time", "The slot holding the current time to display"),
        animOffsetSlot("anim::offset", "Slot used to synchronize the animation offset"),
        timeFrame(0.0f), timeFrameForced(false),
        showBBox("showBBox", "Bool parameter to show/hide the bounding box"),
        showLookAt("showLookAt", "Flag showing the look at point"),
        cameraSettingsSlot("camsettings", "The stored camera settings"),
        storeCameraSettingsSlot("storecam", "Triggers the storage of the camera settings"),
        restoreCameraSettingsSlot("restorecam", "Triggers the restore of the camera settings"),
        resetViewSlot("resetView", "Triggers the reset of the view"),
        fpsCounter(10), fpsOutputTimer(0), firstImg(false), frozenValues(NULL),
        isCamLightSlot("light::isCamLight", "Flag whether the light is relative to the camera or to the world coordinate system"),
        lightDirSlot("light::direction", "Direction vector of the light"),
        lightColDifSlot("light::diffuseCol", "Diffuse light colour"),
        lightColAmbSlot("light::ambientCol", "Ambient light colour"),
        stereoFocusDistSlot("stereo::focusDist", "focus distance for stereo projection"),
        stereoEyeDistSlot("stereo::eyeDist", "eye distance for stereo projection"),
        overrideCall(NULL),
#ifdef ENABLE_KEYBOARD_VIEW_CONTROL
        viewKeyMoveStepSlot("viewKey::MoveStep", "The move step size in world coordinates"),
        viewKeyAngleStepSlot("viewKey::AngleStep", "The angle rotate step in degrees"),
        viewKeyRotPointSlot("viewKey::RotPoint", "The point around which the view will be roateted"),
        viewKeyRotLeftSlot("viewKey::RotLeft", "Rotates the view to the left (around the up-axis)"),
        viewKeyRotRightSlot("viewKey::RotRight", "Rotates the view to the right (around the up-axis)"),
        viewKeyRotUpSlot("viewKey::RotUp", "Rotates the view to the top (around the right-axis)"),
        viewKeyRotDownSlot("viewKey::RotDown", "Rotates the view to the bottom (around the right-axis)"),
        viewKeyRollLeftSlot("viewKey::RollLeft", "Rotates the view counter-clockwise (around the view-axis)"),
        viewKeyRollRightSlot("viewKey::RollRight", "Rotates the view clockwise (around the view-axis)"),
        viewKeyZoomInSlot("viewKey::ZoomIn", "Zooms in (moves the camera)"),
        viewKeyZoomOutSlot("viewKey::ZoomOut", "Zooms out (moves the camera)"),
        viewKeyMoveLeftSlot("viewKey::MoveLeft", "Moves to the left"),
        viewKeyMoveRightSlot("viewKey::MoveRight", "Moves to the right"),
        viewKeyMoveUpSlot("viewKey::MoveUp", "Moves to the top"),
        viewKeyMoveDownSlot("viewKey::MoveDown", "Moves to the bottom"),
#endif /* ENABLE_KEYBOARD_VIEW_CONTROL */
        toggleBBoxSlot("toggleBBox", "Button to toggle the bounding box"),
        toggleSoftCursorSlot("toggleSoftCursor", "Button to toggle the soft cursor"),
        toggleAnimPlaySlot("toggleAnimPlay", "Button to toggle animation"),
        animSpeedUpSlot("anim::SpeedUp", "Speeds up the animation"),
        animSpeedDownSlot("anim::SpeedDown", "Slows down the animation") {
    using vislib::sys::KeyCode;

    this->camParams = this->cam.Parameters();
    this->camOverrides = new CameraParamOverride(this->camParams);

    vislib::graphics::ImageSpaceType defWidth(
        static_cast<vislib::graphics::ImageSpaceType>(100));
    vislib::graphics::ImageSpaceType defHeight(
        static_cast<vislib::graphics::ImageSpaceType>(100));

    this->camParams->SetVirtualViewSize(defWidth, defHeight);
    this->camParams->SetTileRect(vislib::math::Rectangle<float>(0.0f, 0.0f,
        defWidth, defHeight));

    this->rendererSlot.SetCompatibleCall<CallRender3DDescription>();
    this->MakeSlotAvailable(&this->rendererSlot);

    // empty bounding box will trigger initialisation
    this->bboxs.Clear();

    // simple animation time controlling (TODO: replace)
    this->animPlaySlot << new param::BoolParam(false);
    this->animPlaySlot.SetUpdateCallback(&View3D::onAnimPlayChanged);
    this->MakeSlotAvailable(&this->animPlaySlot);
    this->animSpeedSlot << new param::FloatParam(4.0f, 0.01f, 100.0f);
    this->animSpeedSlot.SetUpdateCallback(&View3D::onAnimSpeedChanged);
    this->MakeSlotAvailable(&this->animSpeedSlot);
    this->animTimeSlot << new param::FloatParam(this->timeFrame, 0.0f);
    this->MakeSlotAvailable(&this->animTimeSlot);
    this->animOffsetSlot << new param::FloatParam(0.0f);
    this->MakeSlotAvailable(&this->animOffsetSlot);

    this->animSpeedUpSlot << new param::ButtonParam('m');
    this->animSpeedUpSlot.SetUpdateCallback(&View3D::onAnimSpeedStep);
    this->MakeSlotAvailable(&this->animSpeedUpSlot);
    this->animSpeedDownSlot << new param::ButtonParam('n');
    this->animSpeedDownSlot.SetUpdateCallback(&View3D::onAnimSpeedStep);
    this->MakeSlotAvailable(&this->animSpeedDownSlot);

    this->showBBox << new param::BoolParam(true);
    this->MakeSlotAvailable(&this->showBBox);
    this->showLookAt << new param::BoolParam(false);
    this->MakeSlotAvailable(&this->showLookAt);

    this->cameraSettingsSlot << new param::StringParam("");
    this->MakeSlotAvailable(&this->cameraSettingsSlot);

    this->storeCameraSettingsSlot << new param::ButtonParam(
        vislib::sys::KeyCode::KEY_MOD_ALT | vislib::sys::KeyCode::KEY_MOD_SHIFT | 'C');
    this->storeCameraSettingsSlot.SetUpdateCallback(&View3D::onStoreCamera);
    this->MakeSlotAvailable(&this->storeCameraSettingsSlot);

    this->restoreCameraSettingsSlot << new param::ButtonParam(
        vislib::sys::KeyCode::KEY_MOD_ALT | 'c');
    this->restoreCameraSettingsSlot.SetUpdateCallback(&View3D::onRestoreCamera);
    this->MakeSlotAvailable(&this->restoreCameraSettingsSlot);

    this->resetViewSlot << new param::ButtonParam(vislib::sys::KeyCode::KEY_HOME);
    this->resetViewSlot.SetUpdateCallback(&View3D::onResetView);
    this->MakeSlotAvailable(&this->resetViewSlot);

    this->isCamLightSlot << new param::BoolParam(this->isCamLight);
    this->MakeSlotAvailable(&this->isCamLightSlot);

    this->lightDirSlot << new param::Vector3fParam(this->lightDir);
    this->MakeSlotAvailable(&this->lightDirSlot);

    this->lightColDif[0] = this->lightColDif[1] = this->lightColDif[2] = 1.0f;
    this->lightColDif[3] = 1.0f;

    this->lightColAmb[0] = this->lightColAmb[1] = this->lightColAmb[2] = 0.2f;
    this->lightColAmb[3] = 1.0f;

    this->lightColDifSlot << new param::StringParam(
        utility::ColourParser::ToString(
        this->lightColDif[0], this->lightColDif[1], this->lightColDif[2]));
    this->MakeSlotAvailable(&this->lightColDifSlot);

    this->lightColAmbSlot << new param::StringParam(
        utility::ColourParser::ToString(
        this->lightColAmb[0], this->lightColAmb[1], this->lightColAmb[2]));
    this->MakeSlotAvailable(&this->lightColAmbSlot);

    this->ResetView();

    this->stereoEyeDistSlot << new param::FloatParam(this->camParams->StereoDisparity(), 0.0f);
    this->MakeSlotAvailable(&this->stereoEyeDistSlot);

    this->stereoFocusDistSlot << new param::FloatParam(this->camParams->FocalDistance(false), 0.0f);
    this->MakeSlotAvailable(&this->stereoFocusDistSlot);

#ifdef ENABLE_KEYBOARD_VIEW_CONTROL
    this->viewKeyMoveStepSlot << new param::FloatParam(0.1f, 0.001f);
    this->MakeSlotAvailable(&this->viewKeyMoveStepSlot);

    this->viewKeyAngleStepSlot << new param::FloatParam(15.0f, 0.001f, 360.0f);
    this->MakeSlotAvailable(&this->viewKeyAngleStepSlot);

    param::EnumParam *vrpsev = new param::EnumParam(1);
    vrpsev->SetTypePair(0, "Position");
    vrpsev->SetTypePair(1, "Look-At");
    this->viewKeyRotPointSlot << vrpsev;
    this->MakeSlotAvailable(&this->viewKeyRotPointSlot);

    this->viewKeyRotLeftSlot << new param::ButtonParam(KeyCode::KEY_LEFT | KeyCode::KEY_MOD_CTRL);
    this->viewKeyRotLeftSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyRotLeftSlot);

    this->viewKeyRotRightSlot << new param::ButtonParam(KeyCode::KEY_RIGHT | KeyCode::KEY_MOD_CTRL);
    this->viewKeyRotRightSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyRotRightSlot);

    this->viewKeyRotUpSlot << new param::ButtonParam(KeyCode::KEY_UP | KeyCode::KEY_MOD_CTRL);
    this->viewKeyRotUpSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyRotUpSlot);

    this->viewKeyRotDownSlot << new param::ButtonParam(KeyCode::KEY_DOWN | KeyCode::KEY_MOD_CTRL);
    this->viewKeyRotDownSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyRotDownSlot);

    this->viewKeyRollLeftSlot << new param::ButtonParam(KeyCode::KEY_LEFT | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_SHIFT);
    this->viewKeyRollLeftSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyRollLeftSlot);

    this->viewKeyRollRightSlot << new param::ButtonParam(KeyCode::KEY_RIGHT | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_SHIFT);
    this->viewKeyRollRightSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyRollRightSlot);

    this->viewKeyZoomInSlot << new param::ButtonParam(KeyCode::KEY_UP | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_SHIFT);
    this->viewKeyZoomInSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyZoomInSlot);

    this->viewKeyZoomOutSlot << new param::ButtonParam(KeyCode::KEY_DOWN | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_SHIFT);
    this->viewKeyZoomOutSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyZoomOutSlot);

    this->viewKeyMoveLeftSlot << new param::ButtonParam(KeyCode::KEY_LEFT | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_ALT);
    this->viewKeyMoveLeftSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyMoveLeftSlot);

    this->viewKeyMoveRightSlot << new param::ButtonParam(KeyCode::KEY_RIGHT | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_ALT);
    this->viewKeyMoveRightSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyMoveRightSlot);

    this->viewKeyMoveUpSlot << new param::ButtonParam(KeyCode::KEY_UP | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_ALT);
    this->viewKeyMoveUpSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyMoveUpSlot);

    this->viewKeyMoveDownSlot << new param::ButtonParam(KeyCode::KEY_DOWN | KeyCode::KEY_MOD_CTRL | KeyCode::KEY_MOD_ALT);
    this->viewKeyMoveDownSlot.SetUpdateCallback(&View3D::viewKeyPressed);
    this->MakeSlotAvailable(&this->viewKeyMoveDownSlot);
#endif /* ENABLE_KEYBOARD_VIEW_CONTROL */

    this->toggleAnimPlaySlot << new param::ButtonParam(' ');
    this->toggleAnimPlaySlot.SetUpdateCallback(&View3D::onToggleButton);
    this->MakeSlotAvailable(&this->toggleAnimPlaySlot);

    this->toggleSoftCursorSlot << new param::ButtonParam('i' | KeyCode::KEY_MOD_CTRL);
    this->toggleSoftCursorSlot.SetUpdateCallback(&View3D::onToggleButton);
    this->MakeSlotAvailable(&this->toggleSoftCursorSlot);

    this->toggleBBoxSlot << new param::ButtonParam('i' | KeyCode::KEY_MOD_ALT);
    this->toggleBBoxSlot.SetUpdateCallback(&View3D::onToggleButton);
    this->MakeSlotAvailable(&this->toggleBBoxSlot);

}


/*
 * view::View3D::~View3D
 */
view::View3D::~View3D(void) {
    this->Release();
    SAFE_DELETE(this->frozenValues);
    this->overrideCall = NULL; // DO NOT DELETE
}


/*
 * view::View3D::GetCameraSyncNumber
 */
unsigned int view::View3D::GetCameraSyncNumber(void) const {
    return this->camParams->SyncNumber();
}


/*
 * view::View3D::SerialiseCamera
 */
void view::View3D::SerialiseCamera(vislib::Serialiser& serialiser) const {
    this->camParams->Serialise(serialiser);
}


/*
 * view::View3D::DeserialiseCamera
 */
void view::View3D::DeserialiseCamera(vislib::Serialiser& serialiser) {
    this->camParams->Deserialise(serialiser);
}


/*
 * view::View3D::Render
 */
void view::View3D::Render(void) {
    if (this->doHookCode()) {
        this->doBeforeRenderHook();
    }

    CallRender3D *cr3d = this->rendererSlot.CallAs<CallRender3D>();

    this->fpsCounter.FrameBegin();

    // clear viewport
    if (this->overrideViewport != NULL) {
        if ((this->overrideViewport[0] >= 0) && (this->overrideViewport[1] >= 0)
                && (this->overrideViewport[2] > 0) && (this->overrideViewport[3] > 0)) {
            ::glViewport(
                this->overrideViewport[0], this->overrideViewport[1],
                this->overrideViewport[2], this->overrideViewport[3]);
        }
    } else {
        // this is correct in non-override mode,
        //  because then the tile will be whole viewport
        ::glViewport(0, 0,
            static_cast<GLsizei>(this->camParams->TileRect().Width()),
            static_cast<GLsizei>(this->camParams->TileRect().Height()));
    }

    const float *bkgndCol = (this->overrideBkgndCol != NULL)
        ? this->overrideBkgndCol : this->bkgndColour();
    ::glClearColor(bkgndCol[0], bkgndCol[1], bkgndCol[2], 0.0f);
    ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (cr3d == NULL) {
        this->renderTitle(
            this->cam.Parameters()->TileRect().Left(),
            this->cam.Parameters()->TileRect().Bottom(),
            this->cam.Parameters()->TileRect().Width(),
            this->cam.Parameters()->TileRect().Height(),
            this->cam.Parameters()->VirtualViewSize().Width(),
            this->cam.Parameters()->VirtualViewSize().Height(),
            (this->cam.Parameters()->Projection() != vislib::graphics::CameraParameters::MONO_ORTHOGRAPHIC)
                && (this->cam.Parameters()->Projection() != vislib::graphics::CameraParameters::MONO_PERSPECTIVE),
            this->cam.Parameters()->Eye() == vislib::graphics::CameraParameters::LEFT_EYE);
        this->fpsCounter.FrameEnd();
        return; // empty enought
    } else {
        this->removeTitleRenderer();
    }

    if (this->overrideCall != NULL) {
        this->overrideCall->EnableOutputBuffer();
    } else {
        cr3d->SetOutputBuffer(GL_BACK);
    }

    // camera settings
    if (this->stereoEyeDistSlot.IsDirty()) {
        param::FloatParam *fp = this->stereoEyeDistSlot.Param<param::FloatParam>();
        this->camParams->SetStereoDisparity(fp->Value());
        fp->SetValue(this->camParams->StereoDisparity());
        this->stereoEyeDistSlot.ResetDirty();
    }
    if (this->stereoFocusDistSlot.IsDirty()) {
        param::FloatParam *fp = this->stereoFocusDistSlot.Param<param::FloatParam>();
        this->camParams->SetFocalDistance(fp->Value());
        fp->SetValue(this->camParams->FocalDistance(false));
        this->stereoFocusDistSlot.ResetDirty();
    }
    if (cr3d != NULL) {
        (*cr3d)(1); // GetExtents
        if (!(cr3d->AccessBoundingBoxes() == this->bboxs)) {
            this->bboxs = cr3d->AccessBoundingBoxes();

            if (this->firstImg) {
                this->ResetView();
                this->firstImg = false;
                if (!this->cameraSettingsSlot.Param<param::StringParam>()->Value().IsEmpty()) {
                    this->onRestoreCamera(this->restoreCameraSettingsSlot);
                }

            }
        }

        if (this->timeFrameForced) {
            this->timeFrameForced = false;
        } else {
            unsigned int frameCnt = cr3d->TimeFramesCount();
            if (this->animPlaySlot.Param<param::BoolParam>()->Value()) {
                double time = this->GetCoreInstance()->GetInstanceTime()
                    * static_cast<double>(this->animSpeedSlot.Param<param::FloatParam>()->Value())
                    + static_cast<double>(this->animOffsetSlot.Param<param::FloatParam>()->Value());

                while (time < 0.0) { time += static_cast<double>(frameCnt); }
                while (static_cast<unsigned int>(time) >= frameCnt) { time -= static_cast<double>(frameCnt); }

                this->timeFrame = static_cast<float>(time);
                this->animTimeSlot.Param<param::FloatParam>()->SetValue(this->timeFrame);

            } else if (this->animTimeSlot.IsDirty()) {
                this->animTimeSlot.ResetDirty();
                this->timeFrame = this->animTimeSlot.Param<param::FloatParam>()->Value();
                if (static_cast<unsigned int>(this->timeFrame) >= frameCnt) {
                    this->timeFrame = static_cast<float>(frameCnt - 1);
                    this->animTimeSlot.Param<param::FloatParam>()->SetValue(this->timeFrame);
                }
            }
        }

        cr3d->SetTime(this->frozenValues ? this->frozenValues->time : this->timeFrame);
        cr3d->SetCameraParameters(this->cam.Parameters()); // < here we use the 'active' parameters!
        cr3d->SetLastFrameTime(this->fpsCounter.LastFrameTime());
    }
    this->camParams->CalcClipping(this->bboxs.ClipBox(), 0.1f);

    // set light parameters
    if (this->isCamLightSlot.IsDirty()) {
        this->isCamLightSlot.ResetDirty();
        this->isCamLight = this->isCamLightSlot.Param<param::BoolParam>()->Value();
    }
    if (this->lightDirSlot.IsDirty()) {
        this->lightDirSlot.ResetDirty();
        this->lightDir = this->lightDirSlot.Param<param::Vector3fParam>()->Value();
    }
    if (this->lightColAmbSlot.IsDirty()) {
        this->lightColAmbSlot.ResetDirty();
        utility::ColourParser::FromString(
            this->lightColAmbSlot.Param<param::StringParam>()->Value(),
            this->lightColAmb[0], lightColAmb[1], lightColAmb[2]);
    }
    if (this->lightColDifSlot.IsDirty()) {
        this->lightColDifSlot.ResetDirty();
        utility::ColourParser::FromString(
            this->lightColDifSlot.Param<param::StringParam>()->Value(),
            this->lightColDif[0], lightColDif[1], lightColDif[2]);
    }
    ::glEnable(GL_LIGHTING);    // TODO: check renderer capabilities
    ::glEnable(GL_LIGHT0);
    const float lp[4] = {
        -this->lightDir.X(),
        -this->lightDir.Y(),
        -this->lightDir.Z(),
        0.0f};
    ::glLightfv(GL_LIGHT0, GL_AMBIENT, this->lightColAmb);
    ::glLightfv(GL_LIGHT0, GL_DIFFUSE, this->lightColDif);
    const float zeros[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const float ones[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ::glLightfv(GL_LIGHT0, GL_SPECULAR, ones);
    ::glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zeros);

    // setup matrices
    ::glMatrixMode(GL_PROJECTION);
    ::glLoadIdentity();
    this->cam.glMultProjectionMatrix();

    ::glMatrixMode(GL_MODELVIEW);
    ::glLoadIdentity();

    if (this->isCamLight) glLightfv(GL_LIGHT0, GL_POSITION, lp);

    this->cam.glMultViewMatrix();

    if (!this->isCamLight) glLightfv(GL_LIGHT0, GL_POSITION, lp);

    // render bounding box backside
    if (this->showBBox.Param<param::BoolParam>()->Value()) {
        this->renderBBoxBackside();
    }
    if (this->showLookAt.Param<param::BoolParam>()->Value()) {
        this->renderLookAt();
    }
    ::glPushMatrix();

    if (this->overrideCall) {
        (*static_cast<AbstractCallRender*>(cr3d)) = *this->overrideCall;
    }

    // call for render
    if (cr3d != NULL) {
        (*cr3d)(0);
    }

    // render bounding box front
    ::glPopMatrix();
    if (this->showBBox.Param<param::BoolParam>()->Value()) {
        this->renderBBoxFrontside();
    }

    if (this->showSoftCursor()) {
        this->renderSoftCursor();
    }

    if (true) {
        //::glMatrixMode(GL_PROJECTION);
        //::glLoadIdentity();
        //::glMatrixMode(GL_MODELVIEW);
        //::glLoadIdentity();
        unsigned int ticks = vislib::sys::GetTicksOfDay();
        if ((ticks < this->fpsOutputTimer) || (ticks >= this->fpsOutputTimer + 1000)) {
            this->fpsOutputTimer = ticks;
            printf("FPS: %f\n", this->fpsCounter.FPS());
            fflush(stdout); // grr
        }
    }

    this->fpsCounter.FrameEnd();

}


/*
 * view::View3D::ResetView
 */
void view::View3D::ResetView(void) {
    using namespace vislib::graphics;
    VLTRACE(VISLIB_TRCELVL_INFO, "View3D::ResetView\n");

    this->camParams->SetClip(0.1f, 100.0f);
    this->camParams->SetApertureAngle(30.0f);
    this->camParams->SetProjection(
        vislib::graphics::CameraParameters::MONO_PERSPECTIVE);
    this->camParams->SetStereoParameters(0.3f, /* this is not so clear! */
        vislib::graphics::CameraParameters::LEFT_EYE,
        0.0f /* this is autofocus */);
    this->camParams->Limits()->LimitClippingDistances(0.01f, 0.1f);

    if (!this->bboxs.IsWorldSpaceBBoxValid()) {
        this->bboxs.SetWorldSpaceBBox(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0);
    }
    float dist = (0.5f
        * sqrtf((this->bboxs.WorldSpaceBBox().Width() * this->bboxs.WorldSpaceBBox().Width())
        + (this->bboxs.WorldSpaceBBox().Depth() * this->bboxs.WorldSpaceBBox().Depth())
        + (this->bboxs.WorldSpaceBBox().Height() * this->bboxs.WorldSpaceBBox().Height())))
        / tanf(this->cam.Parameters()->HalfApertureAngle());
    SceneSpacePoint3D bbc = this->bboxs.WorldSpaceBBox().CalcCenter();

    this->camParams->SetView(
        bbc + SceneSpaceVector3D(0.0f, 0.0f, dist),
        bbc, SceneSpaceVector3D(0.0f, 1.0f, 0.0f));

    this->zoomer1.SetSpeed(dist);
    this->lookAtDist.SetSpeed(dist);
}


/*
 * view::View3D::Resize
 */
void view::View3D::Resize(unsigned int width, unsigned int height) {
    this->camParams->SetVirtualViewSize(
        static_cast<vislib::graphics::ImageSpaceType>(width), 
        static_cast<vislib::graphics::ImageSpaceType>(height));
}


/*
 * view::View3D::SetInputModifier
 */
void view::View3D::SetInputModifier(mmcInputModifier mod, bool down) {
    unsigned int modId = 0;
    switch (mod) {
        case MMC_INMOD_SHIFT:
            modId = vislib::graphics::InputModifiers::MODIFIER_SHIFT;
            break;
        case MMC_INMOD_CTRL:
            modId = vislib::graphics::InputModifiers::MODIFIER_CTRL;
            break;
        case MMC_INMOD_ALT:
            modId = vislib::graphics::InputModifiers::MODIFIER_ALT;
            break;
        default: return;
    }
    this->modkeys.SetModifierState(modId, down);
}


/*
 * view::View3D::OnRenderView
 */
bool view::View3D::OnRenderView(Call& call) {
    float overBC[3];
    int overVP[4] = {0, 0, 0, 0};
    view::CallRenderView *crv = dynamic_cast<view::CallRenderView *>(&call);
    if (crv == NULL) return false;

    this->overrideViewport = overVP;
    if (crv->IsProjectionSet() || crv->IsTileSet() || crv->IsViewportSet()) {
        this->cam.SetParameters(this->camOverrides);
        this->camOverrides.DynamicCast<CameraParamOverride>()
            ->SetOverrides(*crv);
        if (crv->IsViewportSet()) {
            overVP[2] = crv->ViewportWidth();
            overVP[3] = crv->ViewportHeight();
            if (!crv->IsTileSet()) {
                this->camOverrides->SetVirtualViewSize(
                    static_cast<vislib::graphics::ImageSpaceType>(crv->ViewportWidth()),
                    static_cast<vislib::graphics::ImageSpaceType>(crv->ViewportHeight()));
                this->camOverrides->ResetTileRect();
            }
        }
    }
    if (crv->IsBackgroundSet()) {
        overBC[0] = static_cast<float>(crv->BackgroundRed()) / 255.0f;
        overBC[1] = static_cast<float>(crv->BackgroundGreen()) / 255.0f;
        overBC[2] = static_cast<float>(crv->BackgroundBlue()) / 255.0f;
        this->overrideBkgndCol = overBC; // hurk
    }

    this->overrideCall = dynamic_cast<AbstractCallRender*>(&call);

    this->Render();

    this->overrideCall = NULL;

    if (crv->IsProjectionSet() || crv->IsTileSet() || crv->IsViewportSet()) {
        this->cam.SetParameters(this->frozenValues ? this->frozenValues->camParams : this->camParams);
    }
    this->overrideBkgndCol = NULL;
    this->overrideViewport = NULL;

    return true;
}


/*
 * view::View3D::UpdateFreeze
 */
void view::View3D::UpdateFreeze(bool freeze) {
    //printf("%s view\n", freeze ? "Freezing" : "Unfreezing");
    if (freeze) {
        if (this->frozenValues == NULL) {
            this->frozenValues = new FrozenValues();
            this->camOverrides.DynamicCast<CameraParamOverride>()
                ->SetParametersBase(this->frozenValues->camParams);
            this->cam.SetParameters(this->frozenValues->camParams);
        }
        *(this->frozenValues->camParams) = *this->camParams;
        this->frozenValues->time = this->timeFrame;
    } else {
        this->camOverrides.DynamicCast<CameraParamOverride>()
            ->SetParametersBase(this->camParams);
        this->cam.SetParameters(this->camParams);
        SAFE_DELETE(this->frozenValues);
    }
}


/*
 * view::View3D::unpackMouseCoordinates
 */
void view::View3D::unpackMouseCoordinates(float &x, float &y) {
    x *= this->camParams->VirtualViewSize().Width();
    y *= this->camParams->VirtualViewSize().Height();
    y -= 1.0f;
}


/*
 * view::View3D::create
 */
bool view::View3D::create(void) {
    
    this->cursor2d.SetButtonCount(3); /* This could be configurable. */
    this->modkeys.SetModifierCount(3);

    this->rotator1.SetCameraParams(this->camParams);
    this->rotator1.SetTestButton(0 /* left mouse button */);
    this->rotator1.SetAltModifier(
        vislib::graphics::InputModifiers::MODIFIER_SHIFT);
    this->rotator1.SetModifierTestCount(1);
    this->rotator1.SetModifierTest(0,
        vislib::graphics::InputModifiers::MODIFIER_CTRL, false);

    this->rotator2.SetCameraParams(this->camParams);
    this->rotator2.SetTestButton(0 /* left mouse button */);
    this->rotator2.SetAltModifier(
        vislib::graphics::InputModifiers::MODIFIER_SHIFT);
    this->rotator2.SetModifierTestCount(1);
    this->rotator2.SetModifierTest(0,
        vislib::graphics::InputModifiers::MODIFIER_CTRL, true);

    this->zoomer1.SetCameraParams(this->camParams);
    this->zoomer1.SetTestButton(2 /* mid mouse button */);
    this->zoomer1.SetModifierTestCount(2);
    this->zoomer1.SetModifierTest(0,
        vislib::graphics::InputModifiers::MODIFIER_ALT, false);
    this->zoomer1.SetModifierTest(1,
        vislib::graphics::InputModifiers::MODIFIER_CTRL, false);
    this->zoomer1.SetZoomBehaviour(vislib::graphics::CameraZoom2DMove::FIX_LOOK_AT);

    this->zoomer2.SetCameraParams(this->camParams);
    this->zoomer2.SetTestButton(2 /* mid mouse button */);
    this->zoomer2.SetModifierTestCount(2);
    this->zoomer2.SetModifierTest(0,
        vislib::graphics::InputModifiers::MODIFIER_ALT, true);
    this->zoomer2.SetModifierTest(1,
        vislib::graphics::InputModifiers::MODIFIER_CTRL, false);

    this->lookAtDist.SetCameraParams(this->camParams);
    this->lookAtDist.SetTestButton(2 /* mid mouse button */);
    this->lookAtDist.SetModifierTestCount(1);
    this->lookAtDist.SetModifierTest(0,
        vislib::graphics::InputModifiers::MODIFIER_CTRL, true);

    this->cursor2d.SetCameraParams(this->camParams);
    this->cursor2d.RegisterCursorEvent(&this->rotator1);
    this->cursor2d.RegisterCursorEvent(&this->rotator2);
    this->cursor2d.RegisterCursorEvent(&this->zoomer1);
    this->cursor2d.RegisterCursorEvent(&this->zoomer2);
    this->cursor2d.RegisterCursorEvent(&this->lookAtDist);
    this->cursor2d.SetInputModifiers(&this->modkeys);

    this->modkeys.RegisterObserver(&this->cursor2d);

    this->firstImg = true;

    return true;
}


/*
 * view::View3D::release
 */
void view::View3D::release(void) {
    this->removeTitleRenderer();
    this->cursor2d.UnregisterCursorEvent(&this->rotator1);
    this->cursor2d.UnregisterCursorEvent(&this->rotator2);
    this->cursor2d.UnregisterCursorEvent(&this->zoomer1);
    this->cursor2d.UnregisterCursorEvent(&this->zoomer2);
    SAFE_DELETE(this->frozenValues);
}


/*
 * view::View3D::renderBBox
 */
void view::View3D::renderBBox(void) {
    const vislib::math::Cuboid<float>& boundingBox
        = this->bboxs.WorldSpaceBBox();
    if (!this->bboxs.IsWorldSpaceBBoxValid()) {
        this->bboxs.SetWorldSpaceBBox(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f);
    }

    ::glBegin(GL_QUADS);

    ::glEdgeFlag(true);

    ::glVertex3f(boundingBox.Left(),  boundingBox.Bottom(), boundingBox.Back());
    ::glVertex3f(boundingBox.Left(),  boundingBox.Top(),    boundingBox.Back());
    ::glVertex3f(boundingBox.Right(), boundingBox.Top(),    boundingBox.Back());
    ::glVertex3f(boundingBox.Right(), boundingBox.Bottom(), boundingBox.Back());

    ::glVertex3f(boundingBox.Left(),  boundingBox.Bottom(), boundingBox.Front());
    ::glVertex3f(boundingBox.Right(), boundingBox.Bottom(), boundingBox.Front());
    ::glVertex3f(boundingBox.Right(), boundingBox.Top(),    boundingBox.Front());
    ::glVertex3f(boundingBox.Left(),  boundingBox.Top(),    boundingBox.Front());

    ::glVertex3f(boundingBox.Left(),  boundingBox.Top(),    boundingBox.Back());
    ::glVertex3f(boundingBox.Left(),  boundingBox.Top(),    boundingBox.Front());
    ::glVertex3f(boundingBox.Right(), boundingBox.Top(),    boundingBox.Front());
    ::glVertex3f(boundingBox.Right(), boundingBox.Top(),    boundingBox.Back());

    ::glVertex3f(boundingBox.Left(),  boundingBox.Bottom(), boundingBox.Back());
    ::glVertex3f(boundingBox.Right(), boundingBox.Bottom(), boundingBox.Back());
    ::glVertex3f(boundingBox.Right(), boundingBox.Bottom(), boundingBox.Front());
    ::glVertex3f(boundingBox.Left(),  boundingBox.Bottom(), boundingBox.Front());

    ::glVertex3f(boundingBox.Left(),  boundingBox.Bottom(), boundingBox.Back());
    ::glVertex3f(boundingBox.Left(),  boundingBox.Bottom(), boundingBox.Front());
    ::glVertex3f(boundingBox.Left(),  boundingBox.Top(),    boundingBox.Front());
    ::glVertex3f(boundingBox.Left(),  boundingBox.Top(),    boundingBox.Back());

    ::glVertex3f(boundingBox.Right(), boundingBox.Bottom(), boundingBox.Back());
    ::glVertex3f(boundingBox.Right(), boundingBox.Top(),    boundingBox.Back());
    ::glVertex3f(boundingBox.Right(), boundingBox.Top(),    boundingBox.Front());
    ::glVertex3f(boundingBox.Right(), boundingBox.Bottom(), boundingBox.Front());

    ::glEnd();
}


/*
 * view::View3D::renderBBoxBackside
 */
void view::View3D::renderBBoxBackside(void) {
    ::glDisable(GL_LIGHTING);
    ::glEnable(GL_BLEND);
    ::glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ::glEnable(GL_DEPTH_TEST);
    ::glEnable(GL_CULL_FACE);
    ::glCullFace(GL_FRONT);
    ::glEnable(GL_LINE_SMOOTH);
    ::glLineWidth(1.25f);
    ::glDisable(GL_TEXTURE_2D);
    ::glPolygonMode(GL_BACK, GL_LINE);

    ::glColor4ub(255, 255, 255, 50);
    this->renderBBox();

    ::glPolygonMode(GL_BACK, GL_FILL);
    ::glDisable(GL_DEPTH_TEST);

    ::glColor4ub(255, 255, 255, 12);
    this->renderBBox();

    ::glCullFace(GL_BACK);
}


/*
 * view::View3D::renderBBoxFrontside
 */
void view::View3D::renderBBoxFrontside(void) {
    ::glDisable(GL_LIGHTING);
    ::glEnable(GL_BLEND);
    ::glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    ::glEnable(GL_DEPTH_TEST);
    ::glDepthFunc(GL_LEQUAL);
    ::glEnable(GL_CULL_FACE);
    ::glCullFace(GL_BACK);
    ::glEnable(GL_LINE_SMOOTH);
    ::glLineWidth(1.75f);
    ::glDisable(GL_TEXTURE_2D);
    ::glPolygonMode(GL_FRONT, GL_LINE);

    ::glColor4ub(255, 255, 255, 100);
    this->renderBBox();

    ::glDepthFunc(GL_LESS);
    ::glPolygonMode(GL_FRONT, GL_FILL);
}


/*
 * view::View3D::renderLookAt
 */
void view::View3D::renderLookAt(void) {
    const vislib::math::Cuboid<float>& boundingBox
        = this->bboxs.WorldSpaceBBox();
    vislib::math::Point<float, 3> minp(
        vislib::math::Min(boundingBox.Left(), boundingBox.Right()),
        vislib::math::Min(boundingBox.Bottom(), boundingBox.Top()),
        vislib::math::Min(boundingBox.Back(), boundingBox.Front()));
    vislib::math::Point<float, 3> maxp(
        vislib::math::Max(boundingBox.Left(), boundingBox.Right()),
        vislib::math::Max(boundingBox.Bottom(), boundingBox.Top()),
        vislib::math::Max(boundingBox.Back(), boundingBox.Front()));
    vislib::math::Point<float, 3> lap = this->cam.Parameters()->LookAt();
    bool xin = true;
    if (lap.X() < minp.X()) { lap.SetX(minp.X()); xin = false; }
    else if (lap.X() > maxp.X()) { lap.SetX(maxp.X()); xin = false; }
    bool yin = true;
    if (lap.Y() < minp.Y()) { lap.SetY(minp.Y()); yin = false; }
    else if (lap.Y() > maxp.Y()) { lap.SetY(maxp.Y()); yin = false; }
    bool zin = true;
    if (lap.Z() < minp.Z()) { lap.SetZ(minp.Z()); zin = false; }
    else if (lap.Z() > maxp.Z()) { lap.SetZ(maxp.Z()); zin = false; }

    ::glDisable(GL_LIGHTING);
    ::glLineWidth(1.4f);
    ::glEnable(GL_BLEND);
    ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ::glEnable(GL_DEPTH_TEST);

    ::glBegin(GL_LINES);

    ::glColor3ub(255, 0, 0);

    ::glVertex3f(lap.X(), lap.Y(), lap.Z());
    ::glVertex3f(maxp.X(), lap.Y(), lap.Z());
    ::glColor3ub(192, 192, 192);
    ::glVertex3f(lap.X(), lap.Y(), lap.Z());
    ::glVertex3f(minp.X(), lap.Y(), lap.Z());

    ::glColor3ub(0, 255, 0);
    ::glVertex3f(lap.X(), lap.Y(), lap.Z());
    ::glVertex3f(lap.X(), maxp.Y(), lap.Z());
    ::glColor3ub(192, 192, 192);
    ::glVertex3f(lap.X(), lap.Y(), lap.Z());
    ::glVertex3f(lap.X(), minp.Y(), lap.Z());

    ::glColor3ub(0, 0, 255);
    ::glVertex3f(lap.X(), lap.Y(), lap.Z());
    ::glVertex3f(lap.X(), lap.Y(), maxp.Z());
    ::glColor3ub(192, 192, 192);
    ::glVertex3f(lap.X(), lap.Y(), lap.Z());
    ::glVertex3f(lap.X(), lap.Y(), minp.Z());

    ::glEnd();

    ::glEnable(GL_LIGHTING);
}


/*
 * view::View3D::renderSoftCursor
 */
void view::View3D::renderSoftCursor(void) {
    ::glMatrixMode(GL_PROJECTION);
    ::glLoadIdentity();
    ::glMatrixMode(GL_MODELVIEW);
    ::glLoadIdentity();

    vislib::SmartPtr<vislib::graphics::CameraParameters> params = this->cam.Parameters();

    const float cursorScale = 1.0f;

    ::glTranslatef(-1.0f, -1.0f, 0.0f);
    ::glScalef(2.0f / params->TileRect().Width(), 2.0f / params->TileRect().Height(), 1.0f);
    ::glTranslatef(-params->TileRect().Left(), -params->TileRect().Bottom(), 0.0f);
    ::glScalef(params->VirtualViewSize().Width() / this->camParams->VirtualViewSize().Width(),
        params->VirtualViewSize().Height() / this->camParams->VirtualViewSize().Height(),
        1.0f);
    ::glTranslatef(this->cursor2d.X(), this->cursor2d.Y(), 0.0f);
    ::glScalef(cursorScale * this->camParams->VirtualViewSize().Width() / params->VirtualViewSize().Width(),
        - cursorScale * this->camParams->VirtualViewSize().Height() / params->VirtualViewSize().Height(),
        1.0f);

    ::glEnable(GL_BLEND);
    ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ::glEnable(GL_LINE_SMOOTH);
    ::glLineWidth(1.5f);
    ::glBindTexture(GL_TEXTURE_2D, 0);
    ::glDisable(GL_TEXTURE_2D);
    ::glDisable(GL_LIGHTING);
    ::glDisable(GL_DEPTH_TEST);

    ::glBegin(GL_TRIANGLE_FAN);
    ::glColor3ub(255, 255, 255); ::glVertex2i(0, 0);
    ::glColor3ub(245, 245, 245); ::glVertex2i(0, 17);
    ::glColor3ub(238, 238, 238); ::glVertex2i(4, 13);
    ::glColor3ub(211, 211, 211); ::glVertex2i(7, 18);
    ::glVertex2i(9, 18);
    ::glVertex2i(9, 16);
    ::glColor3ub(234, 234, 234); ::glVertex2i(7, 12);
    ::glColor3ub(226, 226, 226); ::glVertex2i(12, 12);
    ::glEnd();
    ::glBegin(GL_LINE_LOOP);
    ::glColor3ub(0, 0, 0);
    ::glVertex2i(0, 0);
    ::glVertex2i(0, 17);
    ::glVertex2i(4, 13);
    ::glVertex2i(7, 18);
    ::glVertex2i(9, 18);
    ::glVertex2i(9, 16);
    ::glVertex2i(7, 12);
    ::glVertex2i(12, 12);
    ::glEnd();

    ::glDisable(GL_LINE_SMOOTH);
    ::glDisable(GL_BLEND);
}


/*
 * view::View3D::onStoreCamera
 */
bool view::View3D::onStoreCamera(param::ParamSlot& p) {
    vislib::TStringSerialiser strser;
    strser.ClearData();
    this->camParams->Serialise(strser);
    vislib::TString str(strser.GetString());
    str.EscapeCharacters(_T('\\'), _T("\n\r\t"), _T("nrt"));
    str.Append(_T("}"));
    str.Prepend(_T("{"));
    this->cameraSettingsSlot.Param<param::StringParam>()->SetValue(str);

    vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_INFO,
        "Camera parameters stored in \"%s\"",
        this->cameraSettingsSlot.FullName().PeekBuffer());
    return true;
}


/*
 * view::View3D::onRestoreCamera
 */
bool view::View3D::onRestoreCamera(param::ParamSlot& p) {
    vislib::TString str = this->cameraSettingsSlot.Param<param::StringParam>()->Value();
    try {
        if ((str[0] != _T('{')) || (str[str.Length() - 1] != _T('}'))) {
            throw vislib::Exception("invalid string: surrounding brackets missing", __FILE__, __LINE__);
        }
        str = str.Substring(1, str.Length() - 2);
        if (!str.UnescapeCharacters(_T('\\'), _T("\n\r\t"), _T("nrt"))) {
            throw vislib::Exception("unrecognised escape sequence", __FILE__, __LINE__);
        }
        vislib::TStringSerialiser strser(str);
        vislib::graphics::CameraParamsStore cps;
        cps = *this->camParams.operator->();
        cps.Deserialise(strser);
        cps.SetVirtualViewSize(this->camParams->VirtualViewSize());
        cps.SetTileRect(this->camParams->TileRect());
        *this->camParams.operator->() = cps;
        // now avoid resetting the camera by the initialisation
        if (!this->bboxs.IsWorldSpaceBBoxValid()) {
            this->bboxs.SetWorldSpaceBBox(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f);
        }

        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_INFO,
            "Camera parameters restored from \"%s\"",
            this->cameraSettingsSlot.FullName().PeekBuffer());
    } catch(vislib::Exception ex) {
        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_ERROR,
            "Cannot restore camera parameters from \"%s\": %s (%s; %d)",
            this->cameraSettingsSlot.FullName().PeekBuffer(),
            ex.GetMsgA(), ex.GetFile(), ex.GetLine());
    } catch(...) {
        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_ERROR,
            "Cannot restore camera parameters from \"%s\": unexpected exception",
            this->cameraSettingsSlot.FullName().PeekBuffer());
    }
    return true;
}


/*
 * view::View3D::onResetView
 */
bool view::View3D::onResetView(param::ParamSlot& p) {
    this->ResetView();
    return true;
}


#ifdef ENABLE_KEYBOARD_VIEW_CONTROL

/*
 * view::View3D::viewKeyPressed
 */
bool view::View3D::viewKeyPressed(param::ParamSlot& p) {

    if ((&p == &this->viewKeyRotLeftSlot)
            || (&p == &this->viewKeyRotRightSlot)
            || (&p == &this->viewKeyRotUpSlot)
            || (&p == &this->viewKeyRotDownSlot)
            || (&p == &this->viewKeyRollLeftSlot)
            || (&p == &this->viewKeyRollRightSlot)) {
        // rotate
        float angle = vislib::math::AngleDeg2Rad(this->viewKeyAngleStepSlot.Param<param::FloatParam>()->Value());
        vislib::math::Quaternion<float> q;
        int ptIdx = this->viewKeyRotPointSlot.Param<param::EnumParam>()->Value();
        // ptIdx == 0 : Position
        // ptIdx == 1 : LookAt

        if (&p == &this->viewKeyRotLeftSlot) {
            q.Set(angle, this->camParams->Up());
        } else if (&p == &this->viewKeyRotRightSlot) {
            q.Set(-angle, this->camParams->Up());
        } else if (&p == &this->viewKeyRotUpSlot) {
            q.Set(angle, this->camParams->Right());
        } else if (&p == &this->viewKeyRotDownSlot) {
            q.Set(-angle, this->camParams->Right());
        } else if (&p == &this->viewKeyRollLeftSlot) {
            q.Set(angle, this->camParams->Front());
        } else if (&p == &this->viewKeyRollRightSlot) {
            q.Set(-angle, this->camParams->Front());
        }

        vislib::math::Vector<float, 3> pos(this->camParams->Position().PeekCoordinates());
        vislib::math::Vector<float, 3> lat(this->camParams->LookAt().PeekCoordinates());
        vislib::math::Vector<float, 3> up(this->camParams->Up());

        if (ptIdx == 0) {
            lat -= pos;
            lat = q * lat;
            up = q * up;
            lat += pos;

        } else if (ptIdx == 1) {
            pos -= lat;
            pos = q * pos;
            up = q * up;
            pos += lat;
        }

        this->camParams->SetView(
            vislib::math::Point<float, 3>(pos.PeekComponents()),
            vislib::math::Point<float, 3>(lat.PeekComponents()),
            up);

    } else if ((&p == &this->viewKeyZoomInSlot)
            || (&p == &this->viewKeyZoomOutSlot)
            || (&p == &this->viewKeyMoveLeftSlot)
            || (&p == &this->viewKeyMoveRightSlot)
            || (&p == &this->viewKeyMoveUpSlot)
            || (&p == &this->viewKeyMoveDownSlot)) {
        // move
        float step = this->viewKeyMoveStepSlot.Param<param::FloatParam>()->Value();
        vislib::math::Vector<float, 3> move;

        if (&p == &this->viewKeyZoomInSlot) {
            move = this->camParams->Front();
            move *= step;
        } else if (&p == &this->viewKeyZoomOutSlot) {
            move = this->camParams->Front();
            move *= -step;
        } else if (&p == &this->viewKeyMoveLeftSlot) {
            move = this->camParams->Right();
            move *= -step;
        } else if (&p == &this->viewKeyMoveRightSlot) {
            move = this->camParams->Right();
            move *= step;
        } else if (&p == &this->viewKeyMoveUpSlot) {
            move = this->camParams->Up();
            move *= step;
        } else if (&p == &this->viewKeyMoveDownSlot) {
            move = this->camParams->Up();
            move *= -step;
        }

        this->camParams->SetView(
            this->camParams->Position() + move,
            this->camParams->LookAt() + move,
            vislib::math::Vector<float, 3>(this->camParams->Up()));

    }

    return true;
}

#endif /* ENABLE_KEYBOARD_VIEW_CONTROL */


/*
 * view::View3D::onAnimPlayChanged
 */
bool view::View3D::onAnimPlayChanged(param::ParamSlot& p) {
    ASSERT(&p == &this->animPlaySlot);

    if (this->animPlaySlot.Param<param::BoolParam>()->Value()) {
        double speed = static_cast<double>(this->animSpeedSlot.Param<param::FloatParam>()->Value());
        double offset = static_cast<double>(this->timeFrame)
            - this->GetCoreInstance()->GetInstanceTime() * speed;
        this->animOffsetSlot.Param<param::FloatParam>()->SetValue(static_cast<float>(offset));
    }

    return true;
}


/*
 * view::View3D::onAnimSpeedChanged
 */
bool view::View3D::onAnimSpeedChanged(param::ParamSlot& p) {
    ASSERT(&p == &this->animSpeedSlot);

    if (this->animPlaySlot.Param<param::BoolParam>()->Value()) {
        // assuming 'this->timeFrame' is up-to-date
        double speed = static_cast<double>(this->animSpeedSlot.Param<param::FloatParam>()->Value());
        double offset = static_cast<double>(this->timeFrame)
            - this->GetCoreInstance()->GetInstanceTime() * speed;
        this->animOffsetSlot.Param<param::FloatParam>()->SetValue(static_cast<float>(offset));
    }

    return true;
}


/*
 * view::View3D::onToggleButton
 */
bool view::View3D::onToggleButton(param::ParamSlot& p) {
    param::BoolParam *bp = NULL;

    if (&p == &this->toggleAnimPlaySlot) {
        bp = this->animPlaySlot.Param<param::BoolParam>();
    } else if (&p == &this->toggleSoftCursorSlot) {
        this->toggleSoftCurse();
        return true;
    } else if (&p == &this->toggleBBoxSlot) {
        bp = this->showBBox.Param<param::BoolParam>();
    }

    if (bp != NULL) {
        bp->SetValue(!bp->Value());
    }
    return true;
}


/*
 * view::View3D::onAnimSpeedStep
 */
bool view::View3D::onAnimSpeedStep(param::ParamSlot& p) {
    float spd = this->animSpeedSlot.Param<param::FloatParam>()->Value();
    bool spdset = false;
    if (&p == &this->animSpeedUpSlot) {
        if (spd >= 1.0f && spd < 100.0f) {
            spd += 0.25f;
        } else {
            spd += 0.01f;
            if (spd > 0.999999f) spd = 1.0f;
        }
        spdset = true;

    } else if (&p == &this->animSpeedDownSlot) {
        if (spd > 1.0f) {
            spd -= 0.25f;
        } else {
            if (spd > 0.01f) spd -= 0.01f;
        }
        spdset = true;

    }
    if (spdset) {
        this->animSpeedSlot.Param<param::FloatParam>()->SetValue(spd);
    }
    return true;
}
