/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <cassert>
#include <unordered_map>

#include "LibInternal.h"
#include "ThirdPartyIncluding.h"
#include "TextureInterface.h"
/////////////////////////////////////////////////////////////////
class SOFTX_API SoftX::IRenderTarget;
class SOFTX_API SoftX::DepthBuffer;
struct HWND__; typedef HWND__* HWND;
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

// ── Presentation ─────────────────────────────────────────────
enum class PresentationMode { Window, Console };

struct PresentParameters
{
    uint2 BackBufferSize = uint2(1, 1);
    uint2 ConsoleSize = uint2(1, 1);
    HWND hDeviceWindow = nullptr;
    PresentationMode Output = PresentationMode::Window;
    bool Windowed = true;
    bool Headless = false;

    void Validate() const
    {
        if (Headless) return;

        std::vector<std::string> errors;
        
        if (BackBufferSize.x == 0 || BackBufferSize.y == 0)
            errors.push_back("BackBufferSize must be > 0 in non-headless mode");

        switch (Output)
        {
        case PresentationMode::Window:
            if (hDeviceWindow == nullptr)
                errors.push_back("hDeviceWindow must be a valid window handle for Window output mode");
            break;

        case PresentationMode::Console:
            if (ConsoleSize.x == 0 || ConsoleSize.y == 0)
                errors.push_back("ConsoleSize must be > 0 for Console output mode");
            break;
        }

        if (!errors.empty())
        {
            std::string message = "PresentParameters validation failed: ";
            for (size_t i = 0; i < errors.size(); ++i)
            {
                if (i > 0) message += "; ";
                message += errors[i];
            }
            SOFTX_THROW(InvalidArgument(message));
        }
    }
};

// ── Geometry data ───────────────────────────────────────────
struct Vertex
{
    float4 Color;
    float3 Position;
    float3 Normal;
    float2 UV;

    Vertex() : Color(0, 0, 0, 0), Position(0, 0, 0), Normal(0, 0, 0), UV(0, 0) {}
    Vertex(float3 pos, float4 col = float4(0, 0, 0, 0), float3 norm = float3(0, 0, 0), float2 uv = float2(0, 0)) : Color(col), Position(pos), Normal(norm), UV(uv) {}
};

struct Interpolant
{
    float4 Position;
    float4 Color;
    float3 Normal;
    float2 UV;

    Interpolant() : Position(0, 0, 0, 0), Color(0, 0, 0, 0), Normal(0, 0, 0), UV(0, 0) {}
    Interpolant(float4 pos, float3 norm, float4 col, float2 uv = float2(0, 0)) : Position(pos), Color(col), Normal(norm), UV(uv) {}
};
using VertexOutput = Interpolant;

// ── Buffers ──────────────────────────────────────────────────
class VertexBuffer
{
public:
    using VertexData = std::vector<Vertex>;

    VertexBuffer() = default;
    explicit VertexBuffer(std::shared_ptr<VertexData> data) : data(std::move(data)) {}
    explicit VertexBuffer(const VertexData& data) : data(std::make_shared<VertexData>(data)) {}
    VertexBuffer(std::initializer_list<Vertex> list) : data(std::make_shared<VertexData>(list)) {}

    SOFTX_FORCE_INLINE size_t Size() const { return data ? data->size() : 0; }
    SOFTX_FORCE_INLINE bool IsEmpty() const { return !data || data->empty(); }

    SOFTX_FORCE_INLINE const Vertex& GetByIndex(uint index) const
    {
        assert(data && index < data->size());
        return (*data)[index];
    }

    void Add(const Vertex& vertex)
    {
        PrepareWrite();
        data->push_back(vertex);
    }

    void Add(Vertex&& vertex)
    {
        PrepareWrite();
        data->emplace_back(std::move(vertex));
    }

    template <typename... Args>
    void Add(Args&&... args)
    {
        PrepareWrite();
        data->emplace_back(std::forward<Args>(args)...);
    }

    void Reserve(size_t capacity)
    {
        PrepareWrite();
        data->reserve(capacity);
    }

    SOFTX_FORCE_INLINE const VertexData* operator->() const { return data.get(); }
    SOFTX_FORCE_INLINE const VertexData& operator*() const { return *data; }

private:
    void PrepareWrite()
    {
        if (!data) 
            data = std::make_shared<VertexData>();
        else if (data.use_count() > 1) 
            data = std::make_shared<VertexData>(*data);
    }

    std::shared_ptr<VertexData> data;
};

class IndexBuffer
{
public:
    using IndexData = std::vector<uint>;

    IndexBuffer() = default;
    explicit IndexBuffer(std::shared_ptr<IndexData> data) : data(std::move(data)) {}
    explicit IndexBuffer(const IndexData& data) : data(std::make_shared<IndexData>(data)) {}
    IndexBuffer(std::initializer_list<uint> list) : data(std::make_shared<IndexData>(list)) {}

    SOFTX_FORCE_INLINE size_t Size() const { return data ? data->size() : 0; }
    SOFTX_FORCE_INLINE bool IsEmpty() const { return !data || data->empty(); }

    SOFTX_FORCE_INLINE uint GetByIndex(uint index) const
    {
        assert(data && index < data->size());
        return (*data)[index];
    }

    void Add(uint index)
    {
        PrepareWrite();
        data->push_back(index);
    }

    void Add(std::initializer_list<uint> indices)
    {
        PrepareWrite();
        data->insert(data->end(), indices);
    }

    void AddTri(uint index0, uint index1, uint index2)
    {
        PrepareWrite();
        data->push_back(index0);
        data->push_back(index1);
        data->push_back(index2);
    }

    void Reserve(size_t capacity)
    {
        PrepareWrite();
        data->reserve(capacity);
    }

private:
    void PrepareWrite()
    {
        if (!data)
            data = std::make_shared<IndexData>();
        else if (data.use_count() > 1)
            data = std::make_shared<IndexData>(*data);
    }

    std::shared_ptr<IndexData> data;
};

class ConstantBuffer
{
public:
    using CBufferData = std::vector<char>;

    ConstantBuffer() = default;
    explicit ConstantBuffer(std::shared_ptr<const CBufferData> data) : data(std::move(data)) {}
    ConstantBuffer(const void* srcData, size_t size) : data(std::make_shared<const CBufferData>(
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
        case Wrap::Repeat: return uv - std::floor(uv);
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
    SOFTX_FORCE_INLINE void SetSamplerState(const SamplerState& samp) { sampler = samp; }

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
    SOFTX_FORCE_INLINE void Set(const std::string& name, std::shared_ptr<const ITexture> texture, const SamplerState& sampler = SamplerState{})
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
using PixelShader = std::function<float4(const Interpolant&, const ConstantBuffer&, const TextureTable&)>;
using VertexShader = std::function<Interpolant(const Vertex&, const ConstantBuffer&, const TextureTable&)>;
using GeometryShader = std::function<void(const Interpolant[3], std::vector<Interpolant>&, std::vector<int>&, const TextureTable&)>;

// ── Rasterizer states ────────────────────────────────────────
enum class CullMode { None, Front, Back };
enum class FillMode { Point, Wireframe, Solid };
enum class ComparisonFunc { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };

struct RasterizerState
{
    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = true;
};

// ── Pipeline resources ───────────────────────────────────────
enum class PipelineResource : uint32_t
{
    VertexShader = 1 << 0,
    PixelShader = 1 << 1,
    GeometryShader = 1 << 2,
    VertexBuffer = 1 << 3,
    IndexBuffer = 1 << 4,
    ConstantBuffer = 1 << 5,
    RenderTarget = 1 << 6,
    DepthBuffer = 1 << 7,
    Viewport = 1 << 8,
    TileSize = 1 << 9
};

constexpr uint32_t operator|(PipelineResource a, PipelineResource b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
constexpr uint32_t operator|(uint32_t a, PipelineResource b) { return a | static_cast<uint32_t>(b); }

// ── Pipeline state object ────────────────────────────────────
struct PipelineStateObject
{
    VertexShader vertexShader;
    GeometryShader geometryShader;
    PixelShader pixelShader;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    ConstantBuffer constantBuffer;

    TextureTable textureTable;

    std::shared_ptr<IRenderTarget> renderTarget;
    std::shared_ptr<DepthBuffer> depthBuffer;

    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = true;

    Viewport viewport;
    uint tileSize = 64;

    void Validate(uint32_t requiredResourcesMask) const 
    {
        std::string errors;
        auto check = [&](PipelineResource res, const char* name, bool present) 
        {
            if ((requiredResourcesMask & static_cast<uint32_t>(res)) && !present)
            {
                if (!errors.empty()) errors += "; ";
                errors += name;
            }
        };
        check(PipelineResource::VertexShader, "vertex shader", vertexShader != nullptr);
        check(PipelineResource::PixelShader, "pixel shader", pixelShader != nullptr);
        check(PipelineResource::GeometryShader, "geometry shader", geometryShader != nullptr);
        check(PipelineResource::VertexBuffer, "vertex buffer", !vertexBuffer.IsEmpty());
        check(PipelineResource::IndexBuffer, "index buffer", !indexBuffer.IsEmpty());
        check(PipelineResource::ConstantBuffer, "constant buffer", constantBuffer.Size() > 0);
        check(PipelineResource::RenderTarget, "render target", renderTarget != nullptr);
        check(PipelineResource::DepthBuffer, "depth buffer", depthBuffer != nullptr);
        check(PipelineResource::Viewport, "viewport", viewport.size.x > 0 && viewport.size.y > 0);
        check(PipelineResource::TileSize, "tile size > 0", tileSize > 0);

        if (!errors.empty())
            SOFTX_THROW(InvalidState("Missing required pipeline state: " + errors));
    }
};

struct OcclusionPipelineState
{
    using OcclusionVertexShader = std::function<Interpolant(const Vertex&, const ConstantBuffer&)>;

    VertexBuffer vertexBuffer;
    IndexBuffer  indexBuffer;
    ConstantBuffer constantBuffer;
    OcclusionVertexShader vertexShader;

    std::shared_ptr<DepthBuffer> depthBuffer;
    Viewport viewport;
    CullMode cullMode = CullMode::Back;
    ComparisonFunc depthFunc = ComparisonFunc::Less;
    bool depthWriteEnable = false;

    void Validate(uint32_t requiredResourcesMask) const
    {
        std::string errors;
        auto check = [&](PipelineResource res, const char* name, bool present)
        {
            if ((requiredResourcesMask & static_cast<uint32_t>(res)) && !present)
            {
                if (!errors.empty()) errors += "; ";
                errors += name;
            }
        };
        check(PipelineResource::VertexShader, "vertex shader", vertexShader != nullptr);
        check(PipelineResource::VertexBuffer, "vertex buffer", !vertexBuffer.IsEmpty());
        check(PipelineResource::IndexBuffer, "index buffer", !indexBuffer.IsEmpty());
        check(PipelineResource::ConstantBuffer, "constant buffer", constantBuffer.Size() > 0);
        check(PipelineResource::DepthBuffer, "depth buffer", depthBuffer != nullptr);
        check(PipelineResource::Viewport, "viewport", viewport.size.x > 0 && viewport.size.y > 0);

        if (!errors.empty())
            SOFTX_THROW(InvalidState("Missing required pipeline state: " + errors));
    }
};

// ── Clear target ───────────────────────────────────────
enum class ClearFlags : uint32_t 
{
    None = 0,
    RenderTarget = 1 << 0,
    DepthBuffer = 1 << 1,
    All = RenderTarget | DepthBuffer
};

constexpr ClearFlags operator|(ClearFlags a, ClearFlags b) { return static_cast<ClearFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }
constexpr ClearFlags operator&(ClearFlags a, ClearFlags b) { return static_cast<ClearFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b)); }
constexpr bool operator!(ClearFlags f) { return f == ClearFlags::None; }

SOFTX_END
/////////////////////////////////////////////////////////////////
