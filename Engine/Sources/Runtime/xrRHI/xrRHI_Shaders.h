////////////////////////////////////////////////////////////////////////////////
// Created: 01.10.2026 20:18:56
// Author: NS_Deathman
// File: xrRHI_Shaders.h
// Nocturning studio for X-Platform
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include "xrRHI_Types.h"
#include "xrRHI_Handles.h"
#include <cstddef>
#include <string>
////////////////////////////////////////////////////////////////////////////////
// ============================================================================
// RHI_ShaderType
// ============================================================================
enum class RHI_ShaderType : uint32_t
{
	Vertex = 0,
	Pixel,
	// Задел: Geometry, Hull, Domain, Compute
};

// ============================================================================
// RHI_IncludeType
// ============================================================================
//
// Различает #include "..." и <...>. В D3D9 маппится в D3D_INCLUDE_LOCAL
// / D3D_INCLUDE_SYSTEM, в DXC — в аналогичную пару.
//
enum class RHI_IncludeType : uint32_t
{
	Local = 0, // #include "name"
	System,	   // #include <name>
};

// ============================================================================
// RHI_IncludeHandler
// ============================================================================
//
// Абстрактный include-хендлер. В D3D9 оборачивается в ID3DInclude внутри
// бэкенда, в D3D12 — в IDxcIncludeHandler.
//
// Контракт:
//   Open():
//     - type:       local vs system include
//     - name:       имя как оно написано в #include
//     - parentData: указатель на данные файла, в котором встречен include,
//                   либо nullptr для top-level source
//     - outData/outSize: буфер с содержимым. Должен оставаться валидным
//                        до вызова Close() с тем же указателем.
//     - возвращает true при успехе
//   Close():
//     - data: указатель, ранее возвращённый Open()
//
class XRRHI_API RHI_IncludeHandler
{
  public:
	virtual ~RHI_IncludeHandler() = default;

	virtual bool Open(RHI_IncludeType type, const char* name, const void* parentData, const void** outData, size_t* outSize) = 0;
	virtual void Close(const void* data) = 0;
};

// ============================================================================
// RHI_ShaderMacro
// ============================================================================
// null-terminated массив (name == nullptr — конец).
struct RHI_ShaderMacro
{
	const char* name = nullptr;
	const char* value = nullptr;
};

// ============================================================================
// RHI_ShaderCompileFlags
// ============================================================================
enum RHI_ShaderCompileFlags : uint32_t
{
	RHI_ShaderCompile_Debug = 1u << 0,
	RHI_ShaderCompile_SkipOptimize = 1u << 1,
	RHI_ShaderCompile_RowMajor = 1u << 2,	 // D3DCOMPILE_PACK_MATRIX_ROW_MAJOR
	RHI_ShaderCompile_ColumnMajor = 1u << 3, // D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR
	RHI_ShaderCompile_PreferFlowControl = 1u << 4,
	RHI_ShaderCompile_OptimizeLevel3 = 1u << 5,

	RHI_ShaderCompile_Default = RHI_ShaderCompile_RowMajor | 
								RHI_ShaderCompile_PreferFlowControl | 
								RHI_ShaderCompile_OptimizeLevel3,
};

// ============================================================================
// RHI_ShaderCompileDesc
// ============================================================================
struct RHI_ShaderCompileDesc
{
	const char* source = nullptr;
	size_t sourceSize = 0;
	const char* sourceName = nullptr; // для #line / debug
	const char* entryPoint = "main";
	RHI_ShaderType type = RHI_ShaderType::Vertex;
	const RHI_ShaderMacro* defines = nullptr; // null-terminated, опционально
	RHI_IncludeHandler* includeHandler = nullptr;
	uint32_t flags = RHI_ShaderCompile_Default;
	const char* debugName = nullptr;
};

// ============================================================================
// RHI_ShaderCompileResult
// ============================================================================
struct RHI_ShaderCompileResult
{
	RHI_ShaderHandle shader{};
	std::string errorMessage;

	bool IsValid() const { return shader.IsValid(); }
};
////////////////////////////////////////////////////////////////////////////////
