#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <xrRHI/xrRHI.h>
////////////////////////////////////////////////////////////////////////////////
class CRenderBackendFacade;
class R_transforms;
struct STextureList;
class CTextureLegacy;
class R_constant_table;
class CShaderPass;
struct IDirect3DStateBlock9;
struct IDirect3DPixelShader9;
struct IDirect3DVertexShader9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

class ENGINE_API CBackendResourceBinder
{
public:
	void Invalidate(CRenderBackendFacade& backend);

	void SetStates(CRenderBackendFacade& backend, IDirect3DStateBlock9* state);

	// ====================================================================
	// RHI-путь.
	// ====================================================================
	void SetPixelShader(CRenderBackendFacade& backend, RHI_ShaderHandle ps, LPCSTR name = nullptr);
	void SetVertexShader(CRenderBackendFacade& backend, RHI_ShaderHandle vs, LPCSTR name = nullptr);
	void SetShaderPass(CRenderBackendFacade& backend, CShaderPass* pass);
	void SetShaderPass(CRenderBackendFacade& backend, CShaderPass& pass) { SetShaderPass(backend, &pass); }

	void SetVertexDeclaration(CRenderBackendFacade& backend, RHI_InputLayoutHandle decl);
	void SetVertexBuffer(CRenderBackendFacade& backend, RHI_BufferHandle vb, u32 stride);
	void SetIndexBuffer(CRenderBackendFacade& backend, RHI_BufferHandle ib, RHI_IndexFormat fmt);

	void SetConstantTable(CRenderBackendFacade& backend, R_constant_table* ctable, R_transforms& transforms);
	IC R_constant_table* GetConstantTable() const { return m_ctable; }

	void SetTextures(CRenderBackendFacade& backend, STextureList* T);
	CTextureLegacy* GetActiveTexture(u32 stage) const;

	// ====================================================================
	// LEGACY D3D9-путь — coexistence bridge.
	// ====================================================================
	//
	// Принимает raw D3D9-указатели и вызывает device->SetX напрямую.
	// Нужен, чтобы движок продолжал работать, пока идёт миграция.
	//
	// Два кэша (RHI и legacy) взаимоисключающие: как только один из путей
	// меняет device-state, кэш второго инвалидируется. Это гарантирует,
	// что ни один путь не пропустит bind, сделанный другим путём.
	//
	// Удаляется после полной миграции вызывающего кода на RHI.
	void SetPixelShaderLegacy(CRenderBackendFacade& backend, IDirect3DPixelShader9* ps, LPCSTR name = nullptr);
	void SetVertexShaderLegacy(CRenderBackendFacade& backend, IDirect3DVertexShader9* vs, LPCSTR name = nullptr);
	void SetVertexDeclarationLegacy(CRenderBackendFacade& backend, IDirect3DVertexDeclaration9* decl);
	void SetVertexBufferLegacy(CRenderBackendFacade& backend, IDirect3DVertexBuffer9* vb, u32 stride);
	void SetIndexBufferLegacy(CRenderBackendFacade& backend, IDirect3DIndexBuffer9* ib);

private:
	// Legacy state block.
	IDirect3DStateBlock9* m_state = nullptr;

	// -----------------------------------------------------------------
	// RHI cache.
	// -----------------------------------------------------------------
	RHI_ShaderHandle      m_ps{};
	RHI_ShaderHandle      m_vs{};
	RHI_InputLayoutHandle m_decl{};
	RHI_BufferHandle      m_vb{};
	RHI_BufferHandle      m_ib{};
	RHI_IndexFormat       m_ibFormat = RHI_IndexFormat::UInt16;
	u32                   m_vbStride = 0;

	// -----------------------------------------------------------------
	// Legacy cache.
	// -----------------------------------------------------------------
	IDirect3DPixelShader9* m_psLegacy = nullptr;
	IDirect3DVertexShader9* m_vsLegacy = nullptr;
	IDirect3DVertexDeclaration9* m_declLegacy = nullptr;
	IDirect3DVertexBuffer9* m_vbLegacy = nullptr;
	IDirect3DIndexBuffer9* m_ibLegacy = nullptr;
	u32                          m_vbStrideLegacy = 0;

	// -----------------------------------------------------------------
	// Источник истины для device-state. Только один путь может быть
	// «владельцем» кэша одновременно.
	//
	//   Unknown — после Invalidate(), ни один кэш не валиден
	//   RHI     — device-state соответствует RHI-кэшу
	//   Legacy  — device-state соответствует legacy-кэшу
	//
	// Флаг общий: все bind'ы идут через один поток, и переключение путей
	// происходит редко (обычно раз в кадр), так что общий флаг — норма.
	// -----------------------------------------------------------------
	enum class EStateOwner : uint8_t { Unknown, RHI, Legacy };
	EStateOwner m_owner = EStateOwner::Unknown;

	R_constant_table* m_ctable = nullptr;
	STextureList* m_T = nullptr;

	CTextureLegacy* m_texturesPS[16] = {};
	CTextureLegacy* m_texturesVS[5] = {};

#ifdef DEBUG
	LPCSTR m_psName = nullptr;
	LPCSTR m_vsName = nullptr;
#endif
};
