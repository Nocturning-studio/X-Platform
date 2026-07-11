/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once

#include <windows.h>
#include <vector>
#include <functional>
#include <memory>
#include <cassert>
#include <unordered_map>

#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
#include "TextureInterface.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

// ── Presentation ─────────────────────────────────────────────
enum class PresentationMode { Window, Console };

struct PresentParameters
{
    uint2 BackBufferSize = uint2(1, 1);
    HWND hDeviceWindow = nullptr;
    bool Windowed = true;
    bool Headless = false;
    PresentationMode Output = PresentationMode::Window;
    uint2 ConsoleSize = uint2(1, 1);
};

// ── Geometry data ───────────────────────────────────────────
struct VertexInput
{
    float3 Position;
    float3 Normal;
    float4 Color;
    float2 UV;

    VertexInput() : Position(0, 0, 0), Normal(0, 0, 0), Color(0, 0, 0, 0), UV(0, 0) {}
    VertexInput(const float3& pos, const float3& norm, const float4& col, const float2& uv = float2(0, 0))
        : Position(pos), Normal(norm), Color(col), UV(uv) {}
};

struct VertexOutput
{
    float4 Position;
    float4 Color;
    float3 Normal;
    float2 UV;

    VertexOutput() : Position(0, 0, 0, 0), Normal(0, 0, 0), Color(0, 0, 0, 0), UV(0, 0) {}
    VertexOutput(const float4& pos, const float3& norm, const float4& col, const float2& uv = float2(0, 0))
        : Position(pos), Normal(norm), Color(col), UV(uv) {}
};

// ── Buffers ──────────────────────────────────────────────────
class VertexBuffer
{
public:
    using VertexData = std::vector<VertexInput>;

    VertexBuffer() = default;
    explicit VertexBuffer(std::shared_ptr<const VertexData> data) : data(std::move(data)) {}
    explicit VertexBuffer(const VertexData& data) : data(std::make_shared<const VertexData>(data)) {}
    VertexBuffer(std::initializer_list<VertexInput> list) : data(std::make_shared<const VertexData>(list)) {}

    SOFTX_FORCE_INLINE size_t Size() const { return data ? data->size() : 0; }
    SOFTX_FORCE_INLINE bool IsEmpty() const { return !data || data->empty(); }

    SOFTX_FORCE_INLINE const VertexInput& GetByIndex(uint index) const
    {
        assert(data && index < data->size());
        return (*data)[index];
    }

    SOFTX_FORCE_INLINE const VertexData* operator->() const { return data.get(); }
    SOFTX_FORCE_INLINE const VertexData& operator*() const { return *data; }

private:
    std::shared_ptr<const VertexData> data;
};

class IndexBuffer
{
public:
    using IndexData = std::vector<uint>;

    IndexBuffer() = default;
    explicit IndexBuffer(std::shared_ptr<const IndexData> data) : data(std::move(data)) {}
    explicit IndexBuffer(const IndexData& data) : data(std::make_shared<const IndexData>(data)) {}
    IndexBuffer(std::initializer_list<uint> list) : data(std::make_shared<const IndexData>(list)) {}

    SOFTX_FORCE_INLINE size_t Size() const { return data ? data->size() : 0; }
    SOFTX_FORCE_INLINE bool IsEmpty() const { return !data || data->empty(); }

    SOFTX_FORCE_INLINE uint GetByIndex(uint index) const
    {
        assert(data && index < data->size());
        return (*data)[index];
    }

private:
    std::shared_ptr<const IndexData> data;
};

class ConstantBuffer
{
public:
    using CBufferData = std::vector<char>;

    ConstantBuffer() = default;
    explicit ConstantBuffer(std::shared_ptr<const CBufferData> data) : data(std::move(data)) {}
    ConstantBuffer(const void* srcData, size_t size)
        : data(std::make_shared<const CBufferData>(
            static_cast<const char*>(srcData),
            static_cast<const char*>(srcData) + size))
    {}

    SOFTX_FORCE_INLINE size_t Size() const { return data ? data->size() : 0; }
    SOFTX_FORCE_INLINE const void* Data() const { return data ? data->data() : nullptr; }

private:
    std::shared_ptr<const CBufferData> data;
};

// ── Viewport & Tile ──────────────────────────────────────────
struct Viewport
{
    float2 pos = float2(0.0f, 0.0f);
    int2 size = int2(0, 0);
    float minZ = 0.0f;
    float maxZ = 1.0f;

    Viewport() = default;
    Viewport(float x, float y, int width, int height, float minZ = 0, float maxZ = 1)
        : pos(x, y), size(width, height), minZ(minZ), maxZ(maxZ) {}
    Viewport(float2 pos, int2 size, float minZ = 0, float maxZ = 1)
        : pos(pos), size(size), minZ(minZ), maxZ(maxZ) {}
};

struct Tile
{
    uint2 min;
    uint2 max;
    std::vector<int> triangleIndices;

    Tile(uint2 min, uint2 max) : min(min), max(max) {}
};

// ── Sampler ──────────────────────────────────────────────────
enum class Filter { Nearest, Bilinear };
enum class Wrap { Repeat, Clamp, Mirror };
enum class MipFilter { Nearest, Linear };

struct SamplerState
{
    Filter minFilter = Filter::Bilinear;
    Filter magFilter = Filter::Bilinear;
    Filter filter = Filter::Bilinear;
    MipFilter mipFilter = MipFilter::Nearest;
    float minLOD = 0.0f;
    float maxLOD = 16.0f;
    float lodBias = 0.0f;
    Wrap wrapU = Wrap::Repeat;
    Wrap wrapV = Wrap::Repeat;

    float ApplyWrap(float uv, Wrap mode) const
    {
        switch (mode)
        {
        case Wrap::Clamp:  return AfterMath::clamp(uv, 0.0f, 1.0f);
        case Wrap::Mirror: {
            float t = std::fmod(std::abs(uv), 2.0f);
            return t > 1.0f ? 2.0f - t : t;
        }
        default: return uv - std::floor(uv);
        }
    }

    float2 ApplyWrap(float2 uv) const
    {
        return { ApplyWrap(uv.x, wrapU), ApplyWrap(uv.y, wrapV) };
    }
};

// ── Textures ─────────────────────────────────────────────────
struct TextureBinding
{
    std::shared_ptr<const ITexture> texture;
    SamplerState sampler;

    TextureBinding() = default;
    TextureBinding(std::shared_ptr<const ITexture> tex, SamplerState samp = SamplerState{})
        : texture(std::move(tex)), sampler(samp) {}

    SOFTX_FORCE_INLINE bool IsValid() const { return texture != nullptr; }
    SOFTX_FORCE_INLINE bool IsEmpty() const { return !IsValid(); }

    SOFTX_FORCE_INLINE void SetTexture(std::shared_ptr<const ITexture> tex) { texture = std::move(tex); }
    SOFTX_FORCE_INLINE void SetSamplerState(SamplerState samp) { sampler = samp; }

    float4 Sample(const float2& uv) const
    {
        if (!texture) return float4(1.0f, 0.0f, 1.0f, 1.0f); // magenta
        float2 wrapped = sampler.ApplyWrap(uv);
        return (sampler.minFilter == Filter::Bilinear)
            ? texture->SampleBilinear(wrapped)
            : texture->Sample(wrapped);
    }

    float4 SampleLevel(const float2& uv, const float& lod) const
    {
        if (!texture) return float4(1.0f, 0.0f, 1.0f, 1.0f);
        float2 wrapped = sampler.ApplyWrap(uv);
        float biasedLod = AfterMath::clamp(lod + sampler.lodBias, sampler.minLOD, sampler.maxLOD);
        if (sampler.mipFilter == MipFilter::Nearest)
            biasedLod = std::floor(biasedLod + 0.5f);
        return texture->SampleLevel(wrapped, biasedLod);
    }

    SOFTX_FORCE_INLINE uint2 GetDimensions() const
    {
        return uint2(texture->Width(), texture->Height());
    }
};

class TextureTable
{
public:
    SOFTX_FORCE_INLINE void Set(const std::string& name, std::shared_ptr<const ITexture> texture, SamplerState sampler = SamplerState{})
    {
        bindings[name] = { std::move(texture), sampler };
    }

    const TextureBinding& Get(const std::string& name) const
    {
        auto it = bindings.find(name);
        if (it != bindings.end()) return it->second;
        static const TextureBinding emptyBinding;
        return emptyBinding;
    }

    SOFTX_FORCE_INLINE float4 Sample(const std::string& name, const float2& uv) const { return Get(name).Sample(uv); }
    SOFTX_FORCE_INLINE float4 SampleLevel(const std::string& name, const float2& uv, const float& lod) const { return Get(name).SampleLevel(uv, lod); }
    SOFTX_FORCE_INLINE const TextureBinding& operator[](const std::string& name) const { return Get(name); }

    SOFTX_FORCE_INLINE void Remove(const std::string& name) { bindings.erase(name); }
    SOFTX_FORCE_INLINE void Clear() { bindings.clear(); }
    SOFTX_FORCE_INLINE bool Contains(const std::string& name) const { return bindings.find(name) != bindings.end(); }

private:
    std::unordered_map<std::string, TextureBinding> bindings;
};

// ── Shader types ─────────────────────────────────────────────
using PixelShader = std::function<float4(const VertexOutput&, const ConstantBuffer&, const TextureTable&)>;
using VertexShader = std::function<VertexOutput(const VertexInput&, const ConstantBuffer&, const TextureTable&)>;
using GeometryShader = std::function<void(const VertexOutput[3], std::vector<VertexOutput>&, std::vector<int>&, const TextureTable&)>;

// ── Rasterizer states ────────────────────────────────────────
enum class CullMode { None, Front, Back };
enum class FillMode { Point, Wireframe, Solid };
enum class ComparisonFunc { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };

SOFTX_END
/////////////////////////////////////////////////////////////////
