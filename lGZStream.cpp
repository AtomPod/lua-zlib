#include "lGZStream.h"
#include <zlib.h>
#include <vector>

#ifndef __Reg_Pair_
#define __Reg_Pair_(name) \
		{#name , name} 
#endif

#ifndef __Is_Digit
#define __Is_Digit(c) (((c) >= '0' && (c) <= '9'))
#endif

#ifndef __Is_Pos
#define __Is_Pos(c) (((c) == '+') || ((c) == '-'))
#endif

const int lGZStream::seek_beg = (int)'b' + (int)'e' + (int)'g';
const int lGZStream::seek_cur = (int)'c' + (int)'u' + (int)'r';
const int lGZStream::seek_end = (int)'e' + (int)'n' + (int)'d';

const char *lGZStream::gzlibname = "gzlib-register";
luaL_Reg lGZStream::_GZ_metatable[] = {
	{"__gc" , destroy } ,
	{ nullptr , nullptr }
};

luaL_Reg lGZStream::_GZ_method[] = {
	__Reg_Pair_(close) ,
	__Reg_Pair_(read) ,
	__Reg_Pair_(write) ,
	__Reg_Pair_(flush) ,
	__Reg_Pair_(seek) ,
	__Reg_Pair_(tell) ,
	__Reg_Pair_(rewind) ,
	__Reg_Pair_(eof) , 
	{ nullptr , nullptr }
};

static int set1params(gzFile file , lua_State *L) {
	lua_Integer buflen = luaL_checkinteger(L, 3);
	return gzbuffer(file, buflen);
}

static int set2params(gzFile file , lua_State *L) {
	if (set1params(file, L) == -1) {
		return Z_STREAM_ERROR;
	}

	lua_Integer level = luaL_checkinteger(L, 4);
 	return gzsetparams(file, level , Z_DEFAULT_STRATEGY);
}

static int set3params(gzFile file , lua_State *L) {
	if (set1params(file, L) == -1) {
		return Z_STREAM_ERROR;
	}

	lua_Integer level = luaL_checkinteger(L, 4);
	lua_Integer strategy = luaL_checkinteger(L, 5);
 	return gzsetparams(file, level , strategy);
}

 int lGZStream::open(lua_State *L) {

 	/*
	path 
	mode 
	buffer
	level
	strategy
 	*/

 	const char *path = luaL_checkstring(L, 1);
 	const char *mode = luaL_optstring(L, 2, "wr");

 	gzFile file = gzopen(path , mode);

 	if (file == nullptr) {
 		lua_pushnil(L);
 		lua_pushfstring(L, "gzlib-file:%s No such file or directory" , path);
 		return 2;
 	}

 	int top = lua_gettop(L);
 	int params = top - 2;

 	int ret = Z_OK;
 	switch(params) {
 		case 0:
 			break;
 		case 1:
 			ret = set1params(file, L);
 			break;
 		case 2:
 			ret = set2params(file, L);
 			break;
 		case 3:
 		default:
 			ret = set3params(file, L);
 			break;
 	}

 	if (ret != Z_OK) {
 		gzclose(file);
 		lua_pushnil(L);
 		lua_pushstring(L, "gz-file: set params failed");
 		return 2;
 	}

 	gzFile* lfile = reinterpret_cast<gzFile*>(lua_newuserdata(L, sizeof(gzFile*)));
 	*lfile = file;

 	luaL_getmetatable(L, gzlibname);
 	lua_setmetatable(L, -2);
 	return 1;
 }

 int lGZStream::destroy(lua_State *L) {
 	// gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, -1, gzlibname));
 	
 	// if (*lfile != nullptr) {
 	// 	gzclose(*lfile);
 	// 	*lfile = nullptr;
 	// } 

 	return lGZStream::close(L);
 }

 int lGZStream::close(lua_State *L){
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, -1, gzlibname));
 	
 	if (*lfile != nullptr) {
 		gzclose(*lfile);
 		*lfile = nullptr;
 	}
 	return 0;
 }

 const char *readAll(gzFile file , 
 						std::vector<char> &vbuffer ,
 						char *buf , ssize_t len ,
 						ssize_t &rlen) {
 	ssize_t rbytes = len;
 	char *rbuffer = buf;
 	int ret = gzread(file, rbuffer, rbytes);
 	
 	if ( gzeof(file) ) {
 		rlen = ret;
 		return rbuffer;
 	}

 	ssize_t rtotal = ret;
 	ssize_t trbytes = rbytes * 3 / 2;
 	
 	vbuffer.clear();
 	vbuffer.reserve(trbytes);
 	vbuffer.insert(vbuffer.end(), rbuffer, rbuffer + rtotal);

 	while( !gzeof(file) ) {
 		vbuffer.resize(trbytes);
 		size_t readsize = vbuffer.size() - rbytes;
 		rtotal += gzread(file, &vbuffer[rbytes], readsize);
 		rbytes = trbytes;
 		trbytes = rbytes * 3 / 2;
 	} 
 	
 	rlen = rtotal;
 	return vbuffer.data();
 }

 const char *readNumber(gzFile file , ssize_t number , std::vector<char> &vbuffer ,
 								char *buf , ssize_t len ,
 								ssize_t &rlen) {
 	char *rbyte = number > len ? (vbuffer.resize(number) , vbuffer.data()) : buf;
 	rlen = gzread(file, rbyte, number);
 	return rbyte;
 }

 const char *readLine(gzFile file , std::vector<char> &linebuffer , ssize_t &len) {
 	size_t newsize = 4 * 1024;
 	size_t length = 0;
 	int c = '\0';

 	while (c != EOF && c != '\n') {
 		linebuffer.resize(newsize);
 		while ( length < linebuffer.size() && ((c = gzgetc(file)) != EOF && c != '\n')) {
 			linebuffer[length++] = c;
 		}
 		newsize = newsize * 3 / 2;
 	}
 	len = length;
 	return linebuffer.data();
 }

 const char *readToNumber(lua_State *L , gzFile file , char *buf , ssize_t len , lua_Number &number) {

 	ssize_t length = 0;
 	ssize_t maxlength = len - 1;
 	int c = gzgetc(file);

 	if ( !__Is_Digit(c) && !__Is_Pos(c) ) {
 		gzungetc(c, file);
 		return nullptr;
 	}


 	bool floatpoint = false;
 	buf[length++] = c;
 	while ( length < maxlength && 
 			(((c = gzgetc(file) ) != EOF && __Is_Digit(c)) ||
 			(c == '.' && !floatpoint)
 			)) {
 		floatpoint = floatpoint ? floatpoint : (c == '.');
 		buf[length++] = c;
 	}
 	buf[length] = '\0';

 	if (length == 0) {
 		return nullptr;
 	}

 	return lua_stringtonumber(L, buf) != 0 ? buf : nullptr;
 }

 int lGZStream::read(lua_State *L){ 
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, 1, gzlibname));
 
 	if (*lfile == nullptr) {
 		luaL_error(L, "gzlib-stream: invalid file");
 		return 0;
 	}

 	int type = lua_type(L, 2);

 	char smallbuffer[1024] = {0};
 	std::vector<char> temp;
 	ssize_t rbytes = 0;
 	const char *dest = nullptr;

 	switch (type) {
 		case LUA_TNUMBER: {
 			ssize_t size = 0;

			if (lua_isinteger(L, 2)) {
				size = lua_tointeger(L, 2);
			} else {
				size = lua_tonumber(L, 2);
			}

			dest = readNumber(*lfile, size, temp ,
							smallbuffer, sizeof(smallbuffer) , rbytes);
			break;
 		}
 		case LUA_TSTRING:
 		case LUA_TNIL:
 		case LUA_TNONE: {
 			const char *op_t = luaL_optstring(L, 2, "*l");
 			if (*op_t != '*') {
 				break;
 			}
 			++op_t;
 			switch (*op_t) {

 				case 'a': {
 					dest = readAll(*lfile, temp , smallbuffer , 
 									sizeof(smallbuffer) , rbytes);
 					break;
 				}

 				case 'l' : {
 					dest = readLine(*lfile, temp , rbytes);
 					break;
 				}

 				case 'n' : {
 					lua_Number n = 0;
 					dest = readToNumber(L , *lfile , smallbuffer , 
 									sizeof(smallbuffer) , n);
 					if (dest == nullptr) {
 						lua_pushstring(L, "gzlib-read: invalid number");
 					}
 					return dest == nullptr ? 1 : 2;
 				}

 				default: {
					break;
 				}
 			}
 		}
 		default:
 			break;
 	}

 	if (dest == nullptr) {
 		lua_pushnil(L);
		lua_pushstring(L, "gzlib-read: invalid options");
		return 2;
 	}

 	lua_pushlstring(L, dest, rbytes);
 	return 1;
 }

 int lGZStream::write(lua_State *L){ 
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, 1, gzlibname));

 	if (*lfile == nullptr) {
 		luaL_error(L, "gzlib-stream: invalid file");
 		return 0;
 	}

 	size_t length = 0;
 	char smallbuffer[1024] = {0}; 
 	const char *wbuffer = nullptr;
 	switch(lua_type(L , 2)) {
 		case LUA_TNUMBER: {
 			if (lua_isnumber(L, 2)) {
 				lua_Number number = lua_tonumber(L, 2);
 				length = lua_number2str(smallbuffer , sizeof(smallbuffer) , number);
 			} else {
 				lua_Integer integer = lua_tointeger(L, 2);
 				length = lua_integer2str(smallbuffer, sizeof(smallbuffer), integer);
 			}
 			wbuffer = smallbuffer;
 			break;
 		}
 		case LUA_TSTRING: {
 			wbuffer = lua_tolstring(L , 2 , &length);
 			break;
 		}
 		default:
 			break;
 	}

 	if (wbuffer != nullptr) {
 		int wbytes = gzwrite(*lfile, wbuffer, length);
 		lua_pushinteger(L, wbytes);
 		return 1;
 	}

 	return 0;
 }

 int lGZStream::flush(lua_State *L){ 
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, 1, gzlibname));

 	if (*lfile == nullptr) {
 		luaL_error(L, "gzlib-stream: invalid file");
 		return 0;
 	}

 	int flush_op = luaL_optinteger(L, 2, Z_FULL_FLUSH);
 	int ret = gzflush(*lfile, flush_op);
 	lua_pushinteger(L, ret);
 	return 1;
 }
 int lGZStream::seek(lua_State *L){ 
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, 1, gzlibname));

 	if (*lfile == nullptr) {
 		luaL_error(L, "gzlib-stream: invalid file");
 		return 0;
 	}
 	size_t len = 0;
 	const char *whencestr = luaL_optlstring(L, 2, "cur" , &len);
 	lua_Integer whence = 0;

 	if (len < 3) {
 		luaL_error(L, "invalid option %s" , whencestr);
 		return 0;
 	}

 	int pos = (*whencestr) + *(whencestr + 1) + *(whencestr + 2);
 	switch(pos) {
 		case lGZStream::seek_beg:
 			whence = SEEK_SET;
 			break;
 		case lGZStream::seek_cur:
 			whence = SEEK_CUR;
 			break;
 		case lGZStream::seek_end:
 		default:
 			luaL_error(L, "invalid option %s" , whencestr);
 			return 0;
 	}

 	lua_Integer offset = luaL_optinteger(L, 3, 0);

 	z_off_t roffset = gzseek(*lfile, offset, whence);
 	lua_pushinteger(L, roffset);
 	return 1;
 }
 int lGZStream::tell(lua_State *L){ 
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, 1, gzlibname));

 	if (*lfile == nullptr) {
 		luaL_error(L, "gzlib-stream: invalid file");
 		return 0;
 	}

 	z_off_t tellsize = gztell(*lfile);
 	lua_pushinteger(L, tellsize);
 	return 1;
 }
 int lGZStream::rewind(lua_State *L){ 
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, 1, gzlibname));

 	if (*lfile == nullptr) {
 		luaL_error(L, "gzlib-stream: invalid file");
 		return 0;
 	}

 	int ret = gzrewind(*lfile);
 	lua_pushinteger(L, ret);
 	return 1;
 }
 int lGZStream::eof(lua_State *L){ 
 	gzFile* lfile = reinterpret_cast<gzFile*>(luaL_checkudata(L, 1, gzlibname));

 	if (*lfile == nullptr) {
 		luaL_error(L, "gzlib-stream: invalid file");
 		return 0;
 	}

 	int ret = gzeof(*lfile);
 	lua_pushboolean(L, ret);
 	return 1;
 }
 // int lGZStream::gzerror(lua_State *L){ 

 // }
 bool lGZStream::createGZMetatable(lua_State *L){ 
 	luaL_newmetatable(L, lGZStream::gzlibname);	
 	for (luaL_Reg *b = _GZ_metatable; b->name != nullptr; ++b){
 		lua_pushcfunction(L, b->func);
 		lua_setfield(L, -2, b->name);
 	}

 	luaL_newlib(L, _GZ_method);
 	lua_setfield(L, -2, "__index");
 	lua_pop(L, 1);
 	return true;
 }
