#ifndef __L_ZLIB_STREAM_H_
#define __L_ZLIB_STREAM_H_ 
#include <lua.hpp>

class lZlibStream
{
public:
	static int createZlibMetatable(lua_State *L);
	static int newZlibLibrary(lua_State *L);
public:
	static const char *zlibname;
    static const char *constants;
};

#endif