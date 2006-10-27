/*
 * StereoCamTestApp.h
 *
 * Copyright (C) 2006 by Universitaet Stuttgart (VIS). Alle Rechte vorbehalten.
 */

#ifndef VISLIBTEST_STEREOCAMTESTAPP_H_INCLUDED
#define VISLIBTEST_STEREOCAMTESTAPP_H_INCLUDED
#if (_MSC_VER > 1000)
#pragma once
#endif /* (_MSC_VER > 1000) */

#include "AbstractGlutApp.h"

#include "vislib/types.h"
#include "vislib/Beholder.h"
#define VISLIB_ENABLE_OPENGL
#include "vislib/CameraOpenGL.h"

#define SCTA_CX_TILES 8
#define SCTA_CY_TILES 8

class StereoCamTestApp: public AbstractGlutApp {
public:
    StereoCamTestApp(void);
    virtual ~StereoCamTestApp(void);

    virtual int GLInit(void);

    virtual void Resize(unsigned int w, unsigned int h);
    virtual void Render(void);
    virtual bool KeyPress(unsigned char key, int x, int y);

private:
    void RenderTestBox(void);

    void UpdateCamTiles(void);

    float angle;
    UINT64 lastTime;

    vislib::graphics::Beholder beholder;
    vislib::graphics::gl::CameraOpenGL cameraLeft;
    vislib::graphics::gl::CameraOpenGL cameraRight;

    vislib::graphics::gl::CameraOpenGL camTilesLeft[SCTA_CX_TILES][SCTA_CY_TILES];
};

#endif /* VISLIBTEST_STEREOCAMTESTAPP_H_INCLUDED */
