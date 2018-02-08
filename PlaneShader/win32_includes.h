// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#ifndef __PS_WIN32_INCLUDES_H__
#define __PS_WIN32_INCLUDES_H__
#pragma pack(push)
#pragma pack(8)
#ifdef WINXP
#define WINVER 0x0501 //_WIN32_WINNT_WINXP   
#define _WIN32_WINNT 0x0501
#define NTDDI_VERSION 0x05010300 //NTDDI_WINXPSP3 
#else
#define WINVER 0x0601 //_WIN32_WINNT_WIN7   
#define _WIN32_WINNT 0x0601
#define NTDDI_VERSION 0x06010000 //NTDDI_WIN7 
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX // Some compilers enable this by default
#define NOMINMAX
#endif
#define NODRAWTEXT
#define NOBITMAP
#define NOMCX
#define NOSERVICE
#define NOHELP
#include <windows.h>
#pragma pack(pop)
#endif