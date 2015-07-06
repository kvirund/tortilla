#include "stdafx.h"
#include "popupWindow.h"
void sendLog(const utf8* msg); //debug
extern char buffer[]; //debug

void PopupWindow::onTick()
{
    DWORD dt = m_ticker.getDiff();
    m_ticker.sync();
    if (m_animation_state == ANIMATION_MOVE)
    {
        POINT &p = m_animation.pos;
        const POINT& target = m_move_animation.pos;

        float x = static_cast<float>(p.x);
        float y = static_cast<float>(p.y);

        float dx = m_move_dx * m_move_animation.speed * dt;
        float dy = m_move_dy * m_move_animation.speed * dt;

        float ax = abs(dx); float ay = abs(dy);
        if (ax > 6 || ay > 6  || (ax > 0 && ax < 1) || (ay > 0 && ay < 1))
        {
            p.x = target.x;
            p.y = target.y;
        }
        else
        {
            p.x = static_cast<LONG>(x+dx);
            p.y = static_cast<LONG>(y+dy);
        }

        bool stop = false;
        if ((m_move_dx < 0 && p.x <= target.x) || (m_move_dx > 0 && p.x >= target.x))
            { p.x = target.x; stop = true; }
        if ((m_move_dy < 0 && p.y <= target.y) || (m_move_dy > 0 && p.y >= target.y))
            { p.y = target.y; stop = true; }
        if (!stop && p.x == target.x && p.y == target.y)
            { stop = true; }
        SIZE sz = getSize();
        RECT pos = { p.x, p.y, p.x+sz.cx, p.y+sz.cy };
        MoveWindow(&pos);
        if (stop)
        {
            setState(ANIMATION_WAIT);
            sendNotify(MOVEANIMATION_FINISHED);
        }
    }

    if (m_animation_state == ANIMATION_NONE)
    {
        assert(false);
        return;
    }

    if (m_animation_state == ANIMATION_WAIT)
    {
        wait_timer += dt;
        int end_timer = m_animation.wait_sec * 1000;
        if (wait_timer >= end_timer)
            setState(ANIMATION_TOSTART);
        return;
    }
    float da = static_cast<float>(dt) * m_animation.speed;
    if (m_animation_state == ANIMATION_TOEND)
    {
        alpha = min(alpha+da, 255.0f);
        setAlpha(alpha);
        if (alpha == 255.0f)
        {
            setState(ANIMATION_WAIT);
            sendNotify(STARTANIMATION_FINISHED);
        }
    }
    if (m_animation_state == ANIMATION_TOSTART)
    {
        alpha = max(alpha-da, 0.0f);
        setAlpha(alpha);
        if (alpha == 0.0f)
            setState(ANIMATION_NONE);
    }
}

void PopupWindow::startAnimation(const Animation& a)
{
    m_animation = a;
    setState(ANIMATION_TOEND);

    const POINT &rb = a.pos;
    SIZE sz = getSize();

    char buffer[128];
    sprintf(buffer, "show: %d, %d, %d, %d, %p", rb.x, rb.y, sz.cx, sz.cy, this);
    sendLog(buffer); //debug

}

void PopupWindow::startMoveAnimation(const MoveAnimation& a)
{
    const POINT &p = m_animation.pos;
    m_move_dx = static_cast<float>(a.pos.x - p.x);
    m_move_dy = static_cast<float>(a.pos.y - p.y);
    m_move_animation = a;
    setState(ANIMATION_MOVE);
}

void PopupWindow::setState(int newstate)
{
    const Animation &a = m_animation;
    m_animation_state = newstate;

    char buffer[128];
    switch(m_animation_state)
    {
    case ANIMATION_TOEND:
        sprintf(buffer, "alpha+ %p", this);
        break;
    case ANIMATION_TOSTART:
        sprintf(buffer, "alpha- %p", this);
        break;
    case ANIMATION_MOVE:
        sprintf(buffer, "move %p", this);
        break;
    case ANIMATION_WAIT:
        sprintf(buffer, "wait %p", this);
        break;
    case ANIMATION_NONE:
        sprintf(buffer, "none %p", this);
        break;
      }
    sendLog(buffer);

    switch(m_animation_state)
    {
    case ANIMATION_TOEND:
    {
        fillDC();

        BLENDFUNCTION blend;
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.AlphaFormat = 0;
        blend.SourceConstantAlpha = 0;

        CWindow dw(GetDesktopWindow());
        CDC dstdc(dw.GetDC());
        POINT dstpt = {a.pos.x, a.pos.y};
        SIZE sz = getSize();
        POINT srcpt = {0,0};
        if (!UpdateLayeredWindow(m_hWnd, (HDC)dstdc, &dstpt, &sz, m_src_dc, &srcpt, 0, &blend, ULW_ALPHA))
        {
            DWORD lasterr = GetLastError();
            sprintf(buffer, "ULW error: %d, %p", lasterr, this);
            sendLog(buffer);
        }
        ShowWindow(SW_SHOWNOACTIVATE);
    }
    break;
    case ANIMATION_NONE:

        ShowWindow(SW_HIDE);
        m_src_dc.destroy();
        wait_timer = 0;
        alpha = 0;
        sendNotify(ANIMATION_FINISHED);
    break;
    }
    m_ticker.sync();
}

void PopupWindow::calcDCSize()
{
    assert(m_font);
    CDC dc(GetDC());
    HFONT oldfont = dc.SelectFont(*m_font);
    GetTextExtentPoint32(dc, m_text.c_str(), m_text.length(), &m_dc_size);
    dc.SelectFont(oldfont);
}

void PopupWindow::fillDC()
{
    assert(m_font);
    SIZE sz = getSize();
    CDC m_wnd_dc(GetDC());
    m_src_dc.create(m_wnd_dc, sz);
    CDCHandle pdc(m_src_dc);
    RECT rc = { 0, 0, sz.cx, sz.cy};
    pdc.FillSolidRect(&rc, m_animation.bkgnd_color);
    HFONT old_font = pdc.SelectFont(*m_font);
    pdc.SetBkColor(m_animation.bkgnd_color);
    pdc.SetTextColor(m_animation.text_color);
    pdc.DrawText(m_text.c_str(), m_text.length(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    pdc.SelectFont(old_font);

    CPen p;
    p.CreatePen(PS_SOLID, 2, m_animation.text_color);
    HPEN old = pdc.SelectPen(p);
    pdc.MoveTo(rc.left+1, rc.top+1);
    pdc.LineTo(rc.right-1, rc.top+1);
    pdc.LineTo(rc.right-1, rc.bottom-1);
    pdc.LineTo(rc.left+1, rc.bottom-1);
    pdc.LineTo(rc.left+1, rc.top);
    pdc.SelectPen(old);
}

void PopupWindow::setAlpha(float a)
{
    BYTE va = static_cast<BYTE>(a);
    if (va == 0)
    {
        char buffer[32];
        sprintf(buffer, "fully transparent %p", this);
        sendLog(buffer);
    }
    else if (va == 255)
    {
        char buffer[32];
        sprintf(buffer, "fully opaque %p", this);
        sendLog(buffer);
    }
    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = 0;
    blend.SourceConstantAlpha = va;
    if (!UpdateLayeredWindow(m_hWnd, NULL, NULL, NULL, NULL, NULL,  NULL, &blend, ULW_ALPHA))
    {
        DWORD lasterr = GetLastError();
        char buffer[128];
        sprintf(buffer, "ULW2 error: %d, %p", lasterr, this);
        sendLog(buffer);
    }
}

void PopupWindow::onClickButton()
{
    setState(ANIMATION_TOSTART);
    sendNotify(CLICK_EVENT);
}

void PopupWindow::sendNotify(int state)
{
     if (m_animation.notify_wnd)
            ::PostMessage(m_animation.notify_wnd, m_animation.notify_msg, m_animation.notify_param, state);
}