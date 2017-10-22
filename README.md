# lua-zlib

需要:
  lua5.3
  zlib

local zlib = require("zlib")

压缩函数: 数据 , 压缩等级

	zlib.deflate(string , [default: Z_DEFAULT_COMPRESSION]) return string(压缩后数据) , error(错误代码)

解压函数： 数据 , 解压后的数据大小

	zlib.inflate(string , [default: 16 * 1024]) return string(压缩后数据) , error(错误代码)

创建GZIP文件对象 与 io.open 相同 ， 除了不支持seek的"end"

	zlib.createGZipFile(path , [mode])

创建压缩对象: 初始化表

	zlib.createDeflate([default: DeflateOptions]) return DeflateObject

	  可只指定部分参数或都不指定(使用默认)
  
	  DeflateOptions {
	      level (Z_DEFAULT_COMPRESSION)
	      method (Z_DEFLATED)
	      windowBits (15)
	      memLevel (MAX_MEM_LEVEL)
	      strategy (Z_DEFAULT_STRATEGY)
	      memory (16 * 1024)
	      flushmask (Z_NO_FLUSH)
	      finishflushmask (Z_FINISH)
	  };

创建解压对象：初始化表

	zlib.createInflate([default: InflateOptions]) return InflateObject

  	可只指定部分参数或都不指定(使用默认)
  
	  InflateOptions {
	    windowBits (15)
	    memory (16 * 1024)
	    flushmask (Z_NO_FLUSH)
	    finishflushmask (Z_FINISH)
	  };


压缩解压对象函数:

	  添加压缩或解压数据：数据

		会写入缓冲区，并不一定马上会做出操作，所以可能会返回nil

		(DeflateObject|InflateObject):write (string) return string([nil 或 解(压)缩后数据]) , error(错误代码)


	  强制刷新缓冲区： 刷新类型

		(DeflateObject|InflateObject):flush ([default: Z_FULL_FLUSH])  return string([nil 或 解(压)缩后数据]) , error(错误代码)


	  结束压缩或解压数据: 数据

		刷新缓冲区，返回数据 ， 解压数据可以不需要调用，但如果需要验证数据是否完成可以调用

			(DeflateObject|InflateObject):end (string) return string([nil 或 解(压)缩后数据]) , error(错误代码)

常量:

	zlib.constants = {

		刷新方式: （flush）

		  Z_NO_FLUSH ,

		  Z_PARTIAL_FLUSH ,

		  Z_SYNC_FLUSH ,

		  Z_FULL_FLUSH ,

		  Z_FINISH ,

		  Z_BLOCK ,

		  Z_TREES ,

		错误代码: (error)

		  Z_OK ,

		  Z_STREAM_END ,

		  Z_NEED_DICT ,

		  Z_ERRNO ,

		  Z_STREAM_ERROR ,

		  Z_DATA_ERROR ,

		  Z_MEM_ERROR ,

		  Z_BUF_ERROR ,

		  Z_VERSION_ERROR ,

		压缩等级: (level)

		  Z_NO_COMPRESSION ,

		  Z_BEST_SPEED ,

		  Z_BEST_COMPRESSION ,

		  Z_DEFAULT_COMPRESSION ,

		压缩方式：(strategy)

		  Z_FILTERED ,

		  Z_HUFFMAN_ONLY ,

		  Z_RLE ,

		  Z_FIXED ,

		  Z_DEFAULT_STRATEGY ,

		 位: （windowBits ，只是参考值）

		  ZF_Deflate = -(MAX_WBITS),

		  ZF_Zlib = MAX_WBITS,

		  ZF_GZ = MAX_WBITS + 16

		}
