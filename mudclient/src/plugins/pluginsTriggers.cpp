#include "stdafx.h"
#include "pluginsApi.h"
#include "pluginsTriggers.h"
#include "pluginsParseData.h"
#include "accessors.h"
extern Plugin* _cp;

PluginsTrigger::PluginsTrigger() : L(NULL), p(NULL), m_current_compare_pos(0), m_enabled(false), m_triggered(false)
{
}

PluginsTrigger::~PluginsTrigger()
{
    m_parseData.strings.clear();
    m_trigger_func_ref.unref(L);
}

bool PluginsTrigger::init(lua_State *pl, Plugin *pp)
{
    assert(pl && pp);
    L = pl; 
    p = pp;
    if (luaT_check(L, 2, LUA_TSTRING, LUA_TFUNCTION) ||
        luaT_check(L, 2, LUA_TTABLE, LUA_TFUNCTION))
    {
        m_trigger_func_ref.createRef(L);  // save function on the top of stack
        if (lua_isstring(L, 1))
        {
            m_compare_objects.resize(1);
            tstring key(luaT_towstring(L, 1));
            if (!m_compare_objects[0].init(key, true))
                return false;
        }
        else
        {
            std::vector<tstring> keys;
            lua_pushnil(L);                     // first key
            while (lua_next(L, -2) != 0)        // key index = -2, value index = -1
            {
                if (!lua_isstring(L, -1))
                    return false;
                tstring key(luaT_towstring(L, -1));
                keys.push_back(key);
                lua_pop(L, 1);
            }
            bool all_empty = true;
            for (int i=0,e=keys.size();i<e;++i) {
                if (!keys[i].empty()) { all_empty = false; break; }
            }
            if (all_empty) 
                return false;
            m_compare_objects.resize(keys.size());
            for (int i=0,e=keys.size();i<e;++i) 
            {
                tstring k = keys[i];
                if (k.empty()) { k.assign(L"%%"); }
                if (!m_compare_objects[i].init(k, true))
                    return false;
            }
        }
        int count = m_compare_objects.size();
        m_triggerParseData.init(count);
        m_enabled = true;
        return true;
    }
    return false;
}

void PluginsTrigger::reset()
{
    m_current_compare_pos = 0;
    m_parseData.strings.clear();
    m_parseData.update_prev_string = false;
    m_parseData.last_finished = true;
    m_triggerParseData.reset();
    m_triggered = false;
}

void PluginsTrigger::enable(bool enable)
{
    if (enable != m_enabled)
        reset();
    m_enabled = enable;
}

bool PluginsTrigger::isEnabled() const
{
    return m_enabled;
}

int PluginsTrigger::getLen() const
{
    return m_compare_objects.size();
}

bool PluginsTrigger::compare(const CompareData& cd, bool incompl_flag)
{
    if (!m_enabled)
        return false;
    if (m_triggered)
        reset();
    bool result = false;
    CompareObject &co = m_compare_objects[m_current_compare_pos];
    if (incompl_flag && co.isFullstrReq()) {
        // not compared / full string req.
    }
    else {
        result = co.compare(cd.fullstr);
    }
    if (result)
    {
        m_parseData.strings.push_back(cd.string);
        m_parseData.last_finished = !incompl_flag;

        triggerParseDataString* tpd = m_triggerParseData.get(m_current_compare_pos);
        co.getParameters(&tpd->params);
        cd.string->getMd5(&tpd->crc);

        int last = m_compare_objects.size() - 1;
        if (m_current_compare_pos == last)
        {
            m_triggered = true;
            return true;
        }
        m_current_compare_pos++;
        return false;
    }

    if (m_current_compare_pos > 0)
    {
        reset();
        return compare(cd, incompl_flag);
    }
    return false;
}

void PluginsTrigger::run()
{
    m_trigger_func_ref.pushValue(L);

    PluginsParseData ppd(&m_parseData, &m_triggerParseData);
    luaT_pushobject(L, &ppd, LUAT_VIEWDATA);
    Plugin *oldcp = _cp;
    _cp = p;
    if (lua_pcall(L, 1, 0, 0))
    {
        //error
        if (luaT_check(L, 1, LUA_TSTRING))
            pluginError(luaT_towstring(L, -1));
        else
            pluginError(L"����������� ������");
        lua_settop(L, 0);
    }
    _cp = oldcp;
    reset();
}

int trigger_create(lua_State *L)
{
    if (luaT_check(L, 2, LUA_TSTRING, LUA_TFUNCTION) ||
        luaT_check(L, 2, LUA_TTABLE, LUA_TFUNCTION))
    {
        PluginsTrigger *t = new PluginsTrigger();
        if (!t->init(L, _cp))
            { delete t; }
        else
        {
            _cp->triggers.push_back(t);
            luaT_pushobject(L, t, LUAT_TRIGGER);
            return 1;
        }
    }
    return pluginInvArgs(L, L"createTrigger");
}

int trigger_enable(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_TRIGGER))
    {
        PluginsTrigger *t = (PluginsTrigger*)luaT_toobject(L, 1);
        t->enable(true);
        return 0;
    }
    return pluginInvArgs(L, L"trigger:enable");
}

int trigger_disable(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_TRIGGER))
    {
        PluginsTrigger *t = (PluginsTrigger*)luaT_toobject(L, 1);
        t->enable(false);
        return 0;
    }
    return pluginInvArgs(L, L"trigger:disable");
}

int trigger_isEnabled(lua_State *L)
{
    if (luaT_check(L, 1, LUAT_TRIGGER))
    {
        PluginsTrigger *t = (PluginsTrigger*)luaT_toobject(L, 1);
        lua_pushboolean(L, t->isEnabled() ? 1 : 0);
        return 1;
    }
    return pluginInvArgs(L, L"trigger:isEnabled");
}

void reg_mt_trigger(lua_State *L)
{
    lua_register(L, "createTrigger", trigger_create);
    luaL_newmetatable(L, "trigger");
    regFunction(L, "enable", trigger_enable);
    regFunction(L, "disable", trigger_disable);
    regFunction(L, "isEnabled", trigger_isEnabled);
    regIndexMt(L);
    lua_pop(L, 1);
}