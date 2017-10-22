#include "ZlibStream.h"

ZlibStream::ZlibStream() {

}

ZlibStream::~ZlibStream() {

}


ZBase::ZBase(int flushMask , int finishFlushMask) :
			_FlushMask(flushMask) , _FinishFlushMask(finishFlushMask) {

}
ZBase::~ZBase() {

}

ssize_t ZBase::_writeBuffer(const char *buffer , size_t size ,int f
					 , const ZlibStream::StreamDataCallback &ondata) {

	ZlibStream &stream = getStream();
	auto &InputStream = stream.input;
	int finishFlushMask = getFinishFlushMask();
	int flushMask = getFlushMask();

	size_t remain = InputStream.capacity() - InputStream.size();
	ssize_t error = Z_OK;
	ssize_t ffl = f == finishFlushMask ? flushMask : f;

	while (size >= remain) {
		size_t delta = remain;
		InputStream.insert(InputStream.end(), buffer, buffer + delta);
		error = _flushBuffer(ffl , ondata);
		remain = InputStream.capacity();
		size -= delta;
		buffer += delta;
	}

	InputStream.insert(InputStream.end(), buffer, buffer + size);
		
	if (finishFlushMask == f) {
		error = _flushBuffer(f, ondata);
	} 

	return error;
}

ssize_t ZBase::_flushBuffer(int f
					 , const ZlibStream::StreamDataCallback &ondata) {
	ZlibStream &stream = getStream();
	auto &Stream = stream._stream;
	auto &OutputStream = stream.output;
	auto &InputStream  = stream.input;

	// size_t outputSize = stream.type == ZlibStream::StreamType::Def ?
	// 					deflateBound(&stream._stream, InputStream.size()) :
	// 					OutputStream.size();
						
	// outputSize = std::max(outputSize, OutputStream.size());
	// OutputStream.resize(outputSize);
	size_t outputSize = OutputStream.size();

	Stream.next_in = (Bytef*)InputStream.data();
	Stream.avail_in = InputStream.size();
	ssize_t ret = Z_OK;

	auto flate = stream.type == ZlibStream::StreamType::Def ?
				deflate : inflate;

	do {
		Stream.next_out = (Bytef*)OutputStream.data();
		Stream.avail_out = OutputStream.size();
		ret = flate(&Stream , f);
		ssize_t have = OutputStream.size() - Stream.avail_out;
		OutputStream.resize(have);
		if ( ondata(OutputStream , ret) != have )
			return Z_ERRNO;
		OutputStream.resize(outputSize);
	} while (Stream.avail_out == 0);

	InputStream.clear();
	return ret;
}

ssize_t ZBase::write(const char *buffer , size_t size 
					 , const ZlibStream::StreamDataCallback &ondata) {
	if (buffer == nullptr || size == 0)
		return Z_OK;

	return _writeBuffer(buffer, size, getFlushMask() , ondata);
}

ssize_t ZBase::flush(const ZlibStream::StreamDataCallback &ondata) {
	return _flushBuffer(Z_FULL_FLUSH , ondata);
}

ssize_t ZBase::flush(int kind , const ZlibStream::StreamDataCallback &ondata) {
	return _flushBuffer(kind , ondata);
}

ssize_t ZBase::end(const char *buffer , size_t size 
					 , const ZlibStream::StreamDataCallback &ondata) {
	const char *tbuffer = buffer == nullptr ? "" : buffer;
	return _writeBuffer(tbuffer, size, getFinishFlushMask() , ondata);
}

void ZBase::reset() {
	ZlibStream &stream = getStream();
	auto &Stream = stream._stream;
	stream.type == ZlibStream::StreamType::Def ? deflateReset(&Stream)
				: inflateReset(&Stream);
}

Deflate::Deflate(const DeflateOptions &options) 
					: Deflate(
						options.level,
						options.method,
						options.windowBits,
						options.memLevel,
						options.strategy,
						options.memory,
						options.omemory,
						options.flushmask,
						options.finishflushmask
						) {

}

Deflate::Deflate(int level , int method , 
				int windowBits , int memLevel , 
				int strategy , int memory , int omemory,
				int flushmask , int finishflushmask) : 
				ZBase(flushmask,finishflushmask) {
	ZlibStream &stream = getStream();
	stream._stream = {0};
	deflateInit2(&stream._stream, level, method, windowBits, memLevel, strategy);
	stream.input.reserve(memory);
	stream.output.resize(omemory);
	stream.type = ZlibStream::Def;
}

Deflate::~Deflate() {
	ZlibStream &stream = getStream();
	auto &Stream = stream._stream;
	deflateEnd(&Stream);
}

ssize_t Deflate::compressBound(size_t s) {
	return ::compressBound(s);
}

ssize_t Deflate::compress(const char *src , size_t ssize , 
							char *dest , uLong &dsize , int level) {
	return ::compress2(reinterpret_cast<Bytef*>(dsize), &dsize, 
				reinterpret_cast<const Bytef*>(src), ssize, level);
}

ssize_t Deflate::compress(const char *src , size_t ssize , 
							ZlibStream::Buffer &buffer , int level) {
	uLong size = ::compressBound(ssize);
	buffer.resize(size);
	
	ssize_t ret = ::compress2(reinterpret_cast<Bytef*>(buffer.data()), &size, 
				reinterpret_cast<const Bytef*>(src), ssize, level);
	buffer.resize(size);
	return ret;
}

Inflate::Inflate(const InflateOptions &options) 
			: Inflate(options.windowBits ,
					  options.memory,
					  options.omemory,
					  options.flushmask,
					  options.finishflushmask) {

}

Inflate::Inflate(int windowBits , int memory , int omemory ,
				int flushmask, int finishflushmask) :
				ZBase(flushmask , finishflushmask) {
	ZlibStream &stream = getStream();
	stream._stream = {0};
	inflateInit2(&stream._stream, windowBits);
	stream.input.reserve(memory);
	stream.output.resize(omemory);
	stream.type = ZlibStream::Inf;				
}

Inflate::~Inflate() {
	ZlibStream &stream = getStream();
	auto &Stream = stream._stream;
	inflateEnd(&Stream);
}

ssize_t Inflate::uncompress(const char *src , size_t ssize , char *dest , uLong &dsize) {
	return ::uncompress(reinterpret_cast<Bytef*>(dest), &dsize, 
				reinterpret_cast<const Bytef*>(src), ssize);
}

ssize_t Inflate::uncompress(const char *src , size_t ssize , char *dest , uLong dsize) {
	return ::uncompress(reinterpret_cast<Bytef*>(dest), &dsize, 
				reinterpret_cast<const Bytef*>(src), ssize);
}
