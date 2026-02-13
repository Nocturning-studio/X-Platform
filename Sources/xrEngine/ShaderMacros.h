#ifndef SHADER_MACROS_INCLUDED
#define SHADER_MACROS_INCLUDED
#pragma once
#include "d3dx9.h"
#include "../xrCore/xrstring.h"
#include "../xrCore/math_types.h"

class ENGINE_API CShaderMacros
{
  public:
	enum State
	{
		Enable = 'e',
		Disable = 'd',
		Undef = 'u',
	};

	struct MacroImpl
	{
		shared_str Name;
		shared_str Definition;
		State State;
	};

  private:
	xr_vector<MacroImpl> macros_impl;
	std::string name;

	// Кэш для D3DX макросов с правильным управлением памятью
	mutable xr_vector<D3DXMACRO> d3dx_macros_cache;
	mutable xr_vector<char*> string_storage; // хранилище для строк

	MacroImpl* find(LPCSTR Name);
	void _safe_format_int(char* dest, size_t dest_size, int value);
	void _safe_format_float(char* dest, size_t dest_size, float value);
	void _safe_format_uint(char* dest, size_t dest_size, unsigned int value);
	void _clean_string(char* str);
	bool _is_ascii_printable(const char* str) const; // добавили const
	void _clear_string_storage() const;				 // добавили const

  public:
	CShaderMacros::CShaderMacros()
	{
	}

	CShaderMacros::~CShaderMacros()
	{
		_clear_string_storage();
	}

	// Основные методы
	void add(BOOL Enabled, LPCSTR Name, LPCSTR Definition);
	void add(LPCSTR Name, LPCSTR Definition);

	// Перегрузки для числовых значений
	void add(LPCSTR Name, int value);
	void add(LPCSTR Name, float value);
	void add(LPCSTR Name, u32 value);
	void add(LPCSTR Name, bool value);

	void add(BOOL Enabled, LPCSTR Name, int value);
	void add(BOOL Enabled, LPCSTR Name, float value);
	void add(BOOL Enabled, LPCSTR Name, u32 value);
	void add(BOOL Enabled, LPCSTR Name, bool value);

	void add(CShaderMacros& Macros);
	void undef(LPCSTR Name);
	void clear();

	xr_vector<MacroImpl>& get_impl();
	std::string& get_name();
	xr_vector<D3DXMACRO> get_macros() const;
	void dump_debug_info(LPCSTR context = NULL) const;
};
#endif
