#include "stdafx.h"
#include "mapperRender.h"
#include "helpers.h"
#include "mapper.h"

#define ROOM_SIZE 32
#define MAP_EDGE 16
extern Mapper* m_mapper_window;

MapperRender::MapperRender() : rr(ROOM_SIZE, 5)
{
    m_hscroll_pos = -1;
    m_hscroll_size = 0;
    m_vscroll_pos = -1;
    m_vscroll_size = 0;
    m_hscroll_flag = false;
    m_vscroll_flag = false;
    m_block_center = true;
    m_track_mouse = false;
    m_drag_mode = false;
    m_menu_tracked_room = NULL;
    m_menu_handler = NULL;
}

void MapperRender::setMenuHandler(HWND handler_wnd)
{
    m_menu_handler = handler_wnd;
}

MapCursor MapperRender::getCursor() const
{
    MapCursor cursor = viewpos; // ? viewpos : currentpos;
    return (cursor && cursor->valid()) ? cursor : MapCursor();
}

void MapperRender::onCreate()
{
    createMenu();
    m_background.CreateSolidBrush(RGB(0,90,0));
    updateScrollbars(false);
}

void MapperRender::showPosition(MapCursor pos)
{
    if (pos->valid() && pos->pos().zid >= 0) {
        currentpos = pos;
    }
    viewpos = pos;
    updateScrollbars(false);
    Invalidate();
}

MapCursor MapperRender::getCurrentPosition()
{
    return currentpos;
}

void MapperRender::onPaint()
{
    RECT pos; GetClientRect(&pos);
    CPaintDC dc(m_hWnd);
    CMemoryDC mdc(dc, pos);
    mdc.FillRect(&pos, m_background);
    rr.setDC(mdc);
    rr.setIcons(&m_icons);
    renderMap(getRenderX(), getRenderY());
}

void MapperRender::renderMap(int render_x, int render_y)
{
    MapCursor pos = getCursor();
    if (!pos) return;

    const Rooms3dCubePos& p = pos->pos();
    const Rooms3dCubeSize& sz = pos->size();

    if (sz.minlevel <= (p.z-1))
        renderLevel(p.z-1, render_x+6, render_y+6, 1, pos);
    renderLevel(p.z, render_x, render_y, 0, pos);
    if (sz.maxlevel >= (p.z+1))
        renderLevel(p.z+1, render_x-6, render_y-6, 2, pos);

    if (pos->color() != RCC_NONE)
    {
        int cursor_x = (p.x - sz.left) * ROOM_SIZE + render_x;
        int cursor_y = (p.y - sz.top) * ROOM_SIZE + render_y;
        rr.renderCursor(cursor_x, cursor_y, (pos->color() == RCC_NORMAL) ? 0 : 1 );
    }
}

void MapperRender::renderLevel(int z, int render_x, int render_y, int type, MapCursor pos)
{
    RECT rc;
    GetClientRect(&rc);
    const Rooms3dCubeSize& sz = pos->size();
    Rooms3dCubePos p; p.z = z;
    for (int x=0; x<sz.width(); ++x)
    {
        p.x = x + sz.left;
        for (int y=0; y<sz.height(); ++y)
        {
            p.y = y + sz.top;
            const Room *r = pos->room(p);
            if (!r)
                continue;
            int px = ROOM_SIZE * x + render_x;
            int py = ROOM_SIZE * y + render_y;
            rr.render(px, py, r, type);
        }
    }
}

const Room* MapperRender::findRoomOnScreen(int cursor_x, int cursor_y) const
{
    MapCursor pos = getCursor();
    if (!pos) return NULL;

    const Rooms3dCubeSize& sz = pos->size();
    int sx = sz.width() * ROOM_SIZE;
    int sy = sz.height() * ROOM_SIZE;
    int left = getRenderX();
    int top = getRenderY();
    int right = left + sx - 1;
    int bottom = top + sy - 1;
    if (cursor_x >= left && cursor_x <= right && cursor_y >= top && cursor_y <= bottom)
    {
        Rooms3dCubePos p; 
        p.x = (cursor_x - left) / ROOM_SIZE + sz.left;
        p.y = (cursor_y - top) / ROOM_SIZE + sz.top;
        p.z = pos->pos().z;
        return pos->room(p);
    }
    return NULL;
}

void MapperRender::onHScroll(DWORD position)
{
    if (m_hscroll_pos < 0) return;
    m_block_center = true;
    int thumbpos = HIWORD(position);
    int action = LOWORD(position);
    switch (action) {
    case SB_LINEUP:
        m_hscroll_pos = m_hscroll_pos - ROOM_SIZE / 4;
        break;
    case SB_LINEDOWN:
        m_hscroll_pos = m_hscroll_pos + ROOM_SIZE / 4;
        break;
    case SB_PAGEUP:
        m_hscroll_pos = m_hscroll_pos - ROOM_SIZE;
        break;
    case SB_PAGEDOWN:
        m_hscroll_pos = m_hscroll_pos + ROOM_SIZE;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        m_hscroll_pos = thumbpos;
        break;
    }
    if (m_hscroll_pos < 0) m_hscroll_pos = 0;
    else if (m_hscroll_pos > m_hscroll_size) m_hscroll_pos = m_hscroll_size;    
    SetScrollPos(SB_HORZ, m_hscroll_pos);
    Invalidate();
}

void MapperRender::onVScroll(DWORD position)
{
    if (m_vscroll_pos < 0) return;
    m_block_center = true;
    int thumbpos = HIWORD(position);
    int action = LOWORD(position);
    switch (action) {
    case SB_LINEUP:
        m_vscroll_pos = m_vscroll_pos - ROOM_SIZE / 4;
        break;
    case SB_LINEDOWN:
        m_vscroll_pos = m_vscroll_pos + ROOM_SIZE / 4;
        break;
    case SB_PAGEUP:
        m_vscroll_pos = m_vscroll_pos - ROOM_SIZE;
        break;
    case SB_PAGEDOWN:
        m_vscroll_pos = m_vscroll_pos + ROOM_SIZE;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        m_vscroll_pos = thumbpos;
        break;
    }
    if (m_vscroll_pos < 0) m_vscroll_pos = 0;
    else if (m_vscroll_pos > m_vscroll_size) m_vscroll_pos = m_vscroll_size;
    SetScrollPos(SB_VERT, m_vscroll_pos);
    Invalidate();
}

void MapperRender::onSize()
{
    updateScrollbars(true);
    Invalidate();
}

int MapperRender::getRenderX() const
{
    int x = (m_hscroll_flag) ? -m_hscroll_pos : m_hscroll_size - m_hscroll_pos;
    x = x + MAP_EDGE / 2;
    return x;
}

int MapperRender::getRenderY() const
{
    int y = (m_vscroll_flag) ? -m_vscroll_pos : m_vscroll_size - m_vscroll_pos;    
    y = y + MAP_EDGE / 2;
    return y;
}

void MapperRender::updateScrollbars(bool center)
{
/*#ifdef _DEBUG
    char buffer[64];
    int vmin = 0; int vmax = 0;
    GetScrollRange(SB_VERT, &vmin, &vmax);
    int hmin = 0; int hmax = 0;
    GetScrollRange(SB_HORZ, &hmin, &hmax);
    sprintf(buffer, "vpos[%d-%d] = %d(%d), hpos[%d-%d] = %d(%d)\r\n", vmin, vmax, GetScrollPos(SB_VERT), m_vscroll_pos,
        hmin, hmax, GetScrollPos(SB_HORZ), m_hscroll_pos);
    OutputDebugStringA(buffer);
#endif*/

    MapCursor pos = getCursor();
    if (!pos) return;

    const Rooms3dCubeSize& sz = pos->size();

    int width = sz.width()*ROOM_SIZE + MAP_EDGE;
    int height = sz.height()*ROOM_SIZE + MAP_EDGE;

    RECT rc; GetClientRect(&rc);
    int window_width = rc.right;
    int window_height = rc.bottom;

    if (width < window_width)
    {
        m_hscroll_size = window_width - width - 1;
        if (m_hscroll_pos == -1)
            m_hscroll_pos = m_hscroll_size / 2;
        else if (center && !m_block_center)
            m_hscroll_pos = m_hscroll_size / 2;
        m_hscroll_flag = false;
    }
    else
    {
        m_hscroll_size = width - window_width;
        m_hscroll_flag = true;
        m_block_center = false;
    }
    SetScrollRange(SB_HORZ, 0, m_hscroll_size);
    if (m_hscroll_pos < 0) m_hscroll_pos = 0;
    else if (m_hscroll_pos > m_hscroll_size) m_hscroll_pos = m_hscroll_size;
    SetScrollPos(SB_HORZ, m_hscroll_pos);
       
    if (height < window_height)
    {
        m_vscroll_size = window_height - height - 1;
        if (m_vscroll_pos == -1)
            m_vscroll_pos = m_vscroll_size / 2;
        else if (center && !m_block_center)
            m_vscroll_pos = m_vscroll_size / 2;
        m_vscroll_flag = false;
    }
    else
    {
        m_vscroll_size = height - window_height;
        m_vscroll_flag = true;
        m_block_center = false;
    }
    SetScrollRange(SB_VERT, 0, m_vscroll_size);
    if (m_vscroll_pos < 0) m_vscroll_pos = 0;
    else if (m_vscroll_pos > m_vscroll_size) m_vscroll_pos = m_vscroll_size;
    SetScrollPos(SB_VERT, m_vscroll_pos);
}

void MapperRender::mouseLeftButtonDown()
{
    if (!m_drag_mode) {
        m_drag_mode = true;
        GetCursorPos(&m_drag_point);
        SetCapture();
    }
}

void MapperRender::mouseLeftButtonUp()
{
    ReleaseCapture();
    m_drag_mode = false;
}

void MapperRender::mouseMove(int x, int y)
{
    if (!m_drag_mode) return;
    POINT pos;
    GetCursorPos(&pos);

    int dx = pos.x - m_drag_point.x;
    int dy = pos.y - m_drag_point.y;
    m_drag_point = pos;

    m_hscroll_pos = m_hscroll_pos - dx;
    m_vscroll_pos = m_vscroll_pos - dy;

    if (m_hscroll_pos < 0) m_hscroll_pos = 0;
    else if (m_hscroll_pos > m_hscroll_size) m_hscroll_pos = m_hscroll_size;
    if (m_vscroll_pos < 0) m_vscroll_pos = 0;
    else if (m_vscroll_pos > m_vscroll_size) m_vscroll_pos = m_vscroll_size;

    SetScrollPos(SB_HORZ, m_hscroll_pos);
    SetScrollPos(SB_VERT, m_vscroll_pos);
    Invalidate();
}

void MapperRender::mouseLeave()
{
}

void MapperRender::mouseRightButtonDown()
{
    POINT pt; GetCursorPos(&pt);
    int cursor_x = pt.x; 
    int cursor_y = pt.y;
    ScreenToClient(&pt);

    const Room *room = findRoomOnScreen(pt.x, pt.y);
    if (!room)
        return;
    m_menu_tracked_room = room;
    RoomHelper c(room);
    m_menu.SetItemState(MENU_NEWZONE_NORTH, c.isExplored(RD_NORTH));
    m_menu.SetItemState(MENU_NEWZONE_SOUTH, c.isExplored(RD_SOUTH));
    m_menu.SetItemState(MENU_NEWZONE_WEST, c.isExplored(RD_WEST));
    m_menu.SetItemState(MENU_NEWZONE_EAST, c.isExplored(RD_EAST));
    m_menu.SetItemState(MENU_NEWZONE_UP, c.isExplored(RD_UP));
    m_menu.SetItemState(MENU_NEWZONE_DOWN, c.isExplored(RD_DOWN));

    m_menu.SetItemState(MENU_JOINZONE_NORTH, c.isZoneExit(RD_NORTH));
    m_menu.SetItemState(MENU_JOINZONE_SOUTH, c.isZoneExit(RD_SOUTH));
    m_menu.SetItemState(MENU_JOINZONE_WEST, c.isZoneExit(RD_WEST));
    m_menu.SetItemState(MENU_JOINZONE_EAST, c.isZoneExit(RD_EAST));
    m_menu.SetItemState(MENU_JOINZONE_UP, c.isZoneExit(RD_UP));
    m_menu.SetItemState(MENU_JOINZONE_DOWN, c.isZoneExit(RD_DOWN));

    m_menu.SetItemState(MENU_MOVEROOM_NORTH, c.isZoneExit(RD_NORTH));
    m_menu.SetItemState(MENU_MOVEROOM_SOUTH, c.isZoneExit(RD_SOUTH));
    m_menu.SetItemState(MENU_MOVEROOM_WEST, c.isZoneExit(RD_WEST));
    m_menu.SetItemState(MENU_MOVEROOM_EAST, c.isZoneExit(RD_EAST));
    m_menu.SetItemState(MENU_MOVEROOM_UP, c.isZoneExit(RD_UP));
    m_menu.SetItemState(MENU_MOVEROOM_DOWN, c.isZoneExit(RD_DOWN));

    m_menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NOANIMATION, cursor_x - 2, cursor_y - 2, m_hWnd, NULL);
}

void MapperRender::createMenu()
{
    m_icons.Create(16, 16, ILC_COLOR24 | ILC_MASK, 0, 0);
    HANDLE hBmp = LoadImage(NULL, L"plugins\\mapper.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (hBmp)
        m_icons.Add((HBITMAP)hBmp, RGB(128, 0, 128));
    m_menu.CreatePopupMenu();
    if (m_icons.GetImageCount() > 0)
    {
        CMenuXP *pictures = new CMenuXP();
        pictures->CreatePopupMenu();
        for (int i = 0, e = m_icons.GetImageCount(); i <= e; i++)
        {
            if (i == 0)
              { pictures->AppendODMenu(new CMenuXPButton(MENU_SETICON_FIRST, NULL)); continue; }            
            if (i % 6 == 0) pictures->Break();
            pictures->AppendODMenu(new CMenuXPButton(i + MENU_SETICON_FIRST, m_icons.ExtractIcon(i - 1)));
        }
        m_menu.AppendODPopup(pictures, new CMenuXPText(0, L"������"));
        m_menu.AppendSeparator();
    }
    m_menu.AppendODMenu(new CMenuXPText(MENU_SETCOLOR, L"����..."));
    m_menu.AppendODMenu(new CMenuXPText(MENU_RESETCOLOR, L"�������� ����"));
    m_menu.AppendSeparator();

    CMenuXP *newzone = new CMenuXP();
    newzone->CreatePopupMenu();
    newzone->AppendODMenu(new CMenuXPText(MENU_NEWZONE_NORTH, L"�� �����"));
    newzone->AppendODMenu(new CMenuXPText(MENU_NEWZONE_SOUTH, L"�� ��"));
    newzone->AppendODMenu(new CMenuXPText(MENU_NEWZONE_WEST, L"�� �����"));
    newzone->AppendODMenu(new CMenuXPText(MENU_NEWZONE_EAST, L"�� ������"));
    newzone->AppendODMenu(new CMenuXPText(MENU_NEWZONE_UP, L"�����"));
    newzone->AppendODMenu(new CMenuXPText(MENU_NEWZONE_DOWN, L"����"));
    m_menu.AppendODPopup(newzone, new CMenuXPText(0, L"������ ����� ����"));

    CMenuXP *joinzone = new CMenuXP();
    joinzone->CreatePopupMenu();
    joinzone->AppendODMenu(new CMenuXPText(MENU_JOINZONE_NORTH, L"�� �����"));
    joinzone->AppendODMenu(new CMenuXPText(MENU_JOINZONE_SOUTH, L"�� ��"));
    joinzone->AppendODMenu(new CMenuXPText(MENU_JOINZONE_WEST, L"�� �����"));
    joinzone->AppendODMenu(new CMenuXPText(MENU_JOINZONE_EAST, L"�� ������"));
    joinzone->AppendODMenu(new CMenuXPText(MENU_JOINZONE_UP, L"�����"));
    joinzone->AppendODMenu(new CMenuXPText(MENU_JOINZONE_DOWN, L"����"));
    m_menu.AppendODPopup(joinzone, new CMenuXPText(0, L"������� ����"));

    CMenuXP *moveroom = new CMenuXP();
    moveroom->CreatePopupMenu();
    moveroom->AppendODMenu(new CMenuXPText(MENU_MOVEROOM_NORTH, L"�� �����"));
    moveroom->AppendODMenu(new CMenuXPText(MENU_MOVEROOM_SOUTH, L"�� ��"));
    moveroom->AppendODMenu(new CMenuXPText(MENU_MOVEROOM_WEST, L"�� �����"));
    moveroom->AppendODMenu(new CMenuXPText(MENU_MOVEROOM_EAST, L"�� ������"));
    moveroom->AppendODMenu(new CMenuXPText(MENU_MOVEROOM_UP, L"�����"));
    moveroom->AppendODMenu(new CMenuXPText(MENU_MOVEROOM_DOWN, L"����"));
    m_menu.AppendODPopup(moveroom, new CMenuXPText(0, L"��������� ������� � ����"));
}

bool MapperRender::runMenuPoint(DWORD wparam, LPARAM lparam)
{
    WORD id = LOWORD(wparam);
    if (id == MENU_SETCOLOR)
    {
        COLORREF color = m_menu_tracked_room->color;
        if (!m_menu_tracked_room->use_color)
            color = RGB(180, 180, 180);
        CColorDialog dlg(color, CC_FULLOPEN, m_hWnd);
        if (dlg.DoModal() == IDOK)
        {
            m_menu_tracked_room->color = dlg.GetColor();
            m_menu_tracked_room->use_color = 1;
            Invalidate();
        }
        return true;
    }

    if (id == MENU_RESETCOLOR)
    {
        m_menu_tracked_room->color = 0;
        m_menu_tracked_room->use_color = 0;
        Invalidate();
        return true;
    }

    if (id >= MENU_SETICON_FIRST && id <= MENU_SETICON_LAST)
    {
        int icon = id - MENU_SETICON_FIRST;
        m_menu_tracked_room->icon = icon;
        Invalidate();
        return true;
    }
    if (m_menu_handler)
    {
        ::PostMessage(m_menu_handler, WM_COMMAND, wparam, lparam);
        return true;
    }
    return false;
}
