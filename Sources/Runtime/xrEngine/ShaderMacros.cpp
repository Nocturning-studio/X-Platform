#include "stdafx.h"
#include "ShaderMacros.h"

// Добавили const
void CShaderMacros::_clear_string_storage() const
{
	for (char* str : string_storage)
	{
		xr_free(str);
	}
	string_storage.clear();
	d3dx_macros_cache.clear();
}

// Вспомогательные методы форматирования (остаются без изменений)
void CShaderMacros::_safe_format_int(char* dest, size_t dest_size, int value)
{
	_snprintf(dest, dest_size, "%d", value);
	dest[dest_size - 1] = '\0';
}

void CShaderMacros::_safe_format_float(char* dest, size_t dest_size, float value)
{
	_snprintf(dest, dest_size, "%.8f", value);
	dest[dest_size - 1] = '\0';

	// Убираем trailing zeros
	char* dot = strchr(dest, '.');
	if (dot)
	{
		char* end = dest + strlen(dest) - 1;
		while (end > dot && *end == '0')
			*end-- = '\0';
		if (end == dot)
			*end = '\0';
	}
}

void CShaderMacros::_safe_format_uint(char* dest, size_t dest_size, unsigned int value)
{
	_snprintf(dest, dest_size, "%u", value);
	dest[dest_size - 1] = '\0';
}

void CShaderMacros::_clean_string(char* str)
{
	if (!str)
		return;

	char* src = str;
	char* dst = str;

	while (*src)
	{
		// Оставляем только ASCII печатные символы
		if (*src >= 32 && *src <= 126)
		{
			*dst++ = *src;
		}
		src++;
	}
	*dst = '\0';
}

// Добавили const
bool CShaderMacros::_is_ascii_printable(const char* str) const
{
	if (!str)
		return false;

	for (const char* p = str; *p; ++p)
	{
		if (*p < 32 || *p > 126)
			return false;
	}
	return true;
}

CShaderMacros::MacroImpl* CShaderMacros::find(LPCSTR Name)
{
	if (Name == NULL)
		return NULL;

	for (auto& it : macros_impl)
	{
		if (xr_strcmp(Name, it.Name.c_str()) == 0)
			return &it;
	}

	return NULL;
}

void CShaderMacros::add(BOOL Enabled, LPCSTR Name, LPCSTR Definition)
{
	if (!Name || !Definition)
		return;

	MacroImpl* pMacro = find(Name);

	if (pMacro)
	{
		pMacro->Definition = Definition;
		pMacro->State = Enabled ? Enable : Disable;
	}
	else
	{
		MacroImpl macro;
		macro.Name = Name;
		macro.Definition = Definition;
		macro.State = Enabled ? Enable : Disable;
		macros_impl.push_back(macro);
	}
}

void CShaderMacros::add(LPCSTR Name, LPCSTR Definition)
{
	add(TRUE, Name, Definition);
}

void CShaderMacros::add(LPCSTR Name, int value)
{
	if (!Name)
		return;

	string32 formatted;
	_safe_format_int(formatted, sizeof(formatted), value);
	_clean_string(formatted);

	if (_is_ascii_printable(formatted))
		add(Name, formatted);
	else
	{
		Msg("! CShaderMacros: Invalid int value for %s, using 0", Name);
		add(Name, "0");
	}
}

void CShaderMacros::add(LPCSTR Name, float value)
{
	if (!Name)
		return;

	string64 formatted;
	_safe_format_float(formatted, sizeof(formatted), value);
	_clean_string(formatted);

	if (_is_ascii_printable(formatted))
		add(Name, formatted);
	else
	{
		Msg("! CShaderMacros: Invalid float value for %s, using 0.0", Name);
		add(Name, "0.0");
	}
}

void CShaderMacros::add(LPCSTR Name, u32 value)
{
	if (!Name)
		return;

	string32 formatted;
	_safe_format_uint(formatted, sizeof(formatted), value);
	_clean_string(formatted);

	if (_is_ascii_printable(formatted))
		add(Name, formatted);
	else
	{
		Msg("! CShaderMacros: Invalid uint value for %s, using 0", Name);
		add(Name, "0");
	}
}

void CShaderMacros::add(LPCSTR Name, bool value)
{
	add(Name, value ? "1" : "0");
}

// Условные версии с Enabled
void CShaderMacros::add(BOOL Enabled, LPCSTR Name, int value)
{
	string32 formatted;
	_safe_format_int(formatted, sizeof(formatted), value);
	_clean_string(formatted);

	if (_is_ascii_printable(formatted))
		add(Enabled, Name, formatted);
	else
		add(Enabled, Name, "0");
}

void CShaderMacros::add(BOOL Enabled, LPCSTR Name, float value)
{
	string64 formatted;
	_safe_format_float(formatted, sizeof(formatted), value);
	_clean_string(formatted);

	if (_is_ascii_printable(formatted))
		add(Enabled, Name, formatted);
	else
		add(Enabled, Name, "0.0");
}

void CShaderMacros::add(BOOL Enabled, LPCSTR Name, u32 value)
{
	string32 formatted;
	_safe_format_uint(formatted, sizeof(formatted), value);
	_clean_string(formatted);

	if (_is_ascii_printable(formatted))
		add(Enabled, Name, formatted);
	else
		add(Enabled, Name, "0");
}

void CShaderMacros::add(BOOL Enabled, LPCSTR Name, bool value)
{
	add(Enabled, Name, value ? "1" : "0");
}

void CShaderMacros::add(CShaderMacros& Macros)
{
	for (auto& it : Macros.macros_impl)
	{
		MacroImpl* pMacro = find(it.Name.c_str());

		if (pMacro)
		{
			pMacro->Definition = it.Definition;
			pMacro->State = it.State;
		}
		else
		{
			macros_impl.push_back(it);
		}
	}
}

void CShaderMacros::undef(LPCSTR Name)
{
	MacroImpl* pMacro = find(Name);

	if (pMacro)
	{
		pMacro->State = Undef;
	}
}

void CShaderMacros::clear()
{
	macros_impl.clear();
}

xr_vector<CShaderMacros::MacroImpl>& CShaderMacros::get_impl()
{
	return macros_impl;
}

std::string& CShaderMacros::get_name()
{
	name.clear();

	for (auto& it : macros_impl)
	{
		// Проверяем что Definition валиден и не пуст
		LPCSTR def_str = it.Definition.c_str();
		if (def_str && def_str[0] != '\0')
		{
			if (it.State == Enable)
			{
				name += def_str;
			}
			else
			{
				// Используем длину строки
				size_t len = strlen(def_str);
				for (size_t j = 0; j < len; ++j)
					name += it.State;
			}
		}
	}

	return name;
}

xr_vector<D3DXMACRO> CShaderMacros::get_macros() const
{
	// Очищаем предыдущее хранилище
	_clear_string_storage();

	d3dx_macros_cache.clear();

	for (const auto& it : macros_impl)
	{
		LPCSTR name_str = it.Name.c_str();
		LPCSTR def_str = it.Definition.c_str();

		// Проверяем через размер или указатель
		if (it.State == Enable && name_str && name_str[0] != '\0' && def_str && def_str[0] != '\0')
		{
			// Дополнительная проверка на валидность строк
			if (!_is_ascii_printable(name_str) || !_is_ascii_printable(def_str))
			{
				Msg("! CShaderMacros: Skipping invalid macro: %s = %s", name_str, def_str);
				continue;
			}

			// СОЗДАЕМ КОПИИ строк с выделением памяти
			char* name_copy = xr_strdup(name_str);
			char* definition_copy = xr_strdup(def_str);

			// Сохраняем указатели для последующего освобождения
			string_storage.push_back(name_copy);
			string_storage.push_back(definition_copy);

			D3DXMACRO macro;
			macro.Name = name_copy;
			macro.Definition = definition_copy;

			d3dx_macros_cache.push_back(macro);
		}
	}

	// Добавляем терминальный NULL макрос
	D3DXMACRO terminator = {NULL, NULL};
	d3dx_macros_cache.push_back(terminator);

	return d3dx_macros_cache;
}

void CShaderMacros::dump_debug_info(LPCSTR context) const
{
	for (const auto& macro : macros_impl)
	{
		const char* state_str = "Unknown";
		switch (macro.State)
		{
		case Enable:
			state_str = "Enable";
			break;
		case Disable:
			state_str = "Disable";
			break;
		case Undef:
			state_str = "Undef";
			break;
		}

		LPCSTR name_str = macro.Name.c_str();
		LPCSTR def_str = macro.Definition.c_str();

		Msg("Macro: %s = %s [State: %s]", name_str ? name_str : "NULL", def_str ? def_str : "NULL", state_str);
	}
}
