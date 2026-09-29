////////////////////////////////////////////////////////////////////////////////
// Created: 29.09.2026
// Author: NSDeathman
// Nocturning studio for NS Platform X
////////////////////////////////////////////////////////////////////////////////
#pragma once
////////////////////////////////////////////////////////////////////////////////
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <xrRenderBackend/xrRenderBackendAPI.h>
////////////////////////////////////////////////////////////////////////////////

class XRRB_API CSharedResource
{
  public:
	CSharedResource() = default;
	virtual ~CSharedResource() = default;

	CSharedResource(const CSharedResource&) = delete;
	CSharedResource& operator=(const CSharedResource&) = delete;

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
	static_assert(std::is_base_of<CSharedResource, T>::value, "CSharedPtr<T>: T must inherit from CSharedResource");

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
