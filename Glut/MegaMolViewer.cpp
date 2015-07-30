/*
 * MegaMolViewer.cpp
 *
 * Copyright (C) 2008 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include "MegaMolViewer.h"
#include "ApiHandle.h"
#include "CallbackSlot.h"
#include "Viewer.h"
#include "versioninfo.h"
#include "Window.h"
#include "vislib/memutils.h"
#include "vislib/String.h"
#include "vislib/sys/ThreadSafeStackTrace.h"


#ifdef _WIN32
/* windows dll entry point */
#ifdef _MANAGED
#pragma managed(push, off)
#endif /* _MANAGED */

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif /* _MANAGED */

#else /* _WIN32 */
/* linux shared object main */

#include "stdlib.h"
#include "stdio.h"

extern "C" {

const char interp[] __attribute__((section(".interp"))) = 
"/lib/ld-linux.so.2";

void mmCoreMain(int argc, char *argv[]) {
    printf("Horst!\n");
    //printf("argc = %i (%u)\nargv = %p\n", argc, argc, argv);
    //printf("*argv = %s\n", *argv);
    exit(0);
}

}

#endif /* _WIN32 */


/*
 * mmvGetVersionInfo
 */
MEGAMOLVIEWER_API mmcBinaryVersionInfo* MEGAMOLVIEWER_CALL(mmvGetVersionInfo)(void) {
    mmcBinaryVersionInfo* rv = static_cast<mmcBinaryVersionInfo*>(malloc(sizeof(mmcBinaryVersionInfo)));

    rv->VersionNumber[0] = MEGAMOL_GLUT_MAJOR_VER;
    rv->VersionNumber[1] = MEGAMOL_GLUT_MINOR_VER;
    rv->VersionNumber[2] = MEGAMOL_GLUT_MAJOR_REV;
    rv->VersionNumber[3] = MEGAMOL_GLUT_MINOR_REV;

    rv->SystemType = MMC_OSYSTEM_UNKNOWN;
#ifdef _WIN32
#if defined(WINVER)
#if (WINVER >= 0x0501)
    rv->SystemType = MMC_OSYSTEM_WINDOWS;
#endif /* (WINVER >= 0x0501) */
#endif /* defined(WINVER) */
#else /* _WIN32 */
    rv->SystemType = MMC_OSYSTEM_LINUX;
#endif /* _WIN32 */

    rv->HardwareArchitecture = MMC_HARCH_UNKNOWN;
#if defined(_WIN64) || defined(_LIN64)
    rv->HardwareArchitecture = MMC_HARCH_X64;
#else /* defined(_WIN64) || defined(_LIN64) */
    rv->HardwareArchitecture = MMC_HARCH_I86;
#endif /* defined(_WIN64) || defined(_LIN64) */

    rv->Flags = 0
#if defined(_DEBUG) || defined(DEBUG)
        | MMV_BFLAG_DEBUG
#endif /* defined(_DEBUG) || defined(DEBUG) */
#ifdef MEGAMOL_GLUT_ISDIRTY
        | MMV_BFLAG_DIRTY
#endif /* MEGAMOL_GLUT_ISDIRTY */
        ;

    const char *src = MEGAMOL_GLUT_NAME;
    size_t buf_len = ::strlen(src);
    char *buf = static_cast<char*>(::malloc(buf_len + 1));
    ::memcpy(buf, src, buf_len);
    buf[buf_len] = 0;
    rv->NameStr = buf;

    src = MEGAMOL_GLUT_COPYRIGHT;
    buf_len = ::strlen(src);
    buf = static_cast<char*>(::malloc(buf_len + 1));
    ::memcpy(buf, src, buf_len);
    buf[buf_len] = 0;
    rv->CopyrightStr = buf;

    src = MEGAMOL_GLUT_COMMENTS;
    buf_len = ::strlen(src);
    buf = static_cast<char*>(::malloc(buf_len + 1));
    ::memcpy(buf, src, buf_len);
    buf[buf_len] = 0;
    rv->CommentStr = buf;

    return rv;
}


/*
 * mmvFreeVersionInfo
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvFreeVersionInfo)(mmcBinaryVersionInfo* info) {
    if (info == nullptr) return;
    if (info->NameStr != nullptr) {
        ::free(const_cast<char*>(info->NameStr));
        info->NameStr = nullptr;
    }
    if (info->CopyrightStr != nullptr) {
        ::free(const_cast<char*>(info->CopyrightStr));
        info->CopyrightStr = nullptr;
    }
    if (info->CommentStr != nullptr) {
        ::free(const_cast<char*>(info->CommentStr));
        info->CommentStr = nullptr;
    }
    ::free(info);
}


/*
 * mmvGetHandleSize
 */
MEGAMOLVIEWER_API unsigned int MEGAMOLVIEWER_CALL(mmvGetHandleSize)(void) {
    VLSTACKTRACE("mmvGetHandleSize", __FILE__, __LINE__);
    return megamol::viewer::ApiHandle::GetHandleSize();
}


/*
 * mmvDisposeHandle
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvDisposeHandle)(void *hndl) {
    VLSTACKTRACE("mmvDisposeHandle", __FILE__, __LINE__);
    megamol::viewer::ApiHandle::DestroyHandle(hndl);
}


/*
 * mmvIsHandleValid
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvIsHandleValid)(void *hndl) {
    VLSTACKTRACE("mmvIsHandleValid", __FILE__, __LINE__);
    return (megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::ApiHandle>(hndl) != NULL);
}


/*
 * mmvGetHandleType
 */
MEGAMOLVIEWER_API mmvHandleType MEGAMOLVIEWER_CALL(mmvGetHandleType)(void *hndl) {
    VLSTACKTRACE("mmvGetHandleType", __FILE__, __LINE__);

    if (megamol::viewer::ApiHandle::InterpretHandle<
            megamol::viewer::ApiHandle>(hndl) == NULL) {
        return MMV_HTYPE_INVALID;

    } else if (megamol::viewer::ApiHandle::InterpretHandle<
            megamol::viewer::Viewer>(hndl) == NULL) {
        return MMV_HTYPE_VIEWCOREINSTANCE;

    } else {
        return MMV_HTYPE_UNKNOWN;
    }
}


/*
 * mmvCreateViewerHandle
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvCreateViewerHandle)(void *hView, unsigned int hints) {
    VLSTACKTRACE("mmvCreateViewerHandle", __FILE__, __LINE__);

    if (mmvIsHandleValid(hView) != 0) {
        return false; // handle was already valid.
    }
    if (*static_cast<unsigned char*>(hView) != 0) {
        return false; // memory pointer seams to be invalid.
    }

    megamol::viewer::Viewer *viewer = new megamol::viewer::Viewer();
    if (viewer == NULL) {
        return false; // out of memory or initialisation failed.
    }
    if (!viewer->Initialise(hints)) {
        delete viewer;
        return false; // initialisation failed.
    }

    return megamol::viewer::ApiHandle::CreateHandle(hView, viewer);
}


/*
 * mmvInitStackTracer
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvInitStackTracer)(void *stm) {
    VLSTACKTRACE("mmvInitStackTracer", __FILE__, __LINE__);

    vislib::sys::ThreadSafeStackTrace::Initialise(
        *static_cast<const vislib::SmartPtr<vislib::StackTrace>*>(stm), true);
}


/*
 * mmvProcessEvents
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvProcessEvents)(void *hView) {
    VLSTACKTRACE("mmvProcessEvents", __FILE__, __LINE__);

    megamol::viewer::Viewer *viewer 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Viewer>(hView);
    if (viewer == NULL) return false;

    return viewer->ProcessEvents();
}


/*
 * mmvCreateWindow
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvCreateWindow)(void *hView,
        void *hWnd) {
    VLSTACKTRACE("mmvCreateWindow", __FILE__, __LINE__);

    megamol::viewer::Viewer *viewer 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Viewer>(hView);
    if (viewer == NULL) return false;

    megamol::viewer::Window *win
        = new megamol::viewer::Window(*viewer);
    if (win == NULL) return false;

    if (megamol::viewer::ApiHandle::CreateHandle(hWnd, win)) {
        viewer->OwnWindow(win);
        return true;
    }
    // DO NOT DELET win. Has already be deleted by 'CreateHandle' as sfx
    return false;
}


/*
 * mmvSetUserData
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvSetUserData)(void *hndl,
        void *userData) {
    VLSTACKTRACE("mmvSetUserData", __FILE__, __LINE__);

    megamol::viewer::ApiHandle *obj 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::ApiHandle>(hndl);
    if (obj != NULL) {
        obj->UserData = userData;
    }
}


/*
 * mmvGetUserData
 */
MEGAMOLVIEWER_API void * MEGAMOLVIEWER_CALL(mmvGetUserData)(void *hndl) {
    VLSTACKTRACE("mmvGetUserData", __FILE__, __LINE__);

    megamol::viewer::ApiHandle *obj 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::ApiHandle>(hndl);
    return (obj != NULL) ? obj->UserData : NULL;
}


/*
 * mmvRegisterWindowCallback
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvRegisterWindowCallback)(
        void *hWnd, mmvWindowCallbacks slot, mmvCallback function) {
    VLSTACKTRACE("mmvRegisterWindowCallback", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;

    megamol::viewer::CallbackSlot *cbs = win->Callback(slot);
    if (cbs != NULL) {
        cbs->Register(function);
    }
}


/*
 * mmvUnregisterWindowCallback
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvUnregisterWindowCallback)(
        void *hWnd, mmvWindowCallbacks slot, mmvCallback function) {
    VLSTACKTRACE("mmvUnregisterWindowCallback", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;

    megamol::viewer::CallbackSlot *cbs = win->Callback(slot);
    if (cbs != NULL) {
        cbs->Unregister(function);
    }
}


/*
 * mmvSupportContextMenu
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvSupportContextMenu)(void *hWnd){
    VLSTACKTRACE("mmvSupportContextMenu", __FILE__, __LINE__);
    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    return (win != NULL);
}


/*
 * mmvInstallContextMenu
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvInstallContextMenu)(void *hWnd) {
    VLSTACKTRACE("mmvInstallContextMenu", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return false;
    win->InstallContextMenu();
    return true;
}


/*
 * mmvInstallContextMenuCommandA
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvInstallContextMenuCommandA)(
        void *hWnd, const char *caption, int value) {
    VLSTACKTRACE("mmvInstallContextMenuCommandA", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;
    win->AddCommand(caption, value);
}


/*
 * mmvInstallContextMenuCommandW
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvInstallContextMenuCommandW)(
        void *hWnd, const wchar_t *caption, int value) {
    VLSTACKTRACE("mmvInstallContextMenuCommandW", __FILE__, __LINE__);

    mmvInstallContextMenuCommandA(hWnd, vislib::StringA(caption).PeekBuffer(),
        value);
}


/*
 * mmvSupportParameterGUI
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvSupportParameterGUI)(void *hWnd) {
    VLSTACKTRACE("mmvSupportParameterGUI", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return false;

    // TODO: Implement

    return false;
}


/*
 * mmvInstallParameterGUI
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvInstallParameterGUI)(void *hWnd) {
    VLSTACKTRACE("mmvInstallParameterGUI", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return false;

    // TODO: Implement

    return false;
}


/*
 * mmvInstallParameter
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvInstallParameter)(void *hWnd,
        void *paramID, const unsigned char *desc, unsigned int len) {
    VLSTACKTRACE("mmvInstallParameter", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return false;

    // TODO: Implement

    return false;
}


/*
 * mmvRemoveParameter
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvRemoveParameter)(void *hWnd,
        void *paramID) {
    VLSTACKTRACE("mmvRemoveParameter", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;

    // TODO: Implement

}


/*
 * mmvSetParameterValueA
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvSetParameterValueA)(void *hWnd,
        void *paramID, const char *value) {
    VLSTACKTRACE("mmvSetParameterValueA", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return false;

    // TODO: Implement

    return false;
}


/*
 * mmvSetParameterValueW
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvSetParameterValueW)(void *hWnd,
        void *paramID, const wchar_t *value) {
    VLSTACKTRACE("mmvSetParameterValueW", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return false;

    // TODO: Implement

    return false;
}


/*
 * mmvRegisterParameterCallback
 */
MEGAMOLVIEWER_API bool MEGAMOLVIEWER_CALL(mmvRegisterParameterCallback)(
        void *hWnd, void *paramID, mmvCallback function) {
    VLSTACKTRACE("mmvRegisterParameterCallback", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return false;

    // TODO: Implement

    return false;
}


/*
 * mmvUnregisterParameterCallback
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvUnregisterParameterCallback)(
        void *hWnd, void *paramID) {
    VLSTACKTRACE("mmvUnregisterParameterCallback", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;

    // TODO: Implement

}


/*
 * mmvSetWindowSize
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvSetWindowSize)(void *hWnd,
        unsigned int width, unsigned int height) {
    VLSTACKTRACE("mmvSetWindowSize", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;
    win->ResizeWindow(width, height);
}


/*
 * mmvSetWindowTitleA
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvSetWindowTitleA)(void *hWnd,
        const char *title) {
    VLSTACKTRACE("mmvSetWindowTitleA", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win != NULL) win->SetTitle(title);
}


/*
 * mmvSetWindowTitleW
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvSetWindowTitleW)(void *hWnd,
        const wchar_t *title) {
    VLSTACKTRACE("mmvSetWindowTitleW", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win != NULL) win->SetTitle(title);
}


/*
 * mmvSetWindowPosition
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvSetWindowPosition)(void *hWnd,
        int x, int y) {
    VLSTACKTRACE("mmvSetWindowPosition", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;
    win->MoveWindowTo(x, y);
}


/*
 * mmvSetWindowFullscreen
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvSetWindowFullscreen)(void *hWnd) {
    VLSTACKTRACE("mmvSetWindowFullscreen", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;
    win->SetFullscreen();
}


/*
 * mmvSetWindowHints
 */
MEGAMOLVIEWER_API void MEGAMOLVIEWER_CALL(mmvSetWindowHints)(void *hWnd,
        unsigned int mask, unsigned int hints) {
    VLSTACKTRACE("mmvSetWindowHints", __FILE__, __LINE__);

    megamol::viewer::Window *win 
        = megamol::viewer::ApiHandle::InterpretHandle<
        megamol::viewer::Window>(hWnd);
    if (win == NULL) return;

    if ((mask & MMV_WINHINT_NODECORATIONS) != 0) win->ShowDecorations((hints & MMV_WINHINT_NODECORATIONS) == 0);
    if ((mask & MMV_WINHINT_HIDECURSOR) != 0) win->SetCursorVisibility((hints & MMV_WINHINT_HIDECURSOR) == 0);
    if ((mask & MMV_WINHINT_STAYONTOP) != 0) win->StayOnTop((hints & MMV_WINHINT_STAYONTOP) != 0);
    if ((mask & MMV_WINHINT_PRESENTATION) != 0) win->SetPresentationMode((hints & MMV_WINHINT_PRESENTATION) != 0);
    if ((mask & MMV_WINHINT_VSYNC) != 0) win->SetVSync((hints & MMV_WINHINT_VSYNC) != 0);
    if ((mask & MMV_WINHINT_PARAMGUI) != 0) win->ShowParameterGUI((hints & MMV_WINHINT_PARAMGUI) != 0);

}
