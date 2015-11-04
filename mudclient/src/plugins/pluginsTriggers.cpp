#include "stdafx.h"
#include "pluginsApi.h"
#include "pluginsTriggers.h"
#include "accessors.h"
extern Plugin* _cp;

PluginsTrigger::PluginsTrigger() : L(NULL), m_tigger_func_index(0), m_enabled(false)
{
}

PluginsTrigger::~PluginsTrigger()
{
    m_enabled = false;
    lua_getglobal(L, "_triggers");
    if (lua_istable(L, -1) && m_tigger_func_index > 0)
    {
        lua_pushinteger(L, m_tigger_func_index);
        lua_pushnil(L);
        lua_settable(L, -3);
    }
    lua_pop(L, 1);
}

bool PluginsTrigger::init(lua_State *pL)
{
    L = pL;
    assert(luaT_check(L, 2, LUA_TSTRING, LUA_TFUNCTION));

    lua_getglobal(L, "_triggers");
    if (!lua_istable(L, -1))
    {
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setglobal(L, "_triggers");
    }

    tstring key(luaT_towstring(L, 1));
    m_compare.init(key, true);

    lua_len(L, -1);
    int index = lua_tointeger(L, -1) + 1;
    lua_pop(L, 1);
    lua_insert(L, -2);
    lua_pushinteger(L, index);
    lua_insert(L, -2);
    lua_settable(L, -3);
    lua_pop(L, 1);
    m_tigger_func_index = index;
    m_enabled = true;
    return true;
}

bool PluginsTrigger::compare(const CompareData& cd, bool incompl_flag)
{
    if (!m_enabled)
        return false;
    if (incompl_flag && m_compare.isFullstrReq())
        return false;
    if (!m_compare.compare(cd.fullstr))
        return false;
    lua_getglobal(L, "_triggers");
    lua_pushinteger(L, m_tigger_func_index);
    lua_gettable(L, -2);
    lua_insert(L, -2);
    lua_pop(L, 1);

    PluginsTriggerString vs(cd.string, m_compare);
    luaT_pushobject(L, &vs, LUAT_VIEWSTRING);
    if (lua_pcall(L, 1, 0, 0))
    {
        if (luaT_check(L, 1, LUA_TSTRING))
            pluginError("trigger", lua_tostring(L, -1));
        else
            pluginError("trigger", "����������� ������");
        lua_settop(L, 0);
    }
    return true;
}

void PluginsTrigger::enable(bool enable)
{
    m_enabled = enable;
}

int trigger_create(lua_State *L)
{
    if (luaT_check(L, 2, LUA_TSTRING, LUA_TFUNCTION))
    {
        PluginsTrigger *t = new PluginsTrigger();
        if (!t->init(L))
            { delete t;  t = NULL; }
        else
            { _cp->triggers.push_back(t); }
        luaT_pushobject(L, t, LUAT_TRIGGER);
        return 1;
    }
    return pluginInvArgs(L, "createTrigger");
}

int trigger_enable(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_TRIGGER))
    {
        PluginsTrigger *t = (PluginsTrigger*)luaT_toobject(L, 1);
        t->enable(true);
        return 0;
    }
    return pluginInvArgs(L, "trigger:enable");
}

int trigger_disable(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_TRIGGER))
    {
        PluginsTrigger *t = (PluginsTrigger*)luaT_toobject(L, 1);
        t->enable(false);
        return 0;
    }
    return pluginInvArgs(L, "trigger:disable");
}

void reg_mt_trigger_string(lua_State *L);
void reg_mt_trigger(lua_State *L)
{
    lua_register(L, "createTrigger", trigger_create);
    luaL_newmetatable(L, "trigger");
    regFunction(L, "enable", trigger_enable);
    regFunction(L, "disable", trigger_disable);
    regIndexMt(L);
    lua_pop(L, 1);
    reg_mt_trigger_string(L);
}

int ts_getBlocksCount(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        lua_pushinteger(L, s->string()->blocks.size());
        return 1;
    }
    return pluginInvArgs(L, "viewstring:getBlocksCount");
}

int ts_getText(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        tstring text;
        s->string()->getText(&text);
        lua_pushwstring(L, text.c_str() );
        return 1;
    }
    return pluginInvArgs(L, "viewstring:getText");
}

int ts_getParameter(lua_State *L)
{
    if (luaT_check(L, 2, LUAT_VIEWSTRING, LUA_TNUMBER))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        tstring p;
        if (!s->getParam( lua_tointeger(L, 2), &p))
            lua_pushnil(L);
        else
            lua_pushwstring(L, p.c_str());
        return 1;
    }
    return pluginInvArgs(L, "viewstring:getParameter");
}

int ts_getParamsCount(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        lua_pushinteger(L, s->getParamsCount());
        return 1;
    }
    return pluginInvArgs(L, "viewstring:getParamsCount");
}

int ts_getComparedText(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        tstring p;
        if (!s->getCompared(&p))
            lua_pushnil(L);
        else
            lua_pushwstring(L, p.c_str());
        return 1;
    }
    return pluginInvArgs(L, "viewstring:getParamsCount");
}

int ts_isPrompt(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        lua_pushboolean(L, (s->string()->prompt > 0) ? 1 : 0);
        return 1;
    }
    return pluginInvArgs(L, "viewstring:isPrompt");
}

int ts_getPrompt(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        if (s->string()->prompt <= 0)
            lua_pushnil(L);
        else {
            tstring text;
            s->string()->getPrompt(&text);
            lua_pushwstring(L, text.c_str());
        }
        return 1;
    }
    return pluginInvArgs(L, "viewstring:getPrompt");
}

int ts_isGameCmd(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        lua_pushboolean(L, s->string()->gamecmd ? 1 : 0);
        return 1;
    }
    return pluginInvArgs(L, "viewstring:isGameCmd");
}

int ts_isSystem(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        lua_pushboolean(L, s->string()->system ? 1 : 0);
        return 1;
    }
    return pluginInvArgs(L, "viewstring:isSystem");
}

int ts_drop(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        s->string()->dropped = true;
        return 0;
    }
    return pluginInvArgs(L, "viewstring:drop");
}

int ts_deleteBlock(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING, LUA_TNUMBER))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        MudViewString *vs = s->string();
        bool ok = false;
        int size = vs->blocks.size();
        int index = lua_tointeger(L, 2);
        if (index >= 1 && index <= size)
        {
            index = index - 1;
            vs->blocks.erase(vs->blocks.begin() + index);
            ok = true;        
        }
        lua_pushboolean(L, ok ? 1 : 0);
        return 0;
    }
    return pluginInvArgs(L, "viewstring:deleteBlock");
}

int ts_deleteAllBlocks(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_VIEWSTRING))
    {
        PluginsTriggerString *s = (PluginsTriggerString*)luaT_toobject(L, 1);
        s->string()->blocks.clear();
        return 0;
    }
    return pluginInvArgs(L, "viewstring:drop");
}

int vd_gettype(const utf8* type);
tbyte _check(unsigned int val, unsigned int min, unsigned int max);
int ts_get(lua_State *L)
{
    if (luaT_check(L, 3, LUAT_VIEWSTRING, LUA_TNUMBER, LUA_TNUMBER)||
        luaT_check(L, 3, LUAT_VIEWSTRING, LUA_TNUMBER, LUA_TSTRING))
    {
        int type = 0;
        if (lua_isnumber(L, 3))
            type = lua_tointeger(L, 3);
        else
        {
            type = vd_gettype(lua_tostring(L, 3));
            if (type == -1)
                return pluginInvArgs(L, "viewstring:get");
        }

        bool ok = false;
        PluginsTriggerString *s = (PluginsTriggerString *)luaT_toobject(L, 1);
        MudViewString* str = s->string();
        {
            int block = lua_tointeger(L, 2);
            int size = str->blocks.size();
            if (block >= 1 && block <= size)
            {
                MudViewStringParams &p = str->blocks[block-1].params;
                ok = true;
                switch (type)
                {
                    case luaT_ViewData::TEXTCOLOR:
                        if (p.use_ext_colors)
                            ok = false;
                        else
                        {
                            tbyte color = p.text_color;
                            if (color <= 7 && p.intensive_status) color += 8;
                            lua_pushunsigned(L, color);
                        }
                        break;
                    case luaT_ViewData::BKGCOLOR:
                        if (p.use_ext_colors)
                            ok = false;
                        else
                            lua_pushunsigned(L, p.bkg_color);
                        break;
                    case luaT_ViewData::UNDERLINE:
                        lua_pushunsigned(L, p.underline_status);
                        break;
                    case luaT_ViewData::ITALIC:
                        lua_pushunsigned(L, p.italic_status);
                        break;
                    case luaT_ViewData::BLINK:
                        lua_pushunsigned(L, p.blink_status);
                        break;
                    case luaT_ViewData::REVERSE:
                        lua_pushunsigned(L, p.reverse_video);
                        break;
                    case luaT_ViewData::EXTTEXTCOLOR:
                        if (p.use_ext_colors)
                            lua_pushunsigned(L, p.ext_text_color);
                        else
                            ok = false;
                        break;
                    case luaT_ViewData::EXTBKGCOLOR:
                        if (p.use_ext_colors)
                            lua_pushunsigned(L, p.ext_bkg_color);
                        else
                            ok = false;
                        break;
                    default:
                        ok = false;
                        break;
                }
            }
        }
        if (!ok)
            lua_pushnil(L);
        return 1;
    }
    return pluginInvArgs(L, "viewstring:get");
}

int ts_set(lua_State *L)
{
    if (luaT_check(L, 4, LUAT_VIEWDATA, LUA_TNUMBER, LUA_TNUMBER, LUA_TNUMBER) || 
        luaT_check(L, 4, LUAT_VIEWDATA, LUA_TNUMBER, LUA_TSTRING, LUA_TNUMBER))
    {
        int type = 0;
        if (lua_isnumber(L, 3))
            type = lua_tointeger(L, 3);
        else
        {
            type = vd_gettype(lua_tostring(L, 3));
            if (type == -1)
                return pluginInvArgs(L, "viewstring:set");
        }

        bool ok = false;
        PluginsTriggerString *s = (PluginsTriggerString *)luaT_toobject(L, 1);
        MudViewString* str = s->string();
        {
            int block = lua_tointeger(L, 2);
            int size = str->blocks.size();
            if (block >= 1 && block <= size)
            {
                unsigned int v = lua_tounsigned(L, 4);
                MudViewStringParams &p = str->blocks[block-1].params;
                ok = true;
                switch (type)
                {
                case luaT_ViewData::TEXTCOLOR:
                    if (p.use_ext_colors)
                        p.bkg_color = 0;
                    p.use_ext_colors = 0;
                    p.intensive_status = 0;
                    p.text_color = _check(v, 0, 255);
                    break;
                case luaT_ViewData::BKGCOLOR:
                    if (p.use_ext_colors)
                        p.text_color = 7;
                    p.use_ext_colors = 0;
                    p.intensive_status = 0;
                    p.bkg_color = _check(v, 0, 255);
                    break;                
                case luaT_ViewData::UNDERLINE:
                    p.underline_status = _check(v, 0, 1);
                    break;
                case luaT_ViewData::ITALIC:
                    p.italic_status = _check(v, 0, 1);
                    break;
                case luaT_ViewData::BLINK:
                    p.blink_status = _check(v, 0, 1);
                    break;
                case luaT_ViewData::REVERSE:
                    p.reverse_video = _check(v, 0, 1);
                    break;
                case luaT_ViewData::EXTTEXTCOLOR:
                    if (!p.use_ext_colors)
                        p.ext_bkg_color = tortilla::getPalette()->getColor(p.bkg_color);
                    p.use_ext_colors = 1;
                    p.ext_text_color = v;
                    break;
                case luaT_ViewData::EXTBKGCOLOR:
                    if (!p.use_ext_colors)
                        p.ext_text_color = tortilla::getPalette()->getColor(p.text_color);
                    p.use_ext_colors = 1;
                    p.ext_bkg_color = v;
                    break;
                default:
                    ok = false;
                    break;
                }
            }
        }
        lua_pushboolean(L, ok ? 1 : 0);
        return 1;
    }
    return pluginInvArgs(L, "viewstring:set");
}

void reg_mt_trigger_string(lua_State *L)
{
    luaL_newmetatable(L, "viewstring");
    regFunction(L, "getBlocksCount", ts_getBlocksCount);
    regFunction(L, "getText", ts_getText);
    regFunction(L, "getParamsCount", ts_getParamsCount);
    regFunction(L, "getParameter", ts_getParameter);
    regFunction(L, "getComparedText", ts_getComparedText);
    regFunction(L, "isPrompt", ts_isPrompt);
    regFunction(L, "getPrompt", ts_getPrompt);
    regFunction(L, "isGameCmd", ts_isGameCmd);
    regFunction(L, "isSystem", ts_isSystem);
    regFunction(L, "drop", ts_drop);
    regFunction(L, "deleteBlock", ts_deleteBlock);
    regFunction(L, "deleteAllBlocks", ts_deleteAllBlocks);
    regFunction(L, "get", ts_get);
    regFunction(L, "set", ts_set);
    regIndexMt(L);
    lua_pop(L, 1);
}
