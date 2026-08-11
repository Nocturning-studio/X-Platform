#ifndef _FIXEDMAP_H
#define _FIXEDMAP_H
#pragma once

template <class K, class T, class allocator = xr_allocator> class FixedMAP
{
	enum
	{
		SG_REALLOC_ADVANCE = 64
	};

  public:
	struct TNode
	{
		K key;
		T val;
		TNode *left, *right;
	};
	typedef void __fastcall callback(TNode*);
	typedef bool __fastcall callback_cmp(TNode& N1, TNode& N2);

  private:
	TNode* nodes;
	u32 pool;
	u32 limit;

	IC u32 Size(u32 Count)
	{
		return Count * sizeof(TNode);
	}

	void Realloc()
	{
		u32 newLimit = limit + SG_REALLOC_ADVANCE;
		TNode* newNodes = (TNode*)allocator::alloc(sizeof(TNode) * newLimit);

		ZeroMemory(newNodes, Size(newLimit));

		for (u32 i = 0; i < pool; ++i) {
			TNode* oldNode = nodes + i;
			TNode* newNode = newNodes + i;

			new (&newNode->key) K(std::move(oldNode->key));
			new (&newNode->val) T(std::move(oldNode->val));

			oldNode->val.~T();
			oldNode->key.~K();

			newNode->left = nullptr;
			newNode->right = nullptr;
		}

		for (u32 i = 0; i < pool; ++i) {
			TNode* oldNode = nodes + i;
			TNode* newNode = newNodes + i;

			if (oldNode->left) {
				size_t lid = oldNode->left - nodes;
				newNode->left = newNodes + lid;
			}
			if (oldNode->right) {
				size_t rid = oldNode->right - nodes;
				newNode->right = newNodes + rid;
			}
		}

		if (nodes)
			allocator::dealloc(nodes);

		nodes = newNodes;
		limit = newLimit;
	}

	IC TNode* Alloc(const K& key)
	{
		if (pool == limit) Realloc();

		TNode* node = nodes + pool;
		new (&node->key) K(key);
		new (&node->val) T();
		node->left = node->right = nullptr;
		++pool;
		return node;
	}

	IC TNode* CreateChild(TNode*& parent, const K& key)
	{
		size_t parentIdx = parent - nodes;
		TNode* N = Alloc(key);
		parent = nodes + parentIdx;
		return N;
	}

	IC void recurseLR(TNode* N, callback CB)
	{
		if (N->left)
			recurseLR(N->left, CB);
		CB(N);
		if (N->right)
			recurseLR(N->right, CB);
	}
	IC void recurseRL(TNode* N, callback CB)
	{
		if (N->right)
			recurseRL(N->right, CB);
		CB(N);
		if (N->left)
			recurseRL(N->left, CB);
	}
	IC void getLR(TNode* N, xr_vector<T, typename allocator::template helper<T>::result>& D)
	{
		if (N->left)
			getLR(N->left, D);
		D.push_back(N->val);
		if (N->right)
			getLR(N->right, D);
	}
	IC void getRL(TNode* N, xr_vector<T, typename allocator::template helper<T>::result>& D)
	{
		if (N->right)
			getRL(N->right, D);
		D.push_back(N->val);
		if (N->left)
			getRL(N->left, D);
	}
	IC void getLR_P(TNode* N, xr_vector<TNode*, typename allocator::template helper<TNode*>::result>& D)
	{
		if (N->left)
			getLR_P(N->left, D);
		D.push_back(N);
		if (N->right)
			getLR_P(N->right, D);
	}
	IC void getRL_P(TNode* N, xr_vector<TNode*, typename allocator::template helper<TNode*>::result>& D)
	{
		if (N->right)
			getRL_P(N->right, D);
		D.push_back(N);
		if (N->left)
			getRL_P(N->left, D);
	}

  public:
	FixedMAP()
	{
		pool = 0;
		limit = 0;
		nodes = 0;
	}
	FixedMAP(FixedMAP&& other) noexcept : nodes(other.nodes), pool(other.pool), limit(other.limit)
	{
		other.nodes = nullptr;
		other.pool = 0;
		other.limit = 0;
	}
	FixedMAP& operator=(FixedMAP&& other) noexcept
	{
		if (this != &other)
		{
			destroy();
			nodes = other.nodes;
			pool = other.pool;
			limit = other.limit;
			other.nodes = nullptr;
			other.pool = 0;
			other.limit = 0;
		}
		return *this;
	}
	~FixedMAP()
	{
		destroy();
	}
	void destroy()
	{
		if (nodes) {
			clear();
			allocator::dealloc(nodes);
			nodes = nullptr;
			pool = 0;
			limit = 0;
		}
	}
	IC TNode* insert(const K& k)
	{
		if (pool) {
			TNode* node = nodes;
		once_more:
			if (k < node->key) {
				if (node->left) { node = node->left; goto once_more; }
				else {
					TNode* N = CreateChild(node, k);
					node->left = N;
					return N;
				}
			}
			else if (node->key < k) {
				if (node->right) { node = node->right; goto once_more; }
				else {
					TNode* N = CreateChild(node, k);
					node->right = N;
					return N;
				}
			}
			else return node;
		}
		else {
			return Alloc(k);
		}
	}
	IC TNode* insertInAnyWay(const K& k)
	{
		if (pool)
		{
			TNode* node = nodes;

		once_more:
			if (k <= node->key)
			{
				if (node->left)
				{
					node = node->left;
					goto once_more;
				}
				else
				{
					TNode* N = CreateChild(node, k);
					node->left = N;
					return N;
				}
			}
			else
			{
				if (node->right)
				{
					node = node->right;
					goto once_more;
				}
				else
				{
					TNode* N = CreateChild(node, k);
					node->right = N;
					return N;
				}
			}
		}
		else
		{
			return Alloc(k);
		}
	}
	IC TNode* insert(const K& k, const T& v)
	{
		TNode* N = insert(k);
		N->val = v;
		return N;
	}
	IC TNode* insertInAnyWay(const K& k, const T& v)
	{
		TNode* N = insertInAnyWay(k);
		N->val = v;
		return N;
	}
	IC void discard()
	{
		if (nodes)
			allocator::dealloc(nodes);
		nodes = 0;
		pool = 0;
		limit = 0;
	}
	IC u32 allocated()
	{
		return this->limit;
	}
	IC void clear()
	{
		TNode* it = begin();
		for (u32 i = 0; i < pool; ++i, ++it) {
			it->val.~T();
			it->key.~K();
		}
		pool = 0;
	}
	IC TNode* begin()
	{
		return nodes;
	}
	IC TNode* end()
	{
		return nodes + pool;
	}
	IC TNode* last()
	{
		return nodes + limit;
	}
	IC u32 size()
	{
		return pool;
	}
	IC TNode& operator[](int v)
	{
		return nodes[v];
	}

	IC void traverseLR(callback CB)
	{
		if (pool)
			recurseLR(nodes, CB);
	}
	IC void traverseRL(callback CB)
	{
		if (pool)
			recurseRL(nodes, CB);
	}
	IC void traverseANY(callback CB)
	{
		TNode* _end = end();
		for (TNode* cur = begin(); cur != _end; cur++)
			CB(cur);
	}

	IC void getLR(xr_vector<T, typename allocator::template helper<T>::result>& D)
	{
		if (pool)
			getLR(nodes, D);
	}
	IC void getLR_P(xr_vector<TNode*, typename allocator::template helper<TNode*>::result>& D)
	{
		if (pool)
			getLR_P(nodes, D);
	}
	IC void getRL(xr_vector<T, typename allocator::template helper<T>::result>& D)
	{
		if (pool)
			getRL(nodes, D);
	}
	IC void getRL_P(xr_vector<TNode*, typename allocator::template helper<TNode*>::result>& D)
	{
		if (pool)
			getRL_P(nodes, D);
	}
	IC void getANY(xr_vector<T, typename allocator::template helper<T>::result>& D)
	{
		TNode* _end = end();
		for (TNode* cur = begin(); cur != _end; cur++)
			D.push_back(cur->val);
	}
	IC void getANY_P(xr_vector<TNode*, typename allocator::template helper<TNode*>::result>& D)
	{
		D.resize(size());
		TNode** _it = D.data();
		TNode* _end = nodes + pool;
		for (TNode* cur = begin(); cur != _end; cur++, _it++)
			*_it = cur;
	}
	IC void getANY_P(xr_vector<void*, typename allocator::template helper<void*>::result>& D)
	{
		D.resize(size());
		TNode** _it = D.data();
		TNode* _end = nodes + pool;
		for (TNode* cur = begin(); cur != _end; cur++, _it++)
			*_it = cur;
	}
	IC void setup(callback CB)
	{
		for (int i = 0; i < limit; i++)
			CB(nodes + i);
	}
	IC TNode* find(const K& k)
	{
		if (pool == 0)
			return nullptr;

		TNode* node = nodes;
		while (node)
		{
			if (k < node->key)
				node = node->left;
			else if (node->key < k)
				node = node->right;
			else
				return node;
		}
		return nullptr;
	}
};
#endif
