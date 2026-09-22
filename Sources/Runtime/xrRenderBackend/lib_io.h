#pragma once

#ifdef XRRENDERBACKEND_EXPORTS
#  define XRRB_API __declspec(dllexport)
#else
#  define XRRB_API __declspec(dllimport)
#endif

