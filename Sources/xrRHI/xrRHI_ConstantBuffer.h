#pragma once

#include "framework.h"
#include "xrRHI_Internal.h"
#include "xrRHI_BackendInterface.h"

RHI_BEGIN

class TypedConstantBuffer
{
public:
    TypedConstantBuffer() = default;

    void Create(IRenderBackend& backend, const ShaderConstantLayout& layout)
    {
        m_layout = layout;
        m_buffer = backend.CreateConstantBuffer(layout);
        // Строим маппинг имён на индекс для быстрого поиска
        m_nameToIndex.reserve(layout.fields.size());
        for (size_t i = 0; i < layout.fields.size(); ++i)
            m_nameToIndex[layout.fields[i].name] = i;
    }

    ConstantBufferHandle GetHandle() const { return m_buffer; }

    // Установить значение по имени (тип должен совпадать по размеру)
    template<typename T>
    void Set(IRenderBackend& backend, const char* name, const T& value)
    {
        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end()) return;
        const auto& field = m_layout.fields[it->second];
        if (sizeof(T) != field.size)
        {
            Print("! TypedConstantBuffer::Set: size mismatch for '%s'", name);
            return;
        }
        backend.UpdateConstantBuffer(m_buffer, field.offset, &value, sizeof(T));
    }

    void Set(IRenderBackend& backend, const char* name, float value)
    {
        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end()) return;
        const auto& field = m_layout.fields[it->second];
        if (field.cls == ConstantClass::Scalar && field.size == 16) {
            // скалярный регистр – записываем только первый компонент
            backend.UpdateConstantBuffer(m_buffer, field.offset, &value, sizeof(float));
        }
        else if (field.size == sizeof(float)) {
            backend.UpdateConstantBuffer(m_buffer, field.offset, &value, sizeof(float));
        }
        else {
            Print("! TypedConstantBuffer::Set<float>: size mismatch for '%s'", name);
        }
    }

    void Set(IRenderBackend& backend, const char* name, int value)
    {
        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end()) return;
        const auto& field = m_layout.fields[it->second];
        if (field.cls == ConstantClass::Scalar && field.size == 16) {
            float tmp = *reinterpret_cast<float*>(&value);   // побитовое копирование
            backend.UpdateConstantBuffer(m_buffer, field.offset, &tmp, sizeof(float));
        }
        else if (field.size == sizeof(int)) {
            // для поля точного размера (маловероятно в DX9, но пусть будет)
            backend.UpdateConstantBuffer(m_buffer, field.offset, &value, sizeof(int));
        }
        else {
            Print("! TypedConstantBuffer::Set<int>: size mismatch for '%s'", name);
        }
    }

    void Set(IRenderBackend& backend, const char* name, bool value)
    {
        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end()) return;
        const auto& field = m_layout.fields[it->second];
        if (field.cls == ConstantClass::Scalar && field.size == 16) {
            float fval = value ? 1.0f : 0.0f;
            backend.UpdateConstantBuffer(m_buffer, field.offset, &fval, sizeof(float));
        }
        else if (field.size == sizeof(bool)) {
            backend.UpdateConstantBuffer(m_buffer, field.offset, &value, sizeof(bool));
        }
        else {
            Print("! TypedConstantBuffer::Set<bool>: size mismatch for '%s'", name);
        }
    }

    // Специализации для матриц (передаём как fmat4x4 и т.д., размер должен совпадать)
    void SetMatrix4x4(IRenderBackend& backend, const char* name, const fmat4x4& mat)
    {
        Set(backend, name, mat); // sizeof(fmat4x4) == 64
    }

    void SetMatrix3x4(IRenderBackend& backend, const char* name, const fmat4x4& mat) // 3x4 хранится в 3 регистрах
    {
        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end()) return;
        const auto& field = m_layout.fields[it->second];
        if (field.size != 48) // 3 * 16
        {
            Print("! TypedConstantBuffer::SetMatrix3x4: size mismatch");
            return;
        }
        // Передаём только первые 3 float4
        backend.UpdateConstantBuffer(m_buffer, field.offset, &mat, 48);
    }

    void SetVector(IRenderBackend& backend, const char* name, const fvec4& vec)
    {
        Set(backend, name, vec);
    }

    // Установка массива векторов (например, для массивов констант)
    void SetArray(IRenderBackend& backend, const char* name, const void* data, u32 count)
    {
        auto it = m_nameToIndex.find(name);
        if (it == m_nameToIndex.end()) return;
        const auto& field = m_layout.fields[it->second];
        u32 totalSize = count * field.size; // field.size - размер одного элемента
        backend.UpdateConstantBuffer(m_buffer, field.offset, data, totalSize);
    }

    void Bind(IRenderBackend& backend, ShaderType stage) const
    {
        backend.SetShaderConstantBuffer(stage, m_layout.registerBase, m_buffer);
    }

private:
    ShaderConstantLayout m_layout;
    ConstantBufferHandle m_buffer;
    std::unordered_map<std::string, size_t> m_nameToIndex;
};

RHI_END
