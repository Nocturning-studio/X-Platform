#ifndef vis_commonH
#define vis_commonH
#pragma once

#pragma pack(push, 4)
#include <atomic> // Не забудьте подключить, если еще нет

struct vis_data
{
	Fsphere sphere;						 //
	Fbox box;							 //
	std::atomic<u32> m_traversal_marker; // for different sub-renders
	u32 accept_frame;					 // when it was requisted accepted for main render
	u32 hom_frame;						 // when to perform test - shedule
	u32 hom_tested;						 // when it was last time tested

	IC void clear()
	{
		sphere.P.set(0, 0, 0);
		sphere.R = 0;
		box.invalidate();
		m_traversal_marker = 0;
		accept_frame = 0;
		hom_frame = 0;
		hom_tested = 0;
	}

	// Конструктор по умолчанию
	vis_data()
	{
		box.invalidate();
		sphere.P.set(0, 0, 0);
		sphere.R = 0;
		m_traversal_marker = 0;
		accept_frame = 0;
		hom_frame = 0;
		hom_tested = 0;
	}

	// Оператор присваивания
	vis_data& operator=(const vis_data& other)
	{
		if (this != &other)
		{
			// Копируем обычные данные
			box = other.box;
			sphere = other.sphere;
			accept_frame = other.accept_frame;
			hom_frame = other.hom_frame;
			hom_tested = other.hom_tested;

			// Атомарно копируем значение маркера
			m_traversal_marker.store(other.m_traversal_marker.load(std::memory_order_relaxed), std::memory_order_relaxed);
		}
		return *this;
	}

	// Конструктор копирования
	vis_data(const vis_data& other)
	{
		*this = other; // Используем оператор присваивания
	}
};
#pragma pack(pop)
#endif
