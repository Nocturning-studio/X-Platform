#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(default : 4995)

#include "ResourceManager.h"
#include "r_constants.h"

bool CShaderConstantLayout::LoadFromD3D9Bytecode(void* bytecode, u16 destination)
{
    D3DXSHADER_CONSTANTTABLE* desc = (D3DXSHADER_CONSTANTTABLE*)bytecode;
    if (!desc) return false;

    D3DXSHADER_CONSTANTINFO* it = (D3DXSHADER_CONSTANTINFO*)(LPBYTE(desc) + desc->ConstantInfo);
    LPBYTE ptr = LPBYTE(desc);

    for (u32 i = 0; i < desc->Constants; i++, it++)
    {
        LPCSTR name = LPCSTR(ptr + it->Name);

        // ќпредел€ем базовый тип
        u16 type = RC_float;
        if (D3DXRS_BOOL == it->RegisterSet) type = RC_bool;
        if (D3DXRS_INT4 == it->RegisterSet) type = RC_int;

        u16 r_index = it->RegisterIndex;
        u16 r_class = u16(-1);
        bool bSkip = false;

        D3DXSHADER_TYPEINFO* T = (D3DXSHADER_TYPEINFO*)(ptr + it->TypeInfo);
        switch (T->Class)
        {
        case D3DXPC_SCALAR:
            r_class = RC_1x1;
            break;
        case D3DXPC_VECTOR:
            r_class = RC_1x4;
            break;
        case D3DXPC_MATRIX_ROWS:
            switch (T->Columns)
            {
            case 4:
                switch (T->Rows)
                {
                case 3:
                    if (it->RegisterCount == 2) r_class = RC_2x4;
                    else if (it->RegisterCount == 3) r_class = RC_3x4;
                    else FATAL("MATRIX_ROWS: unsupported RegisterCount");
                    break;
                case 4:
                    r_class = RC_4x4;
                    VERIFY(it->RegisterCount == 4);
                    break;
                default: FATAL("MATRIX_ROWS: unsupported Rows");
                }
                break;
            default: FATAL("MATRIX_ROWS: unsupported Columns");
            }
            break;
        case D3DXPC_MATRIX_COLUMNS:
            FATAL("MATRIX_COLUMNS unsupported");
            break;
        case D3DXPC_STRUCT:
            FATAL("D3DXPC_STRUCT unsupported");
            break;
        case D3DXPC_OBJECT:
            if (T->Type >= D3DXPT_SAMPLER && T->Type <= D3DXPT_SAMPLERCUBE)
            {
                // Ёто сэмплер Ц обрабатываем отдельно
                ParamDesc sampParam;
                sampParam.name = name;
                sampParam.type = RC_sampler;
                sampParam.destination = RC_dest_sampler;
                sampParam.samp.offset = r_index + ((destination & 1) ? 0 : D3DVERTEXTEXTURESAMPLER0);
                sampParam.samp.size_class = RC_sampler;
                sampParam.handler = nullptr;

                // ѕровер€ем, нет ли уже такого
                bool exists = false;
                for (auto& p : m_params)
                {
                    if (p.name == name)
                    {
                        VERIFY(p.destination == RC_dest_sampler);
                        VERIFY(p.type == RC_sampler);
                        VERIFY(p.samp.offset == sampParam.samp.offset);
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                    m_params.push_back(sampParam);
            }
            else
            {
                FATAL("D3DXPC_OBJECT - not a sampler");
            }
            bSkip = true;
            break;
        default:
            bSkip = true;
            break;
        }

        if (bSkip) continue;

        // ќбычна€ константа
        ParamDesc param;
        param.name = name;
        param.type = type;
        param.destination = destination;
        param.handler = nullptr;

        R_constant_load& load = (destination & 1) ? param.ps : param.vs;
        load.offset = r_index;
        load.size_class = r_class;

        // »щем, есть ли уже така€ (могла быть объ€влена в другом шейдере)
        bool exists = false;
        for (auto& p : m_params)
        {
            if (p.name == name)
            {
                p.destination |= destination;
                VERIFY(p.type == type);
                R_constant_load& existingLoad = (destination & 1) ? p.ps : p.vs;
                existingLoad.offset = r_index;
                existingLoad.size_class = r_class;
                exists = true;
                break;
            }
        }
        if (!exists)
            m_params.push_back(param);
    }

    // —ортируем по имени (дл€ детерминированности)
    std::sort(m_params.begin(), m_params.end(), [](const ParamDesc& a, const ParamDesc& b) { return xr_strcmp(a.name.c_str(), b.name.c_str()) < 0; });

    return true;
}

const CShaderConstantLayout::ParamDesc* CShaderConstantLayout::FindParam(const char* name) const
{
    auto it = std::lower_bound(m_params.begin(), m_params.end(), name, [](const ParamDesc& p, const char* s) { return xr_strcmp(p.name.c_str(), s) < 0; });
    if (it != m_params.end() && xr_strcmp(it->name.c_str(), name) == 0)
        return &(*it);
    return nullptr;
}

const CShaderConstantLayout::ParamDesc* CShaderConstantLayout::FindParam(const shared_str& name) const
{
    for (auto& p : m_params)
        if (p.name.equal(name)) return &p;
    return nullptr;
}

void CShaderConstantLayout::Merge(const CShaderConstantLayout& other)
{
    for (const auto& src : other.m_params)
    {
        auto it = std::find_if(m_params.begin(), m_params.end(),
            [&](const ParamDesc& p) { return p.name == src.name; });
        if (it == m_params.end())
        {
            m_params.push_back(src);
        }
        else
        {
            it->destination |= src.destination;
            VERIFY(it->type == src.type);
            if (src.destination & RC_dest_pixel)  it->ps = src.ps;
            if (src.destination & RC_dest_vertex) it->vs = src.vs;
            if (src.destination & RC_dest_sampler) it->samp = src.samp;
        }
    }
    std::sort(m_params.begin(), m_params.end(), [](const ParamDesc& a, const ParamDesc& b) { return xr_strcmp(a.name.c_str(), b.name.c_str()) < 0; });
}

void CShaderConstantLayout::Clear()
{
    m_params.clear();
}

bool CShaderConstantLayout::Equal(const CShaderConstantLayout& other) const
{
    if (m_params.size() != other.m_params.size()) return false;
    for (size_t i = 0; i < m_params.size(); ++i)
    {
        const auto& a = m_params[i];
        const auto& b = other.m_params[i];
        if (a.name != b.name || a.type != b.type || a.destination != b.destination ||
            !a.ps.equal(b.ps) || !a.vs.equal(b.vs) || !a.samp.equal(b.samp) || a.handler != b.handler)
            return false;
    }
    return true;
}

R_constant_table::~R_constant_table()
{
    Engine.ResourceManager->_DeleteConstantTable(this);
}

void R_constant_table::clear()
{
    m_layout.Clear();
    table.clear();
}

BOOL R_constant_table::parse(void* desc, u16 destination)
{
    if (!m_layout.LoadFromD3D9Bytecode(desc, destination))
        return FALSE;

    table.clear();
    for (const auto& p : m_layout.GetParams())
    {
        ref_constant C = xr_new<R_constant>();
        C->name = p.name;
        C->type = p.type;
        C->destination = p.destination;
        C->ps = p.ps;
        C->vs = p.vs;
        C->samp = p.samp;
        C->handler = p.handler;
        table.push_back(C);
    }

    return TRUE;
}

void R_constant_table::merge(R_constant_table* T)
{
    if (!T) return;
    m_layout.Merge(T->m_layout);

    table.clear();
    for (const auto& p : m_layout.GetParams())
    {
        ref_constant C = xr_new<R_constant>();
        C->name = p.name;
        C->type = p.type;
        C->destination = p.destination;
        C->ps = p.ps;
        C->vs = p.vs;
        C->samp = p.samp;
        C->handler = p.handler;
        table.push_back(C);
    }
}

ref_constant R_constant_table::get(LPCSTR name)
{
    auto* p = m_layout.FindParam(name);
    if (!p) return nullptr;

    for (auto& C : table)
        if (C->name == name) return C;
    return nullptr;
}

ref_constant R_constant_table::get(shared_str& name)
{
    auto* p = m_layout.FindParam(name);
    if (!p) return nullptr;
    for (auto& C : table)
        if (C->name.equal(name)) return C;
    return nullptr;
}

BOOL R_constant_table::equal(R_constant_table& C)
{
    return m_layout.Equal(C.m_layout);
}
