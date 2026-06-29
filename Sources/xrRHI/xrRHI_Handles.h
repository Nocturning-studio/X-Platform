#pragma once
#include "framework.h"
#include "xrRHI_Internal.h"

RHI_BEGIN

constexpr u32 InvalidHandleId = 0xFFFFFFFF;

struct TextureHandle
{
    u32 id = InvalidHandleId;
    bool IsValid() const { return id != InvalidHandleId; }
    bool operator==(const TextureHandle& other) const { return id == other.id; }
};

struct SamplerHandle
{
    u32 id = InvalidHandleId;
    bool IsValid() const { return id != InvalidHandleId; }
    bool operator==(const SamplerHandle& other) const { return id == other.id; }
};

RHI_END
