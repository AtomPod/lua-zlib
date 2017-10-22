#ifndef __L_GZ_STREAM_H_
#define __L_GZ_STREAM_H_
#include <lua.hpp>

class lGZStream
{
public:
	static int open(lua_State *L);
	static int destroy(lua_State *L);
	static int close(lua_State *L);
	static int read(lua_State *L);
	static int write(lua_State *L);
	static int flush(lua_State *L);
	static int seek(lua_State *L);
	static int tell(lua_State *L);
	static int rewind(lua_State *L);
	static int eof(lua_State *L);
	//static int gzerror(lua_State *L);

	static bool createGZMetatable(lua_State *L);
public:
	static luaL_Reg _GZ_metatable[];
	static luaL_Reg _GZ_method[];
	static const char *gzlibname;
	static const int seek_beg;
	static const int seek_cur;
	static const int seek_end;
};

#endif