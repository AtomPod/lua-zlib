#include "main.h"
#include "lZlibStream.h"

int luaopen_zlib(lua_State *L) {
	lZlibStream::createZlibMetatable(L);
	lZlibStream::newZlibLibrary(L);
	return 1;
};