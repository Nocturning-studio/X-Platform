#pragma once

#include "UIWindow.h"
#include "UIWndCallback.h"

class CUICellContainer;
class CUIScrollBar;
class CUIStatic;
class CUICellItem;
class CUIDragItem;

enum EListType
{
	iwSlot,
	iwBag,
	iwBelt
};

struct CUICell
{
	CUICell()
	{
		m_item = NULL;
		Clear();
	}

	CUICellItem* m_item;
	bool m_bMainItem;
	void SetItem(CUICellItem* itm, bool bMain)
	{
		m_item = itm;
		VERIFY(m_item);
		m_bMainItem = bMain;
	}
	bool Empty()
	{
		return m_item == NULL;
	}
	bool MainItem()
	{
		return m_bMainItem;
	}
	void Clear();
	bool operator==(const CUICell& C) const
	{
		return (m_item == C.m_item);
	}
};

typedef xr_vector<CUICell> UI_CELLS_VEC;
typedef UI_CELLS_VEC::iterator UI_CELLS_VEC_IT;

class CUIDragDropListEx : public CUIWindow, public CUIWndCallback
{
  private:
	typedef CUIWindow inherited;

	enum
	{
		flGroupSimilar = (1 << 0),
		flAutoGrow = (1 << 1),
		flCustomPlacement = (1 << 2)
	};
	Flags8 m_flags;
	CUICellItem* m_selected_item;
	int2 m_orig_cell_capacity;

  protected:
	CUICellContainer* m_container;
	CUIScrollBar* m_vScrollBar;

	void __stdcall OnScrollV(CUIWindow* w, void* pData);
	void __stdcall OnItemStartDragging(CUIWindow* w, void* pData);
	void __stdcall OnItemDrop(CUIWindow* w, void* pData);
	void __stdcall OnItemSelected(CUIWindow* w, void* pData);
	void __stdcall OnItemRButtonClick(CUIWindow* w, void* pData);
	void __stdcall OnItemDBClick(CUIWindow* w, void* pData);

  public:
	static CUIDragItem* m_drag_item;
	CUIDragDropListEx();
	virtual ~CUIDragDropListEx();
	void Init(float x, float y, float w, float h);

	typedef fastdelegate::FastDelegate1<CUICellItem*, bool> DRAG_DROP_EVENT;

	DRAG_DROP_EVENT m_f_item_drop;
	DRAG_DROP_EVENT m_f_item_start_drag;
	DRAG_DROP_EVENT m_f_item_db_click;
	DRAG_DROP_EVENT m_f_item_selected;
	DRAG_DROP_EVENT m_f_item_rbutton_click;

	const int2& CellsCapacity();
	void SetCellsCapacity(const int2 c);
	void SetStartCellsCapacity(const int2 c)
	{
		m_orig_cell_capacity = c;
		SetCellsCapacity(c);
	};
	void ResetCellsCapacity()
	{
		VERIFY(ItemsCount() == 0);
		SetCellsCapacity(m_orig_cell_capacity);
	};
	const int2& CellSize();
	void SetCellSize(const int2 new_sz);
	int ScrollPos();
	void ReinitScroll();
	void GetClientArea(Frect& r);
	float2 GetDragItemPosition();

	void SetAutoGrow(bool b);
	bool IsAutoGrow();
	void SetGrouping(bool b);
	bool IsGrouping();
	void SetCustomPlacement(bool b);
	bool GetCustomPlacement();

  public:
	// items management
	virtual void SetItem(CUICellItem* itm);					   // auto
	virtual void SetItem(CUICellItem* itm, float2 abs_pos);  // start at cursor pos
	virtual void SetItem(CUICellItem* itm, int2 cell_pos); // start at cell
	bool CanSetItem(CUICellItem* itm);

	u32 ItemsCount();
	CUICellItem* GetItemIdx(u32 idx);
	virtual CUICellItem* RemoveItem(CUICellItem* itm, bool force_root);
	void CreateDragItem(CUICellItem* itm);

	void DestroyDragItem();
	void ClearAll(bool bDestroy);
	void Compact();
	bool IsOwner(CUICellItem* itm);

  public:
	// UIWindow overriding
	virtual void Draw();
	virtual void Update();
	virtual bool OnMouse(float x, float y, EUIMessages mouse_action);
	virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
};

class CUICellContainer : public CUIWindow
{
	friend class CUIDragDropListEx;

  private:
	typedef CUIWindow inherited;
	ref_shader hShader; // ownerDraw
	ref_geom hGeom;
	UI_CELLS_VEC m_cells_to_draw;

  protected:
	CUIDragDropListEx* m_pParentDragDropList;

	int2 m_cellsCapacity; // count		(col,	row)
	int2 m_cellSize;	  // pixels	(width, height)
	UI_CELLS_VEC m_cells;

	void GetTexUVLT(float2& uv, u32 col, u32 row);
	void ReinitSize();
	u32 GetCellsInRange(const Irect& rect, UI_CELLS_VEC& res);

  public:
	CUICellContainer(CUIDragDropListEx* parent);
	virtual ~CUICellContainer();

  protected:
	virtual void Draw();

	IC const int2& CellsCapacity()
	{
		return m_cellsCapacity;
	};
	void SetCellsCapacity(const int2& c);
	IC const int2& CellSize()
	{
		return m_cellSize;
	};
	void SetCellSize(const int2& new_sz);
	int2 TopVisibleCell();
	CUICell& GetCellAt(const int2& pos);
	int2 PickCell(const float2& abs_pos);
	int2 GetItemPos(CUICellItem* itm);
	int2 FindFreeCell(const int2& size);
	bool HasFreeSpace(const int2& size);
	bool IsRoomFree(const int2& pos, const int2& size);

	bool AddSimilar(CUICellItem* itm);
	CUICellItem* FindSimilar(CUICellItem* itm);

	void PlaceItemAtPos(CUICellItem* itm, int2& cell_pos);
	CUICellItem* RemoveItem(CUICellItem* itm, bool force_root);
	bool ValidCell(const int2& pos) const;

	void Grow();
	void Shrink();
	void ClearAll(bool bDestroy);
};
