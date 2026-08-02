#pragma once

#define BREAK_AT_STRCMP
#ifndef DEBUG
#undef BREAK_AT_STRCMP
#endif
#ifdef _EDITOR
#undef BREAK_AT_STRCMP
#endif

IC u32 xr_strlen(const char* S);

// return pointer to ".ext"
IC char* strext(const char* S)
{
	return (char*)strrchr(S, '.');
}

IC u32 xr_strlen(const char* S)
{
	return (u32)strlen(S);
}

IC char* xr_strlwr(char* S)
{
	if (S)
	{
		for (unsigned char* p = reinterpret_cast<unsigned char*>(S); *p; ++p)
			*p = static_cast<unsigned char>(tolower(*p));
	}
	return S;
}

inline int xr_stricmp(const char* str1, const char* str2)
{
#ifdef _MSC_VER
	return _stricmp(str1, str2);
#else
	return stricmp(str1, str2);
#endif
}

#ifdef BREAK_AT_STRCMP
XRCORE_API int xr_strcmp(const char* S1, const char* S2);
#else
IC int xr_strcmp(const char* S1, const char* S2)
{
	return (int)strcmp(S1, S2);
}
#endif

// token type definition
struct XRCORE_API xr_token
{
	LPCSTR name;
	int id;
};

IC LPCSTR get_token_name(xr_token* tokens, int key)
{
	for (int k = 0; tokens[k].name; k++)
		if (key == tokens[k].id)
			return tokens[k].name;
	return "";
}

IC int get_token_id(xr_token* tokens, LPCSTR key)
{
	for (int k = 0; tokens[k].name; k++)
		if (_stricmp(tokens[k].name, key) == 0)
			return tokens[k].id;
	return -1;
}

struct XRCORE_API xr_token2
{
	LPCSTR name;
	LPCSTR info;
	int id;
};

XRCORE_API char* timestamp(string64& dest);

extern XRCORE_API u32 crc32(const void* P, u32 len);
