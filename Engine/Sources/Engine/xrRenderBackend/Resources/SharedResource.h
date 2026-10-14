////////////////////////////////////////////////////////////////////////////////
//  X-Platform — расширение функциональности X-Ray Engine
//
//  Автор:        NSDeathman
//  Студия:       Nocturning Studio
//
//  SPDX-FileCopyrightText: 2026 Nocturning Studio и участники X-Platform
//  SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
//
//  Этот файл распространяется на условиях PolyForm Noncommercial 1.0.0.
//  Копирование, изменение и распространение разрешены для некоммерческих
//  целей. Публичное указание авторства в вашем проекте, документации или
//  рекламе не требуется — оно приветствуется, но не является условием
//  использования.
//
//  При распространении копий или производных работ лицензия обязывает
//  сохранять настоящие уведомления и передавать получателю текст лицензии
//  или ссылку на него. Пожалуйста, не удаляйте эту плашку — это помогает
//  сохранять историю проекта и корректно соблюдать лицензию.
//
//  Коммерческое использование без отдельного разрешения правообладателей
//  запрещено.
//
//  Полный текст лицензии:
//      https://polyformproject.org/licenses/noncommercial/1.0.0
//
//  Этот файл является source-available, а не open source.
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <xrRenderBackend/xrRenderBackendAPI.h>
////////////////////////////////////////////////////////////////////////////////

class XRRB_API CRefCountedResource
{
  public:
	CRefCountedResource() = default;
	virtual ~CRefCountedResource() = default;

	CRefCountedResource(const CRefCountedResource&) = delete;
	CRefCountedResource& operator=(const CRefCountedResource&) = delete;

	uint32_t AddRef() const noexcept { return ++m_refCount; }
	uint32_t Release() const noexcept { return --m_refCount; }
	uint32_t RefCount() const noexcept { return m_refCount.load(std::memory_order_relaxed); }

  private:
	mutable std::atomic<uint32_t> m_refCount{0};
};

////////////////////////////////////////////////////////////////////////////////

template <class T>
class CSharedPtr
{
	static_assert(std::is_base_of<CRefCountedResource, T>::value, "CSharedPtr<T>: T must inherit from CRefCountedResource");

  public:
	CSharedPtr() noexcept = default;
	CSharedPtr(std::nullptr_t) noexcept {}

	explicit CSharedPtr(T* p) noexcept : m_ptr(p)
	{
		if(m_ptr)
			m_ptr->AddRef();
	}

	CSharedPtr(const CSharedPtr& o) noexcept : m_ptr(o.m_ptr)
	{
		if(m_ptr)
			m_ptr->AddRef();
	}

	CSharedPtr(CSharedPtr&& o) noexcept : m_ptr(o.m_ptr) { o.m_ptr = nullptr; }

	~CSharedPtr() { Reset(); }

	CSharedPtr& operator=(const CSharedPtr& o) noexcept
	{
		if(this != &o)
			Attach(o.m_ptr);
		return *this;
	}

	CSharedPtr& operator=(CSharedPtr&& o) noexcept
	{
		if(this != &o)
		{
			Reset();
			m_ptr = o.m_ptr;
			o.m_ptr = nullptr;
		}
		return *this;
	}

	CSharedPtr& operator=(T* p) noexcept
	{
		if(m_ptr != p)
			Attach(p);
		return *this;
	}

	T* _get() const noexcept { return m_ptr; }
	T* operator->() const noexcept { return m_ptr; }
	T& operator*() const noexcept { return *m_ptr; }
	bool operator!() const noexcept { return m_ptr == nullptr; }
	explicit operator bool() const noexcept { return m_ptr != nullptr; }

	bool operator==(const CSharedPtr& o) const noexcept { return m_ptr == o.m_ptr; }
	bool operator!=(const CSharedPtr& o) const noexcept { return m_ptr != o.m_ptr; }
	bool operator==(std::nullptr_t) const noexcept { return m_ptr == nullptr; }
	bool operator!=(std::nullptr_t) const noexcept { return m_ptr != nullptr; }

	void Set(T* p) noexcept
	{
		if(m_ptr != p)
			Attach(p);
	}
	void Clear() noexcept { Reset(); }
	T* Release() noexcept
	{
		T* r = m_ptr;
		m_ptr = nullptr;
		return r;
	}

  private:
	void Attach(T* p) noexcept
	{
		if(p)
			p->AddRef();
		Reset();
		m_ptr = p;
	}

	void Reset() noexcept
	{
		if(m_ptr)
		{
			m_ptr->Release();
			m_ptr = nullptr;
		}
	}

	T* m_ptr = nullptr;
};
////////////////////////////////////////////////////////////////////////////////
