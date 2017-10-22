#ifndef __MAIN_H_
#define __MAIN_H_ 

#include <lua.hpp>

#ifndef _DLL_EXPORT
#define _DLL_EXPORT __declspec(dllexport)
#endif
extern "C" _DLL_EXPORT int luaopen_zlib(lua_State *L);
#endif