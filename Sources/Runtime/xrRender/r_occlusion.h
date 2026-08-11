#pragma once

const u32 occq_size = 2 * 768; // 256	;	// queue for occlusion queries

// must conform to following order of allocation/free
// a(A), a(B), a(C), a(D), ....
// f(A), f(B), f(C), f(D), ....
// a(A), a(B), a(C), a(D), ....
//	this mean:
//		use as litle of queries as possible
//		first try to use queries allocated first
//	assumption:
//		used queries number is much smaller than total count

class R_occlusion
{
  private:
	struct _Q
	{
		IDirect3DQuery9* Q;
		u32 order;
		u32 frame_issued;   // кадр, в котором был вызван Issue(BEGIN)
	};

	BOOL enabled;		 //
	xr_vector<_Q> pool;	 // sorted (max ... min), insertions are usually at the end
	xr_vector<_Q> used;	 // id's are generated from this and it is cleared from back only
	xr_vector<u32> fids; // free id's
  public:
	R_occlusion();
	~R_occlusion();

	IDirect3DQuery9* GetUsedQueryByID(u32 ID)
	{
		return used[ID].Q;
	}

	u32 GetQuerySize()
	{
		return used.size();
	}

	void occq_create(u32 limit);
	void occq_destroy();
	u32 occq_begin(u32& ID); // returns 'order'
	void occq_end(u32& ID);
	u32 occq_get(u32& ID, bool bWait = true);
};
