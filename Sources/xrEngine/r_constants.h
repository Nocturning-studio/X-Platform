#pragma once

#include "xr_resource.h"

class ENGINE_API R_constant_setup;

enum
{
    RC_float = 0,
    RC_int = 1,
    RC_bool = 2,
    RC_sampler = 99
};

enum
{
    RC_1x1 = 0,    // скаляр
    RC_1x4,         // вектор (float4)
    RC_2x4,         // 4x2 матрица (транспонированная)
    RC_3x4,         // 4x3 матрица
    RC_4x4,         // 4x4 матрица
    RC_1x4a,        // массив векторов
    RC_3x4a,        // массив 4x3 матриц
    RC_4x4a         // массив 4x4 матриц
};

enum
{
    RC_dest_pixel = (1 << 0),
    RC_dest_vertex = (1 << 1),
    RC_dest_sampler = (1 << 2)
};

struct R_constant_load
{
    u16 offset;         // раньше index – смещение в массиве констант (в float4)
    u16 size_class;     // раньше cls – класс размера (RC_1x4, RC_4x4 и т.д.)

    R_constant_load() : offset(u16(-1)), size_class(u16(-1)) {}

    IC BOOL equal(const R_constant_load& C) const
    {
        return (offset == C.offset) && (size_class == C.size_class);
    }
};

struct R_constant : public xr_resource
{
    shared_str         name;         // HLSL-имя
    u16                type;         // RC_float, RC_int, RC_bool, RC_sampler
    u16                destination;  // битовая маска RC_dest_*

    R_constant_load    ps;        // привязка для пиксельного шейдера
    R_constant_load    vs;        // привязка для вершинного шейдера
    R_constant_load    samp;      // привязка для семплера

    R_constant_setup* handler;      // автоматическая установка (для глобальных констант)

    R_constant() : type(u16(-1)), destination(0), handler(nullptr) {}

    IC BOOL equal(R_constant& C)
    {
        return (name == C.name) && 
               (type == C.type) && 
               (destination == C.destination) &&
               ps.equal(C.ps) && 
               vs.equal(C.vs) && 
               samp.equal(C.samp) && 
               handler == C.handler;
    }
    IC BOOL equal(R_constant* C) { return equal(*C); }
};

typedef resptr_core<R_constant, resptr_base<R_constant>> ref_constant;

class ENGINE_API R_constant_setup
{
public:
    virtual void setup(R_constant* C) = 0;
};

class ENGINE_API CShaderConstantLayout
{
public:
    struct ParamDesc
    {
        shared_str         name;
        u16                type;
        u16                destination;
        R_constant_load    ps;
        R_constant_load    vs;
        R_constant_load    samp;
        R_constant_setup* handler;
    };

    bool                LoadFromD3D9Bytecode(void* bytecode, u16 destination);

    const ParamDesc* FindParam(const char* name) const;
    const ParamDesc* FindParam(const shared_str& name) const;
    void                Merge(const CShaderConstantLayout& other);
    void                Clear();
    bool                Equal(const CShaderConstantLayout& other) const;
    bool                IsEmpty() const { return m_params.empty(); }

    const xr_vector<ParamDesc>& GetParams() const { return m_params; }

private:
    xr_vector<ParamDesc> m_params;
};

class ENGINE_API R_constant_table : public xr_resource_flagged
{
public:
    void                clear();
    BOOL                parse(void* desc, u16 destination);
    void                merge(R_constant_table* C);
    ref_constant        get(LPCSTR name);
    ref_constant        get(shared_str& name);
    BOOL                equal(R_constant_table& C);
    BOOL                equal(R_constant_table* C) { return equal(*C); }
    BOOL                empty() { return m_layout.IsEmpty(); }

    const CShaderConstantLayout& GetLayout() const { return m_layout; }

    ~R_constant_table();

private:
    CShaderConstantLayout m_layout;

public:
    typedef xr_vector<ref_constant> c_table;
    c_table table;
};

typedef resptr_core<R_constant_table, resptr_base<R_constant_table>> ref_ctable;
