/*
 * CharTraits.cpp
 *
 * Copyright (C) 2006 by Universitaet Stuttgart (VIS). Alle Rechte vorbehalten.
 */

#include "vislib/CharTraits.h"


/*
 * vislib::CharTraitsA::ParseBool
 */
bool vislib::CharTraitsA::ParseBool(const Char *str) {
    if (str == NULL) {
        throw IllegalParamException("str", __FILE__, __LINE__);
    }

    if (
#ifdef _WIN32
            (_stricmp("true", str) == 0) || (_stricmp("t", str) == 0) || 
            (_stricmp("yes", str) == 0) || (_stricmp("y", str) == 0) || 
            (_stricmp("on", str) == 0)
#else /* _WIN32 */
            (strcasecmp("true", str) == 0) || (strcasecmp("t", str) == 0) ||
            (strcasecmp("yes", str) == 0) || (strcasecmp("y", str) == 0) || 
            (strcasecmp("on", str) == 0)
#endif /* _WIN32 */
            ) {
        return true;
    }

    if (
#ifdef _WIN32
            (_stricmp("false", str) == 0) || (_stricmp("f", str) == 0) || 
            (_stricmp("no", str) == 0) || (_stricmp("n", str) == 0) || 
            (_stricmp("off", str) == 0)
#else /* _WIN32 */
            (strcasecmp("false", str) == 0) || (strcasecmp("f", str) == 0) || 
            (strcasecmp("no", str) == 0) || (strcasecmp("n", str) == 0) || 
            (strcasecmp("off", str) == 0)
#endif /* _WIN32 */
            ) {
        return false;
    }

    try {
        int i = ParseInt(str);
        return (i != 0);
    } catch (...) {
    }

    throw FormatException("Cannot convert String to Boolean", __FILE__, 
        __LINE__);
}


/*
 * vislib::CharTraitsA::ParseDouble
 */
double vislib::CharTraitsA::ParseDouble(const Char *str) {
    double retval;
    if (str == NULL) {
        throw IllegalParamException("str", __FILE__, __LINE__);
    }
    
    if (
#if (_MSC_VER >= 1400)
            sscanf_s
#else  /*(_MSC_VER >= 1400) */
            sscanf
#endif /*(_MSC_VER >= 1400) */
            (str, "%lf", &retval) != 1) {
        throw FormatException("Cannot convert String to Double", __FILE__, 
            __LINE__);
    }

    return retval;
}


/*
 * vislib::CharTraitsA::ParseInt
 */
int vislib::CharTraitsA::ParseInt(const Char *str) {
    int retval;
    if (str == NULL) {
        throw IllegalParamException("str", __FILE__, __LINE__);
    }

    if (
#if (_MSC_VER >= 1400)
            sscanf_s
#else  /*(_MSC_VER >= 1400) */
            sscanf
#endif /*(_MSC_VER >= 1400) */
            (str, "%d", &retval) != 1) {
        throw FormatException("Cannot convert String to Integer", __FILE__, 
            __LINE__);
    }
    
    return retval;
}


/*
 * vislib::CharTraitsA::Format
 */
vislib::CharTraitsA::Size vislib::CharTraitsA::Format(Char *dst, const Size cnt,
        const Char *fmt, va_list argptr) {
    int retval = -1;

#ifdef _WIN32
    if ((dst == NULL) || (cnt <= 0)) {
        /* Answer the prospective size of the string. */
        retval = ::_vscprintf(fmt, argptr);

    } else {
#if (_MSC_VER >= 1400)
        retval = ::_vsnprintf_s(dst, cnt, cnt, fmt, argptr);
#else /* (_MSC_VER >= 1400) */
        retval = ::_vsnprintf(dst, cnt, fmt, argptr);
#endif /* (_MSC_VER >= 1400) */
    } /* end if ((dst == NULL) || (cnt <= 0)) */

#else /* _WIN32 */
    retval = ::vsnprintf(dst, cnt, fmt, argptr);

    if ((dst != NULL) && (cnt > 0) && (retval > cnt - 1)) {
        retval = -1;
    }
#endif /* _WIN32 */ 

    /* Ensure string being terminated. */
    if ((dst != NULL) && (cnt > 0)) {
        dst[cnt - 1] = 0;
    }
    return static_cast<Size>(retval);
}


/*
 * vislib::CharTraitsA::ToLower
 */
vislib::CharTraitsA::Size vislib::CharTraitsA::ToLower(Char *dst, 
        const Size cnt, const Char *str) {
    // TODO: This implementation is a hack! Size might change if conversion
    // is performed correctly.
    ASSERT(str != NULL);
    Size retval = static_cast<Size>(::strlen(str));

#if (_MSC_VER >= 1400)
    if (::strncpy_s(dst, cnt, str, _TRUNCATE) != 0) {
        retval = -1;
    } else if (::_strlwr_s(dst, cnt) != 0) {
        retval = -1;
    }

#else /* (_MSC_VER >= 1400) */
    if (cnt >= retval + 1) {

#ifdef _WIN32
        ::strcpy(dst, str);
        ::_strlwr(dst);
#else /* _WIN32 */
        Char *d = dst;
        const Char *s = str;
        while ((*d++ = ToLower(*s++)) != static_cast<Char>(0));
#endif /* _WIN32 */
    } else {
        retval = -1;
    }
#endif /* (_MSC_VER >= 1400) */

    /* Ensure string being terminated. */
    if ((dst != NULL) && (cnt > 0)) {
        dst[cnt - 1] = 0;
    }
    return retval;
}


/*
 * vislib::CharTraitsA::ToUpper
 */
vislib::CharTraitsA::Size vislib::CharTraitsA::ToUpper(Char *dst, 
        const Size cnt, const Char *str) {
    // TODO: This implementation is a hack! Size might change if conversion
    // is performed correctly.
    ASSERT(str != NULL);
    Size retval = static_cast<Size>(::strlen(str));

#if (_MSC_VER >= 1400)
    if (::strncpy_s(dst, cnt, str, _TRUNCATE) != 0) {
        retval = -1;
    } else if (::_strupr_s(dst, cnt) != 0) {
        retval = -1;
    }

#else /* (_MSC_VER >= 1400) */
    if (cnt >= retval + 1) {

#ifdef _WIN32
        ::strcpy(dst, str);
        ::_strupr(dst);
#else /* _WIN32 */
        Char *d = dst;
        const Char *s = str;
        while ((*d++ = ToUpper(*s++)) != static_cast<Char>(0));
#endif /* _WIN32 */
    } else {
        retval = -1;
    }
#endif /* (_MSC_VER >= 1400) */

    /* Ensure string being terminated. */
    if ((dst != NULL) && (cnt > 0)) {
        dst[cnt - 1] = 0;
    }
    return retval;
}


////////////////////////////////////////////////////////////////////////////////


/*
 * vislib::CharTraitsW::ParseBool
 */
bool vislib::CharTraitsW::ParseBool(const Char *str) {
    if (str == NULL) {
        throw IllegalParamException("str", __FILE__, __LINE__);
    }

    if (
#ifdef _WIN32
            (_wcsicmp(L"true", str) == 0) || (_wcsicmp(L"t", str) == 0) || 
            (_wcsicmp(L"yes", str) == 0) || (_wcsicmp(L"y", str) == 0) || 
            (_wcsicmp(L"on", str) == 0)
#else /* _WIN32 */
            (wcscasecmp(L"true", str) == 0) || (wcscasecmp(L"t", str) == 0) || 
            (wcscasecmp(L"yes", str) == 0) || (wcscasecmp(L"y", str) == 0) || 
            (wcscasecmp(L"on", str) == 0)
#endif /* _WIN32 */
            ) {
        return true;
    }

    if (
#ifdef _WIN32
            (_wcsicmp(L"false", str) == 0) || (_wcsicmp(L"f", str) == 0) || 
            (_wcsicmp(L"no", str) == 0) || (_wcsicmp(L"n", str) == 0) || 
            (_wcsicmp(L"off", str) == 0)
#else /* _WIN32 */
            (wcscasecmp(L"false", str) == 0) || (wcscasecmp(L"f", str) == 0) || 
            (wcscasecmp(L"no", str) == 0) || (wcscasecmp(L"n", str) == 0) || 
            (wcscasecmp(L"off", str) == 0)
#endif /* _WIN32 */
            ) {
        return false;
        }

        try {
            int i = ParseInt(str);
            return (i != 0);
        } catch (...) {
        }

        throw FormatException("Cannot convert String to Boolean", __FILE__, 
            __LINE__);
    }


/*
 * vislib::CharTraitsW::ParseDouble
 */
double vislib::CharTraitsW::ParseDouble(const Char *str) {
    double retval;
    if (str == NULL) {
        throw IllegalParamException("str", __FILE__, __LINE__);
    }

    if (
#if (_MSC_VER >= 1400)
            swscanf_s
#else  /*(_MSC_VER >= 1400) */
            swscanf
#endif /*(_MSC_VER >= 1400) */
            (str, L"%lf", &retval) != 1) {
        throw FormatException("Cannot convert String to Double", __FILE__, 
            __LINE__);
    }

    return retval;
}


/*
 * vislib::CharTraitsW::ParseInt
 */
int vislib::CharTraitsW::ParseInt(const Char *str) {
    int retval;
    if (str == NULL) {
        throw IllegalParamException("str", __FILE__, __LINE__);
    }

    if (
#if (_MSC_VER >= 1400)
            swscanf_s
#else  /*(_MSC_VER >= 1400) */
            swscanf
#endif /*(_MSC_VER >= 1400) */
            (str, L"%d", &retval) != 1) {
        throw FormatException("Cannot convert String to Integer", __FILE__, 
            __LINE__);
    }
    
    return retval;
}


/*
 * vislib::CharTraitsW::Format
 */
vislib::CharTraitsW::Size vislib::CharTraitsW::Format(Char *dst, const Size cnt,
        const Char *fmt, va_list argptr) {
    int retval = -1;

#ifdef _WIN32
    if ((dst == NULL) || (cnt <= 0)) {
        /* Answer the prospective size of the string. */
        retval = ::_vscwprintf(fmt, argptr);

    } else {
#if (_MSC_VER >= 1400)
        retval = ::_vsnwprintf_s(dst, cnt, cnt, fmt, argptr);
#else /* (_MSC_VER >= 1400) */
        retval = ::_vsnwprintf(dst, cnt, fmt, argptr);
#endif /* (_MSC_VER >= 1400) */
    } /* end if ((dst == NULL) || (cnt <= 0)) */

#else /* _WIN32 */
    // Yes, you can trust your eyes: The char and wide char implementations
    // under Linux have a completely different semantics. vswprintf cannot 
    // be used for determining the required size as vsprintf can.
    SIZE_T bufferSize, bufferGrow;
    Char *buffer = NULL;

    if ((dst == NULL) || (cnt <= 0)) {
        /* Just count. */
        bufferSize = static_cast<SIZE_T>(1.1 * static_cast<float>(
            ::wcslen(fmt)) + 1);
        bufferGrow = static_cast<SIZE_T>(0.5 * static_cast<float>(
            bufferSize));
        buffer = new Char[bufferSize];

        while ((retval = ::vswprintf(buffer, bufferSize, fmt, argptr)) 
                == -1) {
            ARY_SAFE_DELETE(buffer);
            bufferSize += bufferGrow;
            buffer = new Char[bufferSize];
        }

        retval = ::wcslen(buffer);
        ARY_SAFE_DELETE(buffer);
            
    } else {
        /* Format the string. */
        retval = ::vswprintf(dst, cnt, fmt, argptr);
    }
#endif /* _WIN32 */

    /* Ensure string being terminated. */
    if ((dst != NULL) && (cnt > 0)) {
        dst[cnt - 1] = 0;
    }
    return static_cast<Size>(retval);
}


/*
* vislib::CharTraitsW::ToLower
 */
vislib::CharTraitsW::Size vislib::CharTraitsW::ToLower(Char *dst, 
        const Size cnt, const Char *str) {
    // TODO: This implementation is a hack! Size might change if conversion
    // is performed correctly.
    ASSERT(str != NULL);
    Size retval = static_cast<Size>(::wcslen(str));

#if (_MSC_VER >= 1400)
    if (::wcsncpy_s(dst, cnt, str, _TRUNCATE) != 0) {
        retval = -1;
    } else if (::_wcslwr_s(dst, cnt) != 0) {
        retval = -1;
    }

#else /* (_MSC_VER >= 1400) */
    if (cnt >= retval + 1) {

#ifdef _WIN32
        ::wcscpy(dst, str);
        ::_wcslwr(dst);
#else /* _WIN32 */
        Char *d = dst;
        const Char *s = str;
        while ((*d++ = ToLower(*s++)) != static_cast<Char>(0));
#endif /* _WIN32 */
    } else {
        retval = -1;
    }
#endif /* (_MSC_VER >= 1400) */

    /* Ensure string being terminated. */
    if ((dst != NULL) && (cnt > 0)) {
        dst[cnt - 1] = 0;
    }
    return retval;
}


/*
 * vislib::CharTraitsW::ToUpper
 */
vislib::CharTraitsW::Size vislib::CharTraitsW::ToUpper(Char *dst,
        const Size cnt, const Char *str) {
    // TODO: This implementation is a hack! Size might change if conversion
    // is performed correctly.
    ASSERT(str != NULL);
    Size retval = static_cast<Size>(::wcslen(str));

#if (_MSC_VER >= 1400)
    if (::wcsncpy_s(dst, cnt, str, _TRUNCATE) != 0) {
        retval = -1;
    } else if (::_wcsupr_s(dst, cnt) != 0) {
        retval = -1;
    }

#else /* (_MSC_VER >= 1400) */
    if (cnt >= retval + 1) {

#ifdef _WIN32
        ::wcscpy(dst, str);
        ::_wcsupr(dst);
#else /* _WIN32 */
        Char *d = dst;
        const Char *s = str;
        while ((*d++ = ToUpper(*s++)) != static_cast<Char>(0));
#endif /* _WIN32 */
    } else {
        retval = -1;
    }
#endif /* (_MSC_VER >= 1400) */

    /* Ensure string being terminated. */
    if ((dst != NULL) && (cnt > 0)) {
        dst[cnt - 1] = 0;
    }
    return retval;
}
