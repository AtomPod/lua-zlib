#ifndef __Z_LIB_STREAM_H_
#define __Z_LIB_STREAM_H_ 
#include <zlib.h>
#include <vector>
#include <functional>

class ZlibStream
{
public:
	enum StreamType {
		Def ,
		Inf
	};
	typedef std::vector<char> Buffer;
	typedef std::function<ssize_t(const Buffer & , ssize_t)> StreamDataCallback;
public:
	ZlibStream();
	~ZlibStream();
public:
	z_stream _stream;
	StreamType type;
	Buffer input;
	Buffer output;
};

class ZBase
{
public:
	ZBase(int flushMask , int finishFlushMask);
	virtual ~ZBase();
public:
	ZlibStream &getStream() { return _Stream; }
	int getFlushMask() const { return _FlushMask; }
	int getFinishFlushMask() const { return _FinishFlushMask; }
	ssize_t write(const char *buffer , size_t size ,
				 const ZlibStream::StreamDataCallback &ondata);
	ssize_t flush(const ZlibStream::StreamDataCallback &ondata);
	ssize_t flush(int kind , const ZlibStream::StreamDataCallback &ondata);
	ssize_t end(const char *buffer , size_t size , 
				const ZlibStream::StreamDataCallback &ondata);
	void reset();
protected:
	ssize_t _writeBuffer(const char *buffer , size_t size ,
				 int f , const ZlibStream::StreamDataCallback &ondata);
	ssize_t _flushBuffer(int f , const ZlibStream::StreamDataCallback &ondata);
private:
	ZlibStream _Stream;
	int _FlushMask;
	int _FinishFlushMask;

};

class Deflate : public ZBase
{
public:
	struct DeflateOptions {
		int level;
		int method;
		int windowBits;
		int memLevel;
		int strategy;
		int memory;
		int omemory;
		int flushmask;
		int finishflushmask;
	};
public:
	Deflate(const DeflateOptions &options);
	Deflate(int level , int method , 
			int windowBits , int memLevel , 
			int strategy , int memory = 16 * 1024, int omemory = 16 * 1024,
			int flushmask = Z_NO_FLUSH , int finishflushmask = Z_FINISH);
	virtual ~Deflate();
public:
	static ssize_t compressBound(size_t s);
	static ssize_t compress(const char *src , size_t ssize , 
							char *dest , uLong &dsize , int level);
	static ssize_t compress(const char *src , size_t ssize , 
							ZlibStream::Buffer &buffer , int level);
};

class Inflate : public ZBase
{
public:
	struct InflateOptions {
		int windowBits;
		int memory;
		int omemory;
		int flushmask;
		int finishflushmask;
	};
public:
	Inflate(const InflateOptions &options);
	Inflate(int windowBits , int memory = 16 * 1024, int omemory = 16 * 1024,
			int flushmask = Z_NO_FLUSH , int finishflushmask = Z_FINISH);
	virtual ~Inflate();
public:
	static ssize_t uncompress(const char *src , size_t ssize , char *dest , uLong &dsize);
	static ssize_t uncompress(const char *src , size_t ssize , char *dest , uLong dsize);
};


#endif