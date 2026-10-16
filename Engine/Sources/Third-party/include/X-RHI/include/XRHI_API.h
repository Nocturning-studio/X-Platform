////////////////////////////////////////////////////////////////////////////////
// Author: NSDeathman
// Nocturning studio for X-RHI
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#ifdef XRHI_EXPORTS
#define XRHI_API __declspec(dllexport)
#else
#define XRHI_API __declspec(dllimport)
#endif

#define _QUOTE(x) #x
#define QUOTE(x) _QUOTE(x)

#define FILE_LINE_STR __FILE__ "(" QUOTE(__LINE__) ")"

#define DEPRECATED __pragma(message(                                           \
	FILE_LINE_STR "\n"                                                         \
				  " +-----------------------------------------------------+\n" \
				  " |                                                     |\n" \
				  " |   [DEPRECATED]  do not use                          |\n" \
				  " |                                                     |\n" \
				  " +-----------------------------------------------------+\n"))

#define DX_DEPRECATED __pragma(message(                                        \
	FILE_LINE_STR "\n"                                                         \
				  " +-----------------------------------------------------+\n" \
				  " |                                                     |\n" \
				  " |   [DEPRECATED]  do not use D3D - use RHI instead    |\n" \
				  " |                                                     |\n" \
				  " +-----------------------------------------------------+\n"))
////////////////////////////////////////////////////////////////////////////////
