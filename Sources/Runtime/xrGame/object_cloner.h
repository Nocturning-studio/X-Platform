////////////////////////////////////////////////////////////////////////////
//	Module 		: object_cloner.h
//	Created 	: 13.07.2004
//  Modified 	: 13.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Object cloner
////////////////////////////////////////////////////////////////////////////

#pragma once

struct CCloner
{
	template <typename T> struct CHelper
	{
		template <bool a> IC static void clone(const T& obj_1, T& obj_2)
		{
			obj_2 = obj_1;
		}

		template <> IC static void clone<true>(const T& obj_1, T& obj_2)
		{
			obj_2 = xr_new<object_type_traits::remove_pointer<T>::type>(*obj_1);
			CCloner::clone(*obj_1, *obj_2);
		}
	};

	IC static void clone(LPCSTR obj_1, LPCSTR& obj_2)
	{
		obj_2 = obj_1;
	}

	IC static void clone(LPSTR obj_1, LPSTR& obj_2)
	{
		obj_2 = xr_strdup(obj_1);
	}

	IC static void clone(const shared_str& obj_1, shared_str& obj_2)
	{
		obj_2 = obj_1;
	}

	template <typename T1, typename T2> IC static void clone(const std::pair<T1, T2>& obj_1, std::pair<T1, T2>& obj_2)
	{
		clone(const_cast<object_type_traits::remove_const<T1>::type&>(obj_1.first),
			  const_cast<object_type_traits::remove_const<T1>::type&>(obj_2.first));
		clone(obj_1.second, obj_2.second);
	}

	template <typename T, int size> IC static void clone(const svector<T, size>& obj_1, svector<T, size>& obj_2)
	{
		obj_2.resize(obj_1.size());
		svector<T, size>::iterator J = obj_2.begin();
		svector<T, size>::const_iterator I = obj_1.begin();
		svector<T, size>::const_iterator E = obj_1.end();
		for (; I != E; ++I, ++J)
			clone(*I, *J);
	}

	template <typename T1, typename T2>
	IC static void clone(const std::queue<T1, T2>& obj__1, std::queue<T1, T2>& obj__2)
	{
		std::queue<T1, T2> obj_1 = obj__1;
		std::queue<T1, T2> obj_2;

		for (; !obj_1.empty(); obj_1.pop())
			obj_2.push(obj_1.front());

		while (!obj__2.empty())
			obj__2.pop();

		for (; !obj_2.empty(); obj_2.pop())
		{
			std::queue<T1, T2>::value_type t;
			CCloner::clone(obj_2.front(), t);
			obj__2.push(t);
		}
	}

	template <template <typename obj_1, typename obj_2> class T1, typename T2, typename T3>
	IC static void clone(const T1<T2, T3>& obj__1, T1<T2, T3>& obj__2, bool)
	{
		T1<T2, T3> obj_1 = obj__1;
		T1<T2, T3> obj_2;

		for (; !obj_1.empty(); obj_1.pop())
			obj_2.push(obj_1.top());

		while (!obj__2.empty())
			obj__2.pop();

		for (; !obj_2.empty(); obj_2.pop())
		{
			T1<T2, T3>::value_type t;
			CCloner::clone(obj_2.top(), t);
			obj__2.push(t);
		}
	}

	template <template <typename obj_1, typename obj_2, typename obj_3> class T1, typename T2, typename T3, typename T4>
	IC static void clone(const T1<T2, T3, T4>& obj__1, T1<T2, T3, T4>& obj__2, bool)
	{
		T1<T2, T3, T4> obj_1 = obj__1;
		T1<T2, T3, T4> obj_2;

		for (; !obj_1.empty(); obj_1.pop())
			obj_2.push(obj_1.top());

		while (!obj__2.empty())
			obj__2.pop();

		for (; !obj_2.empty(); obj_2.pop())
		{
			T1<T2, T3, T4>::value_type t;
			CCloner::clone(obj_2.top(), t);
			obj__2.push(t);
		}
	}

	template <typename T1, typename T2> IC static void clone(const xr_stack<T1, T2>& obj_1, xr_stack<T1, T2>& obj_2)
	{
		return (clone(obj_1, obj_2, true));
	}

	template <typename T1, typename T2, typename T3>
	IC static void clone(const std::priority_queue<T1, T2, T3>& obj_1, std::priority_queue<T1, T2, T3>& obj_2)
	{
		return (clone(obj_1, obj_2, true));
	}

	struct CHelper3
	{
		template <template <typename obj_1> class T1, typename T2>
		IC static void add(T1<T2>& data, typename T1<T2>::value_type& value)
		{
			data.push_back(value);
		}

		template <typename T1, typename T2> IC static void add(T1& data, typename T2& value)
		{
			data.insert(value);
		}

		template <typename T> IC static void clone(const T& obj_1, T& obj_2)
		{
			obj_2.clear();
			T::const_iterator I = obj_1.begin();
			T::const_iterator E = obj_1.end();
			for (; I != E; ++I)
			{
				T::value_type t;
				CCloner::clone(*I, t);
				add(obj_2, t);
			}
		}
	};

	template <typename T> struct CHelper4
	{
		template <bool a> IC static void clone(const T& obj_1, T& obj_2)
		{
			CHelper<T>::clone<object_type_traits::is_pointer<T>::value>(obj_1, obj_2);
		}

		template <> IC static void clone<true>(const T& obj_1, T& obj_2)
		{
			CHelper3::clone(obj_1, obj_2);
		}
	};

	template <typename T> IC static void clone(const T& obj_1, T& obj_2)
	{
		CHelper4<T>::clone<object_type_traits::is_stl_container<T>::value>(obj_1, obj_2);
	}
};

IC void clone(LPCSTR p0, LPSTR& p1)
{
	p1 = xr_strdup(p0);
}

template <typename T> IC void clone(const T& p0, T& p1)
{
	CCloner::clone(p0, p1);
}
