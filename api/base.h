#pragma once

#pragma warning(disable: 4996)

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#pragma comment(lib, "lua.lib")

#include <stdlib.h>
#include <assert.h>
#include <string>

class lua_pushwstring
{
public:
    lua_pushwstring(lua_State* L, const wchar_t* string) {
      int buffer_required = WideCharToMultiByte(CP_UTF8, 0, string, -1, NULL, 0, NULL, NULL);
      char *buffer = new char [buffer_required+1];
      WideCharToMultiByte(CP_UTF8, 0, string, -1, buffer, buffer_required, NULL, NULL);
      buffer[buffer_required] = 0;
      lua_pushstring(L, buffer);
      delete []buffer;
    }
};

class lua_towstring
{
    wchar_t *buffer;
public:
    lua_towstring(lua_State* L, int index) : buffer(NULL) {
      if (!lua_isstring(L, index)) return;
      const char* string = lua_tostring(L, index);
      int buffer_required = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
      buffer = new wchar_t[buffer_required+1];
      MultiByteToWideChar(CP_UTF8, 0, string, -1, buffer, buffer_required);
      buffer[buffer_required] = 0;
    }
    ~lua_towstring() { delete buffer; }
    operator const wchar_t*() const { return buffer; }
};

class wstring_to_int
{
    int result;
public:
    wstring_to_int(const wchar_t *string, bool *check = NULL) : result(0) {
        bool converted = false;
        if (string) {
          int len = wcslen(string);
          if (len > 0)
          {
              const wchar_t *p = string;
              if (*p == L'-') { p++; len--;}
              if (wcsspn(p, L"0123456789") == len)
                { result = _wtoi(string); converted = true; }
          }
        }
        if (check) *check = converted;
    }
    operator int() const { return result; }
};

class int_to_wstring 
{
    wchar_t buffer[16];
public:
    int_to_wstring(int index) { _itow(index, buffer, 10); }
    operator const wchar_t*() const { return buffer; }
};

class lua_ref
{
    mutable int ref;
public:
    lua_ref() : ref(LUA_NOREF) {}
    lua_ref(const lua_ref& r) { ref = r.ref; r.ref = LUA_NOREF; }
    lua_ref& operator=(const lua_ref& r) { ref = r.ref; r.ref = LUA_NOREF; }
    ~lua_ref() { assert(ref==LUA_NOREF); }
    void createRef(lua_State *L) { assert(ref==LUA_NOREF); ref=luaL_ref(L, LUA_REGISTRYINDEX); }
    void pushValue(lua_State *L) { lua_rawgeti(L, LUA_REGISTRYINDEX, ref); }
    void unref(lua_State *L) { luaL_unref(L, LUA_REGISTRYINDEX, ref); ref=LUA_NOREF; }
};

class lua_format
{
public:
    void format(lua_State* L, int index, std::wstring *result)
    {
        int i = index;
        int type = lua_type(L, i);
        wchar_t dbuf[32];
        result->clear();

        switch (type)
        {
        case LUA_TNIL:
            result->append(L"nil");
            break;
        case LUA_TNUMBER:
            swprintf(dbuf, L"number: %d", lua_tointeger(L, i));
            result->append(dbuf);
            break;
        case LUA_TBOOLEAN:
            swprintf(dbuf, L"boolean: %s", (lua_toboolean(L, i) == 0) ? "false" : "true");
            result->append(dbuf);
            break;
        case LUA_TSTRING:
            result->append(L"string: ");
            result->append(lua_towstring(L, i));
            break;
        case LUA_TUSERDATA:
            swprintf(dbuf, L"userdata: 0x%p", lua_topointer(L, i));
            result->append(dbuf);
            break;
        case LUA_TLIGHTUSERDATA:
            swprintf(dbuf, L"lightuserdata: 0x%p", lua_topointer(L, i));
            result->append(dbuf);
            break;
        case LUA_TFUNCTION:
            swprintf(dbuf, L"function: 0x%p", lua_topointer(L, i));
            result->append(dbuf);
            break;
        case LUA_TTHREAD:
            swprintf(dbuf, L"thread: 0x%p", lua_topointer(L, i));
            result->append(dbuf);
            break;
        case LUA_TTABLE:
            swprintf(dbuf, L"table: 0x%p", lua_topointer(L, i));
            result->append(dbuf);
            break;
        default:
            result->append(L"unknown");
            break;
        }
    }
};

class lua_dumpparams
{
    std::wstring dump;
public:
    lua_dumpparams(lua_State *L, const wchar_t* text) 
    {
        lua_format lf;
        if (text) dump.append(text);
        dump.append(L" (");
        int n = lua_gettop(L);
        for (int i = 1; i <= n; ++i)
        {
            if (i != 1)
                dump.append(L",");
            std::wstring result;
            lf.format(L, i, &result);
            dump.append(result);
        }
        dump.append(L")");
    }
    operator const wchar_t*() const { return dump.c_str(); }
};

class lua_toerror
{
    std::wstring error;
public:
    lua_toerror(lua_State *L)
    {
        if (!lua_isstring(L, -1))
            return;
        const char* s = lua_tostring(L, -1);
        const char* p = strchr(s, ':');
        if (p) {
        const char* b = p+1;
        const char *e = strchr(b, ':');
        if (e && e!=b) {
          bool number = true;
          for (const char *t=b; t!=e; ++t)
          {
              if (*t>='0' && *t<='9') {}
              else { number = false; break; }
          }
          if (number) {
            std::string fname(s, p-s);
            std::string err(p);
            std::wstring f, e;
            convert_ansi(fname, f);
            convert_utf8(err, e);
            f.append(e);
            error.swap(f);
            return;
          }
        }}
        std::string err(s);
        convert_utf8(err, error);
    }
    operator const wchar_t*() const { return error.c_str(); }
private:
    void convert_ansi(const std::string& s, std::wstring& res)
    {
       int buffer_required = MultiByteToWideChar(CP_ACP, 0, s.c_str(), s.length(), NULL, 0);
       res.resize(buffer_required);
       MultiByteToWideChar(CP_ACP, 0, s.c_str(), s.length(), &res[0], buffer_required);
    }
    void convert_utf8(const std::string& s, std::wstring& res)
    {
       int buffer_required = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.length(), NULL, 0);
       res.resize(buffer_required);
       MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.length(), &res[0], buffer_required);
    }
};