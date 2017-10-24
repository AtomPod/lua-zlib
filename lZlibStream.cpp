#include "lZlibStream.h"
#include <lua.hpp>
#include <zlib.h>
#include "ZlibStream.h"
#include "lGZStream.h"

struct LZStream {
	ZBase *stream;
	int index;
};

struct EnumPair{
	const char *name;
	int value;
};

#ifndef __Enum_Value_
#define __Enum_Value_(e) {#e , e}
#endif

enum ZlibFormat
{
	ZF_Deflate = -(MAX_WBITS) ,
	ZF_Zlib = MAX_WBITS ,
	ZF_GZ = MAX_WBITS + 16
};

static EnumPair ZlibEnumPair[] = {
	__Enum_Value_(Z_NO_FLUSH) ,
	__Enum_Value_(Z_PARTIAL_FLUSH) ,
	__Enum_Value_(Z_SYNC_FLUSH) ,
	__Enum_Value_(Z_FULL_FLUSH) ,
	__Enum_Value_(Z_FINISH) ,
	__Enum_Value_(Z_BLOCK) ,
	__Enum_Value_(Z_TREES) ,
	__Enum_Value_(Z_OK) ,
	__Enum_Value_(Z_STREAM_END) ,
	__Enum_Value_(Z_NEED_DICT) ,
	__Enum_Value_(Z_ERRNO) ,
	__Enum_Value_(Z_STREAM_ERROR) ,
	__Enum_Value_(Z_DATA_ERROR) ,
	__Enum_Value_(Z_MEM_ERROR) ,
	__Enum_Value_(Z_BUF_ERROR) ,
	__Enum_Value_(Z_VERSION_ERROR) ,
	__Enum_Value_(Z_NO_COMPRESSION) ,
	__Enum_Value_(Z_BEST_SPEED) ,
	__Enum_Value_(Z_BEST_COMPRESSION) ,
	__Enum_Value_(Z_DEFAULT_COMPRESSION) ,
	__Enum_Value_(Z_FILTERED) ,
	__Enum_Value_(Z_HUFFMAN_ONLY) ,
	__Enum_Value_(Z_RLE) ,
	__Enum_Value_(Z_FIXED) ,
	__Enum_Value_(Z_DEFAULT_STRATEGY) ,
	__Enum_Value_(ZF_Deflate) ,
	__Enum_Value_(ZF_Zlib) ,
	__Enum_Value_(ZF_GZ)
};

const char *lZlibStream::zlibname = "zlib-register";
const char *lZlibStream::constants = "constants";

class ZlibMethod
{
public:

	static const Deflate::DeflateOptions defaultDeflateOptions;

	static Deflate::DeflateOptions extends_tableToDeflateOptions(lua_State *L , int index) {
		Deflate::DeflateOptions op = {0};

		int b = lua_gettop(L);

		lua_getfield(L, index, "level"); 
		op.level = luaL_optinteger(L, -1 , defaultDeflateOptions.level);

		lua_getfield(L, index, "method"); 
		op.method = luaL_optinteger(L, -1 , defaultDeflateOptions.method);

		lua_getfield(L, index, "windowBits"); 
		op.windowBits = luaL_optinteger(L, -1 , defaultDeflateOptions.windowBits);

		lua_getfield(L, index, "memLevel"); 
		op.memLevel = luaL_optinteger(L, -1 , defaultDeflateOptions.memLevel);

		lua_getfield(L, index, "strategy"); 
		op.strategy = luaL_optinteger(L, -1 , defaultDeflateOptions.strategy);

		lua_getfield(L, -1, "memory"); 
		op.memory = luaL_optinteger(L, -1 , defaultDeflateOptions.memory);
		op.omemory = op.memory;

		lua_getfield(L, index, "flushmask"); 
		op.flushmask = luaL_optinteger(L, -1 , defaultDeflateOptions.flushmask);

		lua_getfield(L, index, "finishflushmask"); 
		op.finishflushmask = luaL_optinteger(L, -1 , defaultDeflateOptions.finishflushmask);

		int e = lua_gettop(L);

		lua_pop(L, e - b);
		return op;
	}

	static const Inflate::InflateOptions defaultInflateOptions;

	static Inflate::InflateOptions extends_tableToInflateOptions(lua_State *L , int index) {
		Inflate::InflateOptions op = {0};

		int b = lua_gettop(L);

		lua_getfield(L, index, "windowBits"); 
		op.windowBits = luaL_optinteger(L, -1 , defaultInflateOptions.windowBits);
		lua_getfield(L, index, "memory"); 
		op.memory = luaL_optinteger(L, -1 , defaultInflateOptions.memory);
		op.omemory = op.memory;

		lua_getfield(L, index, "flushmask"); 
		op.flushmask = luaL_optinteger(L, -1 , defaultInflateOptions.flushmask);

		lua_getfield(L, index, "finishflushmask"); 
		op.finishflushmask = luaL_optinteger(L, -1 , defaultInflateOptions.finishflushmask);
		
		int e = lua_gettop(L);
		lua_pop(L, e - b);

		return op;
	}
public:
	static int destroy(lua_State *L) {
		LZStream *ppStream =
		reinterpret_cast<LZStream*>(luaL_checkudata(L, 1, lZlibStream::zlibname));
		luaL_unref(L, LUA_REGISTRYINDEX, ppStream->index);
		delete ppStream->stream;
		return 0;
	}

	static int tostring(lua_State *L) {
		LZStream *ppStream =
		reinterpret_cast<LZStream*>(luaL_checkudata(L, 1, lZlibStream::zlibname));

		ZBase *tstream = ppStream->stream;
		size_t t = tstream->getStream().type;
		lua_pushstring(L, t == ZlibStream::StreamType::Def ? 
					"zlib-stream: Deflate" : "zlib-stream: Inflate");
		return 1;
	}

	static int write(lua_State *L) {
		LZStream *ppStream =
		reinterpret_cast<LZStream*>(luaL_checkudata(L, 1, lZlibStream::zlibname));

		ZBase *wstream = ppStream->stream;
		int ref = ppStream->index;

		size_t len = 0;
		const char *buf = luaL_checklstring(L, 2, &len);

		int btop = lua_gettop(L);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		lua_getfield(L, -1, "ondata");
		int absindex = lua_absindex(L, -1);
		lua_getfield(L, -2, "onend");
		int absindexend = lua_absindex(L, -1);

		ssize_t err = wstream->write(buf , len , [L , absindex , absindexend](const ZlibStream::Buffer &buffer , ssize_t err) -> ssize_t {
			ssize_t ret = buffer.size();
			ssize_t args = 2;
			lua_pushvalue(L, err == Z_STREAM_END ? absindexend : absindex);
			lua_pushvalue(L, 1);

			if ((err == Z_OK || err == Z_STREAM_END)) {
				lua_pushnil(L);
				lua_pushlstring(L, buffer.data(), buffer.size());
				args = 3;
			} else {
				lua_pushinteger(L, err);
				ret = -1;
			}
			
			lua_pcall(L, args, 0, 0);
			return ret;
		});

		int etop = lua_gettop(L);

		lua_pop(L, etop - btop);
		return 0;
	}

	static int flush(lua_State *L) {
		LZStream *ppStream =
		reinterpret_cast<LZStream*>(luaL_checkudata(L, 1, lZlibStream::zlibname));

		ZBase *fstream = ppStream->stream;
		int ref = ppStream->index;

		size_t t = fstream->getStream().type;
		ssize_t kind = luaL_optinteger(L, 2, Z_FULL_FLUSH);
		
		int btop = lua_gettop(L);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		lua_getfield(L, -1, "ondata");
		int absindex = lua_absindex(L, -1);
		lua_getfield(L, -2, "onend");
		int absindexend = lua_absindex(L, -1);

		ssize_t err = fstream->flush(kind, [L , absindex , absindexend](const ZlibStream::Buffer &buffer , ssize_t err) -> ssize_t {
			ssize_t ret = buffer.size();
			ssize_t args = 2;
			lua_pushvalue(L, err == Z_STREAM_END ? absindexend : absindex);
			lua_pushvalue(L, 1);
			if ((err == Z_OK || err == Z_STREAM_END)) {
				lua_pushnil(L);
				lua_pushlstring(L, buffer.data(), buffer.size());
				args = 3;
			} else {
				lua_pushinteger(L, err);
				ret = -1;
			}
			
			lua_pcall(L, args, 0, 0);
			return ret;
		});

		int etop = lua_gettop(L);

		lua_pop(L, etop - btop);
		return 0;
	}

	static int end(lua_State *L) {
		LZStream *ppStream =
		reinterpret_cast<LZStream*>(luaL_checkudata(L, 1, lZlibStream::zlibname));

		ZBase *fstream = ppStream->stream;
		int ref = ppStream->index;

		size_t len = 0;
		const char *buf = luaL_optlstring(L, 2, nullptr, &len);
		
		int btop = lua_gettop(L);
		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		lua_getfield(L, -1, "ondata");
		int absindex = lua_absindex(L, -1);
		lua_getfield(L, -2, "onend");
		int absindexend = lua_absindex(L, -1);

		ssize_t err = fstream->end(buf, len, [L , absindex , absindexend](const ZlibStream::Buffer &buffer , ssize_t err) -> ssize_t {
			ssize_t ret = buffer.size();
			ssize_t args = 2;
			lua_pushvalue(L, err == Z_STREAM_END ? absindexend : absindex);
			lua_pushvalue(L, 1);
			if ((err == Z_OK || err == Z_STREAM_END)) {
				lua_pushnil(L);
				lua_pushlstring(L, buffer.data(), buffer.size());
				args = 3;
			} else {
				lua_pushinteger(L, err);
				ret = -1;
			}
			
			lua_pcall(L, args, 0, 0);
			return ret;
		});

		int etop = lua_gettop(L);

		lua_pop(L, etop - btop);
		return 0;
	}

	static int createDeflate(lua_State *L) {

		if (!lua_istable(L, 1)) {
			lua_pushnil(L);
			return 1;
		}

		lua_pushvalue(L, 1);
		int ref = luaL_ref(L, LUA_REGISTRYINDEX);

		Deflate::DeflateOptions nop = defaultDeflateOptions;
		if (lua_istable(L, 2)) {
			nop = extends_tableToDeflateOptions(L , 2);
		}
		
		LZStream *pStream = reinterpret_cast<LZStream*>(lua_newuserdata(L, sizeof(LZStream*)));
		pStream->stream = new Deflate(nop);
		pStream->index = ref;
		luaL_getmetatable(L, lZlibStream::zlibname);
		lua_setmetatable(L, -2);
		return 1;
	}

	static int createInflate(lua_State *L) {
		if (!lua_istable(L, 1)) {
			lua_pushnil(L);
			return 1;
		}

		lua_pushvalue(L, 1);
		int ref = luaL_ref(L, LUA_REGISTRYINDEX);

		Inflate::InflateOptions nop = defaultInflateOptions;
		if (lua_istable(L, 2)) {
			nop = extends_tableToInflateOptions(L , 2);
		}

		LZStream *pStream = reinterpret_cast<LZStream*>(lua_newuserdata(L, sizeof(LZStream*)));
		pStream->stream = new Inflate(nop);
		pStream->index = ref;

		luaL_getmetatable(L, lZlibStream::zlibname);
		lua_setmetatable(L, -2);
		return 1;
	}

	static int lcompress(lua_State *L) {
		size_t len = 0;
		const char *buf = luaL_checklstring(L, 1, &len);
		lua_Integer level = luaL_optinteger(L, 2, Z_DEFAULT_COMPRESSION);
		ZlibStream::Buffer dest;
		ssize_t ret = Deflate::compress(buf, len , dest, level);
		if (ret == Z_OK) {
			lua_pushlstring(L, dest.data(), dest.size());
			lua_pushinteger(L, ret);
		} else {
			lua_pushnil(L);
			lua_pushinteger(L, ret);
		}
		return 2;
	}

	static int luncompress(lua_State *L) {
		size_t len = 0;
		const char *buf = luaL_checklstring(L, 1, &len);
		size_t blen = (len * 3) / 2;
		size_t minbound = defaultInflateOptions.memory;
		size_t range = blen > minbound ? blen : minbound;

		lua_Integer buffersize = luaL_optinteger(L, 2, range);
		ssize_t ret = Z_OK;
		ZlibStream::Buffer buffer;
		do {
			buffer.resize(buffersize);
			ret = Inflate::uncompress(buf, len, buffer.data(), buffer.size());
			buffersize = (buffersize * 3) / 2;
		} while (ret == Z_BUF_ERROR);

		if (ret == Z_OK) {
			lua_pushlstring(L, buffer.data(), buffer.size());
		} else {
			lua_pushnil(L);
		}	
		lua_pushinteger(L , ret);
		return 2;
	}
public:
	static luaL_Reg _meta_methods[]; 
	static luaL_Reg _stream_methods[];
	static luaL_Reg _methods[];
};

const Deflate::DeflateOptions ZlibMethod::defaultDeflateOptions = {
	Z_DEFAULT_COMPRESSION ,
	Z_DEFLATED ,
	15 ,
	MAX_MEM_LEVEL ,
	Z_DEFAULT_STRATEGY,
	16 * 1024 ,
	16 * 1024 ,
	Z_NO_FLUSH,
	Z_FINISH
};

const Inflate::InflateOptions ZlibMethod::defaultInflateOptions = {
	15 ,
	16 * 1024 ,
	16 * 1024 ,
	Z_NO_FLUSH,
	Z_FINISH
};

luaL_Reg ZlibMethod::_meta_methods[] = {
	{ "__gc" , destroy } ,
	{ "__tostring" , tostring } , 
 	{ nullptr , nullptr }
};

luaL_Reg ZlibMethod::_stream_methods[] = {
	{ "write" , write } , 
	{ "flush" , flush } ,
	{ "finish" , end } ,
	{ nullptr , nullptr }
};

luaL_Reg ZlibMethod::_methods[] = {
	{ "createDeflate" , createDeflate } ,
	{ "createInflate" , createInflate } ,
	{ "deflate" , lcompress } ,
	{ "inflate" , luncompress } ,
	{ "createGZipFile" , lGZStream::open } ,
	{ nullptr , nullptr }
};

int lZlibStream::createZlibMetatable(lua_State *L) {

	luaL_newmetatable(L, zlibname);

	for (luaL_Reg *b = ZlibMethod::_meta_methods; b->name != nullptr; ++b) {
		lua_pushcfunction(L, b->func);
		lua_setfield(L, -2, b->name);
	}

	luaL_newlib(L, ZlibMethod::_stream_methods);
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1);

	lGZStream::createGZMetatable(L);
	return 0;
}

int lZlibStream::newZlibLibrary(lua_State *L) {
	luaL_newlib(L, ZlibMethod::_methods);
	lua_newtable(L);
	for (int i = 0; i != sizeof(ZlibEnumPair) / sizeof(ZlibEnumPair[0]); ++i)
	{
		lua_pushinteger(L, ZlibEnumPair[i].value);
		lua_setfield(L, -2, ZlibEnumPair[i].name);
	}

	lua_setfield(L, -2, constants);
	return 1;
}
