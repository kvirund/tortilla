#include "stdafx.h"
#include "pluginsApi.h"
#include "pluginSupport.h"
#include "plugin.h"
#include "MainFrm.h"

extern CMainFrame _wndMain;
extern Plugin* _cp;

int createpanel(lua_State *L)
{
    PluginData &p = find_plugin();
    if (luaT_check(L, 2, LUA_TSTRING, LUA_TNUMBER))
    {
        PanelWindow w;
        w.size = lua_tointeger(L, 2);
        w.side = _wndMain.m_gameview.convertSideFromString(TU2W(lua_tostring(L, 1)));
        if (IsDocked(w.side))
        {
            PluginsView *window = _wndMain.m_gameview.createPanel(w, _cp);
            if (window)
                _cp->panels.push_back(window);
            luaT_pushobject(L, window, LUAT_PANEL);
            return 1;
        }
    }
    return pluginInvArgs(L, "createPanel");
}

int pn_attach(lua_State *L)
{
    if (luaT_check(L, 2, LUAT_PANEL, LUA_TNUMBER))
    {
        PluginsView *v = (PluginsView *)luaT_toobject(L, 1);
        HWND child = (HWND)lua_tointeger(L, 2);
        v->attachChild(child);
        return 0;
    }
    return pluginInvArgs(L, "panel.attach");
}

int pn_setrender(lua_State *L)
{
    if (luaT_check(L, 2, LUAT_PANEL, LUA_TFUNCTION))
    {
        PluginsView *v = (PluginsView *)luaT_toobject(L, 1);
        PluginsViewRender* r = v->setRender(L);
        luaT_pushobject(L, r, LUAT_RENDER);
        return 1;
    }
    return pluginInvArgs(L, "panel.setrender");
}

int pn_hwnd(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_PANEL))
    {
        PluginsView *v = (PluginsView *)luaT_toobject(L, 1);
        HWND wnd = *v;
        lua_pushunsigned(L, (unsigned int)wnd);
        return 1;
    }
    return pluginInvArgs(L, "panel.hwnd");
}

void reg_mt_panels(lua_State *L)
{
    lua_register(L, "createPanel", createpanel);
    luaL_newmetatable(L, "panel");
    regFunction(L, "attach", pn_attach);
    regFunction(L, "setrender", pn_setrender);
    regFunction(L, "hwnd", pn_hwnd);
    regIndexMt(L);
    lua_pop(L, 1);
}
