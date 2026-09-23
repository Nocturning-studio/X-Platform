#pragma once

class CRenderBackendFacade;
class R_transforms;
struct STextureList;
class CTexture;
class R_constant_table;
class CShaderPass;
struct IDirect3DStateBlock9;
struct IDirect3DPixelShader9;
struct IDirect3DVertexShader9;
struct IDirect3DVertexDeclaration9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct IDirect3DDevice9Ex;

class ENGINE_API CBackendResourceBinder
{
  public:
	void Invalidate(CRenderBackendFacade& backend);

	// State block
	void SetStates(CRenderBackendFacade& backend, IDirect3DStateBlock9* state);

	// Shaders
	void SetPixelShader(CRenderBackendFacade& backend, IDirect3DPixelShader9* ps, LPCSTR name = nullptr);
	void SetVertexShader(CRenderBackendFacade& backend, IDirect3DVertexShader9* vs, LPCSTR name = nullptr);
	void SetShaderPass(CRenderBackendFacade& backend, CShaderPass* pass);
	void SetShaderPass(CRenderBackendFacade& backend, CShaderPass& pass) { SetShaderPass(backend, &pass); }

	// Vertex declaration & buffers
	void SetVertexDeclaration(CRenderBackendFacade& backend, IDirect3DVertexDeclaration9* decl);
	void SetVertexBuffer(CRenderBackendFacade& backend, IDirect3DVertexBuffer9* vb, u32 stride);
	void SetIndexBuffer(CRenderBackendFacade& backend, IDirect3DIndexBuffer9* ib);

	// Constant table (with handler setup)
	void SetConstantTable(CRenderBackendFacade& backend, R_constant_table* ctable, R_transforms& transforms);
	IC R_constant_table* GetConstantTable() const { return m_ctable; }

	// Textures
	void SetTextures(CRenderBackendFacade& backend, STextureList* T);

	// Helper for active texture
	CTexture* GetActiveTexture(u32 stage) const;

  private:
	IDirect3DStateBlock9* m_state = nullptr;
	IDirect3DPixelShader9* m_ps = nullptr;
	IDirect3DVertexShader9* m_vs = nullptr;
	IDirect3DVertexDeclaration9* m_decl = nullptr;
	IDirect3DVertexBuffer9* m_vb = nullptr;
	IDirect3DIndexBuffer9* m_ib = nullptr;
	u32 m_vbStride = 0;
	R_constant_table* m_ctable = nullptr;
	STextureList* m_T = nullptr;

	CTexture* m_texturesPS[16] = {};
	CTexture* m_texturesVS[5] = {};

#ifdef DEBUG
	LPCSTR m_psName = nullptr;
	LPCSTR m_vsName = nullptr;
#endif

	// Low‑level D3D9 wrappers
	void D3D_SetPixelShader(IDirect3DDevice9Ex* device, IDirect3DPixelShader9* ps);
	void D3D_SetVertexShader(IDirect3DDevice9Ex* device, IDirect3DVertexShader9* vs);
	void D3D_SetVertexDeclaration(IDirect3DDevice9Ex* device, IDirect3DVertexDeclaration9* decl);
	void D3D_SetStreamSource(IDirect3DDevice9Ex* device, u32 stream, IDirect3DVertexBuffer9* vb, u32 stride);
	void D3D_SetIndices(IDirect3DDevice9Ex* device, IDirect3DIndexBuffer9* ib);
	void D3D_SetTexture(IDirect3DDevice9Ex* device, u32 stage, CTexture* tex);
};
