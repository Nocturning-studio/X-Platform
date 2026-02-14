#include "stdafx.h"
#include "uistatic.h"
#include "UIXmlInit.h"
#include "UITextureMaster.h"
#include "uiframewindow.h"
#include "../HUDManager.h"
#include "../../xrEngine/LightAnimLibrary.h"
#include "uilines.h"
#include "../string_table.h"
#include "../ui_base.h"
#include "UIBtnHint.h"
#include "UIHint.h"

const char* const clDefault = "default";
#define CREATE_LINES                                                                                                   \
	if (!m_pLines)                                                                                                     \
	{                                                                                                                  \
		m_pLines = xr_new<CUILines>();                                                                                 \
		m_pLines->SetTextAlignment(CGameFont::alLeft);                                                                 \
	}
#define LA_CYCLIC (1 << 0)
#define LA_ONLYALPHA (1 << 1)
#define LA_TEXTCOLOR (1 << 2)
#define LA_TEXTURECOLOR (1 << 3)
//(1<<4) registered !!!
void lanim_cont::set_defaults()
{
	m_lanim = NULL;
	m_lanim_start_time = -1.0f;
	m_lanim_delay_time = 0.0f;
	m_lanimFlags.zero();
}

CUIStatic::CUIStatic()
{
	m_bAvailableTexture = false;
	m_bTextureEnable = true;
	m_bClipper = false;
	m_bStretchTexture = false;

	m_TextureOffset.set(0.0f, 0.0f);
	m_TextOffset.set(0.0f, 0.0f);

	m_pMask = NULL;
	m_ElipsisPos = eepNone;
	m_iElipsisIndent = 0;

	m_ClipRect.set(-1, -1, -1, -1);

	m_bCursorOverWindow = false;
	m_bHeading = false;
	m_fHeading = 0.0f;
	m_lanim_clr.set_defaults();
	m_lanim_transform.set_defaults();

	m_pLines = NULL;
	m_bEnableTextHighlighting = false;

	m_bChangeVis = false;
	m_hint = nullptr;
}

CUIStatic::~CUIStatic()
{
	xr_delete(m_pLines);
}

void CUIStatic::SetTransformLightAnim(LPCSTR lanim, bool bCyclic)
{
	if (lanim && lanim[0] != 0)
		m_lanim_transform.m_lanim = LALib.FindItem(lanim);
	else
		m_lanim_transform.m_lanim = NULL;

	m_lanim_transform.m_lanimFlags.zero();

	m_lanim_transform.m_lanimFlags.set(LA_CYCLIC, bCyclic);
}

void CUIStatic::SetClrLightAnim(LPCSTR lanim, bool bCyclic, bool bOnlyAlpha, bool bTextColor, bool bTextureColor)
{
	if (lanim && lanim[0] != 0)
		m_lanim_clr.m_lanim = LALib.FindItem(lanim);
	else
		m_lanim_clr.m_lanim = NULL;

	m_lanim_clr.m_lanimFlags.zero();

	m_lanim_clr.m_lanimFlags.set(LA_CYCLIC, bCyclic);
	m_lanim_clr.m_lanimFlags.set(LA_ONLYALPHA, bOnlyAlpha);
	m_lanim_clr.m_lanimFlags.set(LA_TEXTCOLOR, bTextColor);
	m_lanim_clr.m_lanimFlags.set(LA_TEXTURECOLOR, bTextureColor);
}

void CUIStatic::Init(LPCSTR tex_name, float x, float y, float width, float height)
{
	Init(x, y, width, height);
	InitTexture(tex_name);
	if (m_hint)
	{
		m_hint->SetOwner(this);
	}
}

void CUIStatic::InitEx(LPCSTR tex_name, LPCSTR sh_name, float x, float y, float width, float height)
{
	Init(x, y, width, height);
	InitTextureEx(tex_name, sh_name);
	if (m_hint)
	{
		m_hint->SetOwner(this); // <--- ОБЯЗАТЕЛЬНО ДОБАВИТЬ
	}
}

void CUIStatic::Init(float x, float y, float width, float height)
{
	CUIWindow::Init(x, y, width, height);
	m_xxxRect.set(x, y, x + width, y + height);
	if (m_hint)
	{
		m_hint->SetOwner(this); // <--- ОБЯЗАТЕЛЬНО ДОБАВИТЬ
	}
}

void CUIStatic::InitTexture(LPCSTR texture)
{
	InitTextureEx(texture);
}

void CUIStatic::CreateShader(const char* tex, const char* sh)
{
	m_UIStaticItem.CreateShader(tex, sh);
}

ref_shader& CUIStatic::GetShader()
{
	return m_UIStaticItem.GetShader();
}

void CUIStatic::SetTextureColor(u32 color)
{
	m_UIStaticItem.SetColor(color);
}

u32 CUIStatic::GetTextureColor() const
{
	return m_UIStaticItem.GetColor();
}

void CUIStatic::InitTextureEx(LPCSTR tex_name, LPCSTR sh_name)
{

	string_path buff;
	u32 v_dev = CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
	u32 v_need = CAP_VERSION(2, 0);
	if (/*strstr(Core.Params,"-ps_movie") &&*/ (v_dev >= v_need) && FS.exist(buff, "$game_textures$", tex_name, ".ogm"))
		CUITextureMaster::InitTexture(tex_name, "hud\\movie", &m_UIStaticItem);
	else
		CUITextureMaster::InitTexture(tex_name, sh_name, &m_UIStaticItem);

	float2 p = GetWndPos();
	m_UIStaticItem.SetPos(p.x, p.y);
	m_bAvailableTexture = true;
}

void CUIStatic::Draw()
{
	if (m_bClipper)
	{
		Frect clip_rect;
		if (-1 == m_ClipRect.left && -1 == m_ClipRect.right && -1 == m_ClipRect.top && -1 == m_ClipRect.left)
		{
			Frect our_rect;
			GetAbsoluteRect(our_rect);
			clip_rect = our_rect;
			Frect _r;
			GetParent()->GetAbsoluteRect(_r);
			if (GetParent())
				clip_rect.intersection(our_rect, _r);
		}
		else
			clip_rect = m_ClipRect;

		UI()->PushScissor(clip_rect);
	}

	DrawTexture();
	inherited::Draw();
	DrawText();

	if (m_bClipper)
		UI()->PopScissor();
}

void CUIStatic::DrawText()
{
	if (m_pLines)
	{
		m_pLines->SetWndSize(m_wndSize);

		if (IsHighlightText() && xr_strlen(m_pLines->GetText()) > 0 && m_bEnableTextHighlighting)
			DrawHighlightedText();
		else
		{
			float2 p;
			GetAbsolutePos(p);
			m_pLines->Draw(p.x + m_TextOffset.x, p.y + m_TextOffset.y);
		}
	}
}

void CUIStatic::DrawTexture()
{

	if (m_bAvailableTexture && m_bTextureEnable)
	{
		Frect rect;
		GetAbsoluteRect(rect);
		m_UIStaticItem.SetPos(rect.left + m_TextureOffset.x, rect.top + m_TextureOffset.y);

		if (m_bStretchTexture)
			m_UIStaticItem.SetRect(0, 0, rect.width(), rect.height());
		else
		{
			Frect r = {0.0f, 0.0f, m_UIStaticItem.GetOriginalRectScaled().width(),
					   m_UIStaticItem.GetOriginalRectScaled().height()};
			if (r.width() && r.height())
				m_UIStaticItem.SetRect(r);
		}

		if (Heading())
		{
			m_UIStaticItem.Render(GetHeading());
		}
		else
			m_UIStaticItem.Render();
	}
}

bool is_in_2_(const Frect& b1, const Frect& b2)
{
	return (b1.x1 < b2.x1) && (b1.x2 > b2.x2) && (b1.y1 < b2.y1) && (b1.y2 > b2.y2);
}

void CUIStatic::Update()
{
	inherited::Update();

	// 1. ПРОВЕРКА ИЕРАРХИИ РОДИТЕЛЕЙ
	// Проходимся вверх по дереву окон. Если хоть один родитель скрыт -> bParentVisible = false
	bool bParentVisible = true;
	CUIWindow* parent = GetParent();
	while (parent)
	{
		if (!parent->IsShown())
		{
			bParentVisible = false;
			break;
		}
		parent = parent->GetParent();
	}

	// 2. ОБНОВЛЕНИЕ АНИМАЦИЙ (Light Animations)
	if (m_lanim_clr.m_lanim)
	{
		if (m_lanim_clr.m_lanim_start_time < 0.0f)
			ResetClrAnimation();
		float t = Engine.TimeManager.GetContinualTimeMs() / 1000.0f;

		if (t >= m_lanim_clr.m_lanim_start_time)
		{
			if (m_lanim_clr.m_lanimFlags.test(LA_CYCLIC) ||
				t - m_lanim_clr.m_lanim_start_time < m_lanim_clr.m_lanim->Length_sec())
			{
				int frame;
				u32 clr = m_lanim_clr.m_lanim->CalculateRGB(t - m_lanim_clr.m_lanim_start_time, frame);

				if (m_lanim_clr.m_lanimFlags.test(LA_TEXTURECOLOR))
					if (m_lanim_clr.m_lanimFlags.test(LA_ONLYALPHA))
						SetColor(color_set_alpha(GetColor(), color_get_A(clr)));
					else
						SetColor(clr);

				if (m_lanim_clr.m_lanimFlags.test(LA_TEXTCOLOR))
					if (m_lanim_clr.m_lanimFlags.test(LA_ONLYALPHA))
						SetTextColor(color_set_alpha(GetTextColor(), color_get_A(clr)));
					else
						SetTextColor(clr);
			}
		}
	}

	if (m_lanim_transform.m_lanim)
	{
		if (m_lanim_transform.m_lanim_start_time < 0.0f)
			ResetTransformAnimation();
		float t = Engine.TimeManager.GetContinualTimeMs() / 1000.0f;

		if (m_lanim_transform.m_lanimFlags.test(LA_CYCLIC) ||
			t - m_lanim_transform.m_lanim_start_time < m_lanim_transform.m_lanim->Length_sec())
		{
			int frame;
			u32 clr = m_lanim_transform.m_lanim->CalculateRGB(t - m_lanim_transform.m_lanim_start_time, frame);

			EnableHeading_int(true);
			float heading = (PI_MUL_2 / 255.0f) * color_get_A(clr);
			SetHeading(heading);

			float _value = (float)color_get_R(clr);
			float f_scale = _value / 64.0f;
			float2 _sz;
			_sz.set(m_xxxRect.width() * f_scale, m_xxxRect.height() * f_scale);
			SetWndSize(_sz);
		}
		else
		{
			EnableHeading_int(!!m_lanim_transform.m_lanimFlags.test(1 << 4));
			SetWndSize(float2().set(m_xxxRect.width(), m_xxxRect.height()));
		}
	}

	// 3. ГЛОБАЛЬНЫЙ ХИНТ (g_btnHint)
	// Показываем, ТОЛЬКО если:
	// Родители видимы И мы видимы И курсор на нас И есть текст И никто другой не занял хинт
	if (bParentVisible && GetVisible() && CursorOverWindow() && m_hint_text.size() && !g_btnHint->Owner() &&
		Engine.TimeManager.GetGlobalTimeMs() > m_dwFocusReceiveTime + 500)
	{
		g_btnHint->SetHintText(this, *m_hint_text);

		float2 c_pos = GetUICursor()->GetCursorPosition();
		Frect vis_rect;
		vis_rect.set(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

		Frect r;
		r.set(0.0f, 0.0f, g_btnHint->GetWidth(), g_btnHint->GetHeight());
		r.add(c_pos.x, c_pos.y);

		r.sub(0.0f, r.height());
		if (false == is_in_2_(vis_rect, r))
			r.sub(r.width(), 0.0f);
		if (false == is_in_2_(vis_rect, r))
			r.add(0.0f, r.height());

		if (false == is_in_2_(vis_rect, r))
			r.add(r.width(), 45.0f);

		g_btnHint->SetWndPos(r.lt);
	}
	// Сброс глобального хинта: если мы им владеем, но условия нарушены
	else if (g_btnHint->Owner() == this)
	{
		if (!bParentVisible || !GetVisible() || !CursorOverWindow())
		{
			g_btnHint->Discard();
		}
	}

	// 4. КАСТОМНЫЙ ХИНТ (m_hint)
	if (m_hint && m_hint->HintStatic())
	{
		// Страховка: если Owner не был задан при инициализации
		if (!m_hint->Owner())
			m_hint->SetOwner(this);

		// Можно ли показывать?
		// Должны быть выполнены ВСЕ условия: родители видимы, мы видимы, курсор на нас
		bool bCanShow = bParentVisible && GetVisible() && m_bCursorOverWindow;

		u32 hintColor;

		if (bCanShow)
		{
			// Показываем (Highlighted цвет)
			hintColor = m_hint->HintStatic()->m_bUseTextColor[H] ? m_hint->HintStatic()->m_dwTextColor[H]
																 : m_hint->HintStatic()->m_dwTextColor[E];
			if (m_bChangeVis)
				m_hint->HintStatic()->SetVisible(true);
		}
		else
		{
			// Скрываем (Enabled цвет)
			hintColor = m_hint->HintStatic()->m_dwTextColor[E];

			// Если включено скрытие (changeVisability) ИЛИ родитель вообще исчез - скрываем полностью
			if (m_bChangeVis || !bParentVisible || !GetVisible())
				m_hint->HintStatic()->SetVisible(false);
		}

		m_hint->HintStatic()->SetTextColor(hintColor);

		// Позиционирование рассчитываем только если родитель в принципе виден
		if (bParentVisible)
		{
			float2 parentPos;
			// Используем GetParent() самого хинта, так как он приаттачен к нам
			if (m_hint->GetParent())
			{
				m_hint->GetParent()->GetAbsolutePos(parentPos);
				float2 cursosPos = GetUICursor()->GetCursorPosition();

				float x = cursosPos.x - parentPos.x;
				float y = cursosPos.y - parentPos.y - m_hint->GetHeight();

				if (cursosPos.x + m_hint->GetWidth() > UI_BASE_WIDTH)
				{
					x = (UI_BASE_WIDTH - m_hint->GetWidth()) - parentPos.x;
				}
				else if (cursosPos.x + m_hint->GetWidth() > UI_BASE_WIDTH * 1.5)
				{
					x -= m_hint->GetWidth();
				}

				m_hint->SetWndPos(x, y);
			}
		}
	}
	else if (m_hint && m_hint->HintStatic() && !bParentVisible)
	{
		// Резервное скрытие, если мы не попали в основной блок
		m_hint->HintStatic()->SetVisible(false);
	}
}


void CUIStatic::ResetTransformAnimation()
{
	m_lanim_transform.m_lanim_start_time = Engine.TimeManager.GetContinualTimeMs() / 1000.0f;
}

void CUIStatic::ResetClrAnimation()
{
	m_lanim_clr.m_lanim_start_time = Engine.TimeManager.GetContinualTimeMs() / 1000.0f + m_lanim_clr.m_lanim_delay_time / 1000.0f;
}

void CUIStatic::SetClrAnimDelay(float delay)
{
	m_lanim_clr.m_lanim_delay_time = delay;
}

bool CUIStatic::IsClrAnimStoped()
{
	if (m_lanim_clr.m_lanimFlags.test(LA_CYCLIC) || m_lanim_clr.m_lanim_start_time < 0.0f)
		return false;

	float t = Engine.TimeManager.GetContinualTimeMs() / 1000.0f;
	if (t - m_lanim_clr.m_lanim_start_time < m_lanim_clr.m_lanim->Length_sec())
		return false;
	else
		return true;
}

void CUIStatic::SetFont(CGameFont* pFont)
{
	CUIWindow::SetFont(pFont);
	CREATE_LINES;
	m_pLines->SetFont(pFont);
}

void CUIStatic::SetTextComplexMode(bool md)
{
	CREATE_LINES;
	m_pLines->SetTextComplexMode(md);
}

CGameFont* CUIStatic::GetFont()
{
	CREATE_LINES;
	return m_pLines->GetFont();
}

void CUIStatic::TextureClipper(float offset_x, float offset_y, Frect* pClipRect)
{
	TextureClipper(offset_x, offset_y, pClipRect, m_UIStaticItem);
}

void CUIStatic::TextureClipper(float offset_x, float offset_y, Frect* pClipRect, CUIStaticItem& UIStaticItem)
{
	Frect parent_rect;

	if (pClipRect == NULL)
		if (GetParent())
			GetParent()->GetAbsoluteRect(parent_rect);
		else
			GetAbsoluteRect(parent_rect);
	else
		parent_rect = *pClipRect;

	Frect rect;
	GetAbsoluteRect(rect);
	Frect out_rect;

	// проверить попадает ли изображение в окно
	if (rect.left > parent_rect.right || rect.right < parent_rect.left || rect.top > parent_rect.bottom ||
		rect.bottom < parent_rect.top)
	{
		Frect r;
		r.set(0.0f, 0.0f, 0.0f, 0.0f);
		UIStaticItem.SetRect(r);
		return;
	}

	float out_x, out_y;
	out_x = rect.left;
	out_y = rect.top;

	// out_rect - прямоугольная область в которую будет выводиться
	// изображение, вычисляется с учетом положения относительно родительского
	// окна, а также размеров прямоугольника на текстуре с изображением.

	out_rect.intersection(parent_rect, rect);
	out_rect.left -= out_x;
	out_rect.top -= out_y;
	out_rect.right -= out_x;
	out_rect.bottom -= out_y;

	if (m_bStretchTexture)
		UIStaticItem.SetRect(out_rect);
	else
	{
		Frect r;
		r.x1 = out_rect.left;
		r.x2 = out_rect.right < UIStaticItem.GetOriginalRectScaled().width()
				   ? out_rect.right
				   : UIStaticItem.GetOriginalRectScaled().width();

		r.y1 = out_rect.top;
		r.y2 = out_rect.bottom < UIStaticItem.GetOriginalRectScaled().height()
				   ? out_rect.bottom
				   : UIStaticItem.GetOriginalRectScaled().height();

		UIStaticItem.SetRect(r);
	}
}

void CUIStatic::ClipperOn()
{
	m_bClipper = true;

	TextureClipper(0, 0);
}

void CUIStatic::ClipperOff(CUIStaticItem& UIStaticItem)
{
	m_bClipper = false;

	Frect out_rect;

	out_rect.top = 0;
	out_rect.bottom = GetHeight();
	out_rect.left = 0;
	out_rect.right = GetWidth();

	Frect r;
	r.x1 = out_rect.left;
	r.x2 = out_rect.right < UIStaticItem.GetOriginalRectScaled().width() ? out_rect.right
																		 : UIStaticItem.GetOriginalRectScaled().width();

	r.y1 = out_rect.top;
	r.y2 = out_rect.bottom < UIStaticItem.GetOriginalRectScaled().height()
			   ? out_rect.bottom
			   : UIStaticItem.GetOriginalRectScaled().height();
	UIStaticItem.SetRect(r);
}

void CUIStatic::ClipperOff()
{
	ClipperOff(m_UIStaticItem);
}

void CUIStatic::SetShader(const ref_shader& sh)
{
	m_UIStaticItem.SetShader(sh);
	m_bAvailableTexture = true;
}

LPCSTR CUIStatic::GetText()
{
	static const char empty = 0;
	if (m_pLines)
		return m_pLines->GetText();
	else
		return &empty;
}

void CUIStatic::SetTextColor(u32 color)
{
	CREATE_LINES;
	m_pLines->SetTextColor(color);
}

u32 CUIStatic::GetTextColor()
{
	CREATE_LINES;
	return m_pLines->GetTextColor();
}

u32& CUIStatic::GetTextColorRef()
{
	return m_pLines->GetTextColorRef();
}

void CUIStatic::SetText(LPCSTR str)
{
	if (!str)
		return;
	CREATE_LINES;
	m_pLines->SetText(str);
}

void CUIStatic::SetTextColor(u32 color, E4States state)
{
	m_dwTextColor[state] = color;
	m_bUseTextColor[state] = true;
}

Frect CUIStatic::GetClipperRect()
{
	if (m_bClipper)
		return m_ClipRect;
	else
		return GetSelfClipRect();
}

Frect CUIStatic::GetSelfClipRect()
{
	Frect r;
	if (m_bClipper)
	{
		r.set(GetUIStaticItem().GetRect());
		r.add(GetUIStaticItem().GetPosX(), GetUIStaticItem().GetPosY());
	}
	else
		r.set(0.0f, 0.0f, UI_BASE_WIDTH, UI_BASE_HEIGHT);

	return r;
}

void CUIStatic::SetMask(CUIFrameWindow* pMask)
{
	DetachChild(m_pMask);
	m_pMask = pMask;
	if (m_pMask)
	{
		AttachChild(m_pMask);
		Frect r = GetWndRect();
		m_pMask->SetWidth(r.right - r.left);
		m_pMask->SetHeight(r.bottom - r.top);
	}
}

// CGameFont::EAligment CUIStatic::GetTextAlign(){
//	return m_pLines->GetTextAlignment();
// }

CGameFont::EAligment CUIStatic::GetTextAlignment()
{
	return m_pLines->GetTextAlignment();
}

void CUIStatic::SetVisible(bool vis)
{
	inherited::SetVisible(vis);

	// Скрываем подсказки при скрытии элемента
	if (!vis)
	{
		if (m_hint && m_hint->HintStatic())
			m_hint->HintStatic()->SetVisible(false);

		// Также скрываем глобальную подсказку если она наша
		if (g_btnHint->Owner() == this)
			g_btnHint->Discard();
	}
}

// void CUIStatic::SetTextAlign(CGameFont::EAligment align){
//	CREATE_LINES;
//	m_pLines->SetTextAlignment(align);
// }

void CUIStatic::SetTextAlignment(CGameFont::EAligment align)
{
	CREATE_LINES;
	m_pLines->SetTextAlignment(align);
	m_pLines->GetFont()->SetAligment((CGameFont::EAligment)align);
}

void CUIStatic::SetVTextAlignment(EVTextAlignment al)
{
	CREATE_LINES;
	m_pLines->SetVTextAlignment(al);
}

void CUIStatic::SetTextAlign_script(u32 align)
{
	m_pLines->SetTextAlignment((CGameFont::EAligment)align);
	m_pLines->GetFont()->SetAligment((CGameFont::EAligment)align);
}

u32 CUIStatic::GetTextAlign_script()
{
	return static_cast<u32>(m_pLines->GetTextAlignment());
}

#pragma todo("Satan->Satan : need adaptation")
void CUIStatic::Elipsis(const Frect& rect, EElipsisPosition elipsisPos)
{
	// if (eepNone == elipsisPos) return;

	// CUIStatic::Elipsis(m_sEdit, rect, elipsisPos, GetFont());

	//// Now paste elipsis
	// m_str = &m_sEdit.front();
	// str_len = m_sEdit.size();
	// buf_str.resize(str_len + 1);
}

void CUIStatic::SetElipsis(EElipsisPosition pos, int indent)
{
#pragma todo("Satan->Satan : need adaptation")
	m_ElipsisPos = pos;
	m_iElipsisIndent = indent;
}

void CUIStatic::SetClipRect(Frect r)
{
	m_ClipRect = r;
}

void CUIStatic::OnFocusReceive()
{
	inherited::OnFocusReceive();
	if (GetMessageTarget())
		GetMessageTarget()->SendMessage(this, STATIC_FOCUS_RECEIVED, NULL);
}

void CUIStatic::OnFocusLost()
{
	inherited::OnFocusLost();

	if (GetMessageTarget())
		GetMessageTarget()->SendMessage(this, STATIC_FOCUS_LOST, NULL);

	// Если окно закрывается, фокус теряется. Принудительно убираем хинт.
	if (g_btnHint->Owner() == this)
	{
		g_btnHint->Discard();
	}

	// На случай, если кастомный хинт всё еще висит
	if (m_hint && m_hint->HintStatic())
	{
		m_hint->HintStatic()->SetVisible(false);
	}
}

void CUIStatic::AdjustHeightToText()
{
	if (!m_pLines)
		return;

	// 1. Вычисляем РЕАЛЬНУЮ доступную ширину для текста.
	// Если мы рисуем текст со смещением m_TextOffset.x, то для самого текста
	// остается меньше места, иначе он вылезет за правую границу.
	float available_width = GetWidth() - m_TextOffset.x;

	// Защита от некорректных размеров (чтобы не крашнуть парсер)
	if (available_width < 10.0f)
		available_width = 10.0f;

	m_pLines->SetWidth(available_width);

	// Принудительно пересчитываем перенос строк
	m_pLines->ParseText();

	// 2. Получаем высоту текста
	float text_height = m_pLines->GetVisibleHeight();

	// 3. Итоговая высота окна = Высота текста + Отступ сверху (m_TextOffset.y).
	// Без этого нижняя часть текста будет обрезана окном.
	float final_height = text_height + m_TextOffset.y;

	// 4. Добавляем небольшой буфер (паддинг снизу),
	// чтобы компенсировать погрешности масштабирования на разных разрешениях
	// и чтобы "хвостики" букв (p, g, y, j, q) не касались края текстуры.
	final_height += 4.0f;

	SetHeight(final_height);
}

void CUIStatic::AdjustWidthToText()
{
	float width = 0;

	if (m_pLines->GetTextComplexMode())
	{
		xr_vector<xr_string> tokens;
		{
			xr_string line = m_pLines->GetText();

			size_t pos = 0;
			do
			{
				pos = line.find("\\n");
				xr_string token = line.substr(0, pos);
				tokens.push_back(token);

				line.erase(0, token.size() + 2);
			} while (pos != line.npos);

			for (auto& it : tokens)
			{
				it.erase(std::remove(it.begin(), it.end(), '\n'), it.end());
			}
		}

		for (const auto& it : tokens)
		{
			float _len = m_pLines->GetFont()->SizeOf_(it.c_str());
			if (_len > width)
				width = _len;
		}

		width += 6;
	}
	else
	{
		// Single line calc
		width = m_pLines->GetFont()->SizeOf_(m_pLines->GetText());
	}

	UI()->ClientToScreenScaledWidth(width);
	SetWidth(width);
}

void CUIStatic::RescaleRelative2Rect(const Frect& r)
{
	SetStretchTexture(true);
	Frect my_r = m_xxxRect;
	float h_rel = my_r.width() / r.width();
	float v_rel = my_r.height() / r.height();

	if (ui_core::is_16_9_mode())
	{
		h_rel *= (3.0f / 4.0f);
	}

	float w;
	float h;
	if (h_rel < v_rel)
	{
		w = r.width() * h_rel;
		h = r.height() * h_rel;
	}
	else
	{
		w = r.width() * v_rel;
		h = r.height() * v_rel;
	}

	my_r.x1 += (m_xxxRect.width() - w) / 2;
	my_r.y1 += (m_xxxRect.height() - h) / 2;
	my_r.x2 = my_r.x1 + w;
	my_r.y2 = my_r.y1 + h;
	SetWndRect(my_r);
}

void CUIStatic::SetTextST(LPCSTR str_id)
{
	SetText(*CStringTable().translate(str_id));
}

void CUIStatic::DrawHighlightedText()
{
	Frect rect;
	GetAbsoluteRect(rect);
	u32 def_col = m_pLines->GetTextColor();
	m_pLines->SetTextColor(m_HighlightColor);
	m_pLines->Draw(rect.left + 0 + m_TextOffset.x, rect.top - 0 + m_TextOffset.y);
	m_pLines->SetTextColor(def_col);
}

bool CUIStatic::IsHighlightText()
{
	return m_bCursorOverWindow;
}
