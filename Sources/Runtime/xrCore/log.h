#pragma once

#define VPUSH(a) a.x, a.y, a.z

void XRCORE_API __cdecl Msg(LPCSTR format, ...);
void XRCORE_API __cdecl DbgMsg(LPCSTR format, ...);
void XRCORE_API Log(LPCSTR msg);
void XRCORE_API Log(LPCSTR msg, LPCSTR dop);
void XRCORE_API Log(LPCSTR msg, u32 dop);
void XRCORE_API Log(LPCSTR msg, int dop);
void XRCORE_API Log(LPCSTR msg, float dop);
void XRCORE_API Log(LPCSTR msg, const fvec3& dop);
void XRCORE_API Log(LPCSTR msg, const fmat4x4& dop);
void XRCORE_API LogWinErr(LPCSTR msg, long err_code);

#ifndef PURE_LOG
typedef void (*LogCallback)(LPCSTR string);
void XRCORE_API SetLogCB(LogCallback cb);
void CreateLog(BOOL no_log = FALSE);
void InitLog();
void CloseLog();
void XRCORE_API FlushLog();

extern XRCORE_API xr_vector<shared_str>* LogFile;
#endif
