#include "stdafx.h"
#include "../res/resource.h"
#include "properties.h"
#include "mapper.h"
#include "debugHelpers.h"
#include "mapperUnitTests.h"

bool map_active = false;
PropertiesMapper m_props;
luaT_window m_parent_window;
Mapper* m_mapper_window = NULL;
//-------------------------------------------------------------------------
int get_name(lua_State *L) 
{
    luaT_pushwstring(L, L"�����");
    return 1;
}

int get_description(lua_State *L) 
{
    luaT_pushwstring(L,
        L"���������� ����� ������ � �������. ���������� �������������� ������.\r\n"
        L"������������ ��� ����� � 6 ������������ ��������.\r\n"
        L"������� ��� ������ ��������� MSDP ��������� �� ���-������� c\r\n"
        L"������� � ��������������."
        );
    return 1;
}

int get_version(lua_State *L)
{
    luaT_pushwstring(L, L"0.03 beta");
    return 1;
}

int init(lua_State *L)
{
#ifdef _DEBUG
    MapperUnitTests t;
    t.runTests();
#endif

    DEBUGINIT(L);
	init_clientlog(L);
    luaT_run(L, "addButton", "dds", IDB_MAP, 1, L"���� � ������");

    m_props.initAllDefault();

	tstring error;
    tstring path;
	tstring current_zone;
    base::getPath(L, L"settings.xml", &path);
    xml::node p;
    if (p.load(path.c_str(), &error))
    {
        int width = 0;
        p.get(L"zoneslist/width", &width);
        m_props.zoneslist_width = (width > 0) ? width : -1;
		if (p.get(L"lastzone/name", &current_zone))
            m_props.current_zone = current_zone;
	} else {
		if (!error.empty())
			base::log(L, error.c_str());
	}
    p.deletenode();

	if (!m_parent_window.create(L, L"�����", 400, 400))
		return luaT_error(L, L"�� ������� ������� ���� ��� �����");

    HWND parent = m_parent_window.hwnd();    
    map_active = m_parent_window.isVisible();

    tstring folder;
    base::getPath(L, L"", &folder);

    m_mapper_window = new Mapper(&m_props, folder);
    RECT rc; ::GetClientRect(parent, &rc);
    if (rc.right == 0) rc.right = 400; // requeires for splitter inside map window (if parent window hidden)
    HWND res = m_mapper_window->Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);  
    m_parent_window.attach(res);
    m_parent_window.block(L"left,right,top,bottom");
    if (map_active)
        luaT_run(L, "checkMenu", "d", 1);

    //todo! m_mapper_window->loadMaps();
    return 0;
}

int release(lua_State *L)
{
    //todo! m_mapper_window->saveMaps();

    xml::node p(L"settings");
    p.set(L"zoneslist/width", m_props.zoneslist_width);
	p.set(L"lastzone/name", m_props.current_zone);

    tstring path;
    base::getPath(L, L"settings.xml", &path);
    if (!p.save(path.c_str()))
    {
        tstring error(L"������ ������ �������� ������������: ");
        error.append(path);
        base::log(L, error.c_str());
    }
    p.deletenode();
    return 0;
}

int menucmd(lua_State *L)
{
    if (!luaT_check(L, 1, LUA_TNUMBER))
        return 0;
    int id = lua_tointeger(L, 1);
    lua_pop(L, 1);
    if (id == 1)
    {
        if (map_active)
        {
            luaT_run(L, "uncheckMenu", "d", 1);
            m_parent_window.hide();
        }
        else
        {
            luaT_run(L, "checkMenu", "d", 1);
            m_parent_window.show();
        }
        map_active = !map_active;
    }
    return 0;
}

int closewindow(lua_State *L)
{
    if (!luaT_check(L, 1, LUA_TNUMBER))
        return 0;
    HWND hwnd = (HWND)lua_tointeger(L, 1);
    if (hwnd == m_parent_window.hwnd())
    {
        luaT_run(L, "uncheckMenu", "d", 1);
        m_parent_window.hide();
        map_active = false;
    }
    return 0;
}

void msdpstate(lua_State *L, bool state)
{
    luaT_Msdp m(L);
    std::vector<std::wstring> reports1;
    std::vector<std::wstring> reports2;
    reports1.push_back(L"ROOM");
    //reports2.push_back(L"MOVEMENT");
    if (state)
    {
        m.report(reports1); //todo!
        //m.report(reports2);
    }
    else
    {
        m.unreport(reports1);
        //m.unreport(reports2);
    }
}

int msdpon(lua_State *L)
{
    msdpstate(L, true);
    return 0;
}

int msdpoff(lua_State *L)
{
    msdpstate(L, false);
    return 0;
}

bool popString(lua_State *L, const char* name, tstring* val)
{
    lua_pushstring(L, name);
    lua_gettable(L, -2);
    val->assign(luaT_towstring(L, -1));
    lua_pop(L, 1);
    return (val->empty()) ? false : true;
}

int msdp(lua_State *L)
{
    luaT_showLuaStack(L, L"begin");
    RoomData rd;
    bool inconsistent_data = true;
    if (luaT_check(L, 1, LUA_TTABLE))
    {
        lua_pushstring(L, "ROOM");
        lua_gettable(L, -2);
        if (lua_istable(L, -1))
        {
            if (popString(L, "AREA", &rd.zonename) &&
                popString(L, "VNUM", &rd.vnum) &&
                popString(L, "NAME", &rd.name))
            {
                lua_pushstring(L, "EXITS");
                lua_gettable(L, -2);
                if (lua_istable(L, -1))
                {
                    lua_pushnil(L);
                    while (lua_next(L, -2) != 0)        // key index = -2, value index = -1
                    {
                        tstring dir(luaT_towstring(L, -2));
                        tstring vnum(luaT_towstring(L, -1));
                        rd.exits[dir] = vnum;
                        lua_pop(L, 1);
                    }
                    inconsistent_data = false;
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }
    luaT_showLuaStack(L, L"end");
    luaT_showTableOnTop(L, L"end");
    if (!inconsistent_data)
    {
        m_mapper_window->processMsdp(rd);
        return 1;
    }
    assert(false);
    return 1;
}

static const luaL_Reg mapper_methods[] = 
{
    {"name", get_name},
    {"description", get_description},
    {"version", get_version},
    {"init", init },
    {"release", release },
    {"menucmd", menucmd },
    {"closewindow", closewindow },
    {"msdpon", msdpon },
    {"msdpoff", msdpoff },
    {"msdp", msdp },
    { NULL, NULL }
};

int WINAPI plugin_open(lua_State *L)
{
    luaL_newlib(L, mapper_methods);
    return 1;
}

CAppModule _Module;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        _Module.Init(NULL, hModule);
        break;
    case DLL_PROCESS_DETACH:
        delete m_mapper_window;
        _Module.Term();
        break;
    }
    return TRUE;
}
