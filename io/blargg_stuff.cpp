
#include "blargg_stuff.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
/* Copyright (C) 2005-2006 Shay Green. Permission is hereby granted, free of
charge, to any person obtaining a copy of this software module and associated
documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the
following conditions: The above copyright notice and this permission notice
shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */
#define FEX_ENABLE_RAR 1
#undef  require
#define require( expr ) assert( expr )

/* Like printf() except output goes to debugging console/file.

void dprintf( const char format [], ... ); */
static inline void blargg_dprintf_(const char[], ...) { }
#undef  dprintf
#define dprintf (1) ? (void) 0 : blargg_dprintf_

/* If expr is false, prints file and line number to debug console/log, then
continues execution normally. Meant for flagging potential problems or things
that should be looked into, but that aren't serious problems.

void check( bool expr ); */
#undef  check
#define check( expr ) ((void) 0)

/* If expr yields non-NULL error string, returns it from current function,
otherwise continues normally. */
#undef  RETURN_ERR
#define RETURN_ERR( expr ) \
	do {\
		blargg_err_t blargg_return_err_ = (expr);\
		if ( blargg_return_err_ )\
			return blargg_return_err_;\
	} while ( 0 )

/* If ptr is NULL, returns out-of-memory error, otherwise continues normally. */
#undef  CHECK_ALLOC
#define CHECK_ALLOC( ptr ) \
	do {\
		if ( !(ptr) )\
			return blargg_err_memory;\
	} while ( 0 )

template<typename T> T blargg_min(T x, T y) { return x < y ? x : y; }
template<typename T> T blargg_max(T x, T y) { return x > y ? x : y; }

#undef  min
#define min blargg_min

#undef  max
#define max blargg_max

// typedef unsigned char byte;
typedef unsigned char blargg_byte;
#undef  byte
#define byte blargg_byte

#ifndef BLARGG_EXPORT
#if defined (_WIN32) && BLARGG_BUILD_DLL
#define BLARGG_EXPORT __declspec(dllexport)
#elif defined (__GNUC__)
// can always set visibility, even when not building DLL
#define BLARGG_EXPORT __attribute__ ((visibility ("default")))
#else
#define BLARGG_EXPORT
#endif
#endif

#if BLARGG_LEGACY
#define BLARGG_CHECK_ALLOC CHECK_ALLOC
#define BLARGG_RETURN_ERR  RETURN_ERR
#endif

// Called after failed operation when overall operation may still complete OK.
// Only used by unit testing framework.
#undef ACK_FAILURE
#define ACK_FAILURE() ((void)0)

/* BLARGG_SOURCE_BEGIN: If defined, #included, allowing redefition of dprintf etc.
and check */
#ifdef BLARGG_SOURCE_BEGIN
#include BLARGG_SOURCE_BEGIN
#endif


// to do: remove?
#ifndef RAISE_ERROR
	#define RAISE_ERROR( str ) return str
#endif

typedef blargg_err_t error_t;

error_t Data_Writer::write( const void*, long ) { return 0; }

void Data_Writer::satisfy_lame_linker_() { }

// Std_File_Writer

Std_File_Writer::Std_File_Writer() : file_( 0 ) {
}

Std_File_Writer::~Std_File_Writer() {
	close();
}

error_t Std_File_Writer::open( const char* path )
{
	close();
	file_ = fopen( path, "wb" );
	if ( !file_ )
		RAISE_ERROR( "Couldn't open file for writing" );
		
	// to do: increase file buffer size
	//setvbuf( file_, 0, _IOFBF, 32 * 1024L );
	
	return 0;
}

error_t Std_File_Writer::write( const void* p, long s )
{
	long result = (long) fwrite( p, 1, s, file_ );
	if ( result != s )
		RAISE_ERROR( "Couldn't write to file" );
	return 0;
}

void Std_File_Writer::close()
{
	if ( file_ ) {
		fclose( file_ );
		file_ = 0;
	}
}

// Mem_Writer

Mem_Writer::Mem_Writer( void* p, long s, int b )
{
	data_ = (char*) p;
	size_ = 0;
	allocated = s;
	mode = b ? ignore_excess : fixed;
}

Mem_Writer::Mem_Writer()
{
	data_ = 0;
	size_ = 0;
	allocated = 0;
	mode = expanding;
}

Mem_Writer::~Mem_Writer()
{
	if ( mode == expanding )
		free( data_ );
}

error_t Mem_Writer::write( const void* p, long s )
{
	long remain = allocated - size_;
	if ( s > remain )
	{
		if ( mode == fixed )
			RAISE_ERROR( "Tried to write more data than expected" );
		
		if ( mode == ignore_excess )
		{
			s = remain;
		}
		else // expanding
		{
			long new_allocated = size_ + s;
			new_allocated += (new_allocated >> 1) + 2048;
			void* p = realloc( data_, new_allocated );
			if ( !p )
				RAISE_ERROR( "Out of memory" );
			data_ = (char*) p;
			allocated = new_allocated;
		}
	}
	
	assert( size_ + s <= allocated );
	memcpy( data_ + size_, p, s );
	size_ += s;
	
	return 0;
}

// Null_Writer

error_t Null_Writer::write( const void*, long )
{
	return 0;
}

// Auto_File_Reader

#ifndef STD_AUTO_FILE_WRITER
	#define STD_AUTO_FILE_WRITER Std_File_Writer
#endif

#ifdef HAVE_ZLIB_H
	#ifndef STD_AUTO_FILE_READER
		#define STD_AUTO_FILE_READER Gzip_File_Reader
	#endif

	#ifndef STD_AUTO_FILE_COMP_WRITER
		#define STD_AUTO_FILE_COMP_WRITER Gzip_File_Writer
	#endif

#else
	#ifndef STD_AUTO_FILE_READER
		#define STD_AUTO_FILE_READER Std_File_Reader
	#endif

	#ifndef STD_AUTO_FILE_COMP_WRITER
		#define STD_AUTO_FILE_COMP_WRITER Std_File_Writer
	#endif

#endif

const char* Auto_File_Reader::open()
{
	#ifdef DISABLE_AUTO_FILE
		return 0;
	#else
		if ( data )
			return 0;
		STD_AUTO_FILE_READER* d = new STD_AUTO_FILE_READER;
		data = d;
		return d->open( path );
	#endif
}

Auto_File_Reader::~Auto_File_Reader()
{
	if ( path )
		delete data;
}

// Auto_File_Writer

const char* Auto_File_Writer::open()
{
	#ifdef DISABLE_AUTO_FILE
		return 0;
	#else
		if ( data )
			return 0;
		STD_AUTO_FILE_WRITER* d = new STD_AUTO_FILE_WRITER;
		data = d;
		return d->open( path );
	#endif
}

const char* Auto_File_Writer::open_comp( int level )
{
	#ifdef DISABLE_AUTO_FILE
		return 0;
	#else
		if ( data )
			return 0;
		STD_AUTO_FILE_COMP_WRITER* d = new STD_AUTO_FILE_COMP_WRITER;
		data = d;
		return d->open( path, level );
	#endif
}

Auto_File_Writer::~Auto_File_Writer()
{
	#ifndef DISABLE_AUTO_FILE
		if ( path )
			delete data;
	#endif
}

blargg_err_t Std_File_Reader_u::open(const TCHAR path[])
{
#ifdef _UNICODE
	char * path8 = blargg_to_utf8(path);
	blargg_err_t err = Std_File_Reader::open(path8);
	free(path8);
	return err;
#else
	return Std_File_Reader::open(path);
#endif
}

error_t Std_File_Writer_u::open(const TCHAR* path)
{
	reset(_tfopen(path, _T("wb")));
	if (!file())
		RAISE_ERROR("Couldn't open file for writing");

	// to do: increase file buffer size
	//setvbuf( file_, NULL, _IOFBF, 32 * 1024L );

	return NULL;
}

void blargg_vector_::init()
{
	begin_ = NULL;
	size_ = 0;
}

void blargg_vector_::clear()
{
	void* p = begin_;
	begin_ = NULL;
	size_ = 0;
	free(p);
}

blargg_err_t blargg_vector_::resize_(size_t n, size_t elem_size)
{
	if (n != size_)
	{
		if (n == 0)
		{
			// Simpler to handle explicitly. Realloc will handle a size of 0,
			// but then we have to avoid raising an error for a NULL return.
			clear();
		}
		else
		{
			void* p = realloc(begin_, n * elem_size);
			CHECK_ALLOC(p);
			begin_ = p;
			size_ = n;
		}
	}
	return blargg_ok;
}

blargg_err_def_t blargg_err_generic = BLARGG_ERR_GENERIC;
blargg_err_def_t blargg_err_memory = BLARGG_ERR_MEMORY;
blargg_err_def_t blargg_err_caller = BLARGG_ERR_CALLER;
blargg_err_def_t blargg_err_internal = BLARGG_ERR_INTERNAL;
blargg_err_def_t blargg_err_limitation = BLARGG_ERR_LIMITATION;

blargg_err_def_t blargg_err_file_missing = BLARGG_ERR_FILE_MISSING;
blargg_err_def_t blargg_err_file_read = BLARGG_ERR_FILE_READ;
blargg_err_def_t blargg_err_file_write = BLARGG_ERR_FILE_WRITE;
blargg_err_def_t blargg_err_file_io = BLARGG_ERR_FILE_IO;
blargg_err_def_t blargg_err_file_full = BLARGG_ERR_FILE_FULL;
blargg_err_def_t blargg_err_file_eof = BLARGG_ERR_FILE_EOF;

blargg_err_def_t blargg_err_file_type = BLARGG_ERR_FILE_TYPE;
blargg_err_def_t blargg_err_file_feature = BLARGG_ERR_FILE_FEATURE;
blargg_err_def_t blargg_err_file_corrupt = BLARGG_ERR_FILE_CORRUPT;

const char* blargg_err_str(blargg_err_t err)
{
	if (!err)
		return "";

	if (*err == BLARGG_ERR_TYPE("")[0])
		return err + 1;

	return err;
}

bool blargg_is_err_type(blargg_err_t err, const char type[])
{
	if (err)
	{
		// True if first strlen(type) characters of err match type
		char const* p = err;
		while (*type && *type == *p)
		{
			type++;
			p++;
		}

		if (!*type)
			return true;
	}

	return false;
}

const char* blargg_err_details(blargg_err_t err)
{
	const char* p = err;
	if (!p)
	{
		p = "";
	}
	else if (*p == BLARGG_ERR_TYPE("")[0])
	{
		while (*p && *p != ';')
			p++;

		// Skip ; and space after it
		if (*p)
		{
			p++;

			check(*p == ' ');
			if (*p)
				p++;
		}
	}
	return p;
}

int blargg_err_to_code(blargg_err_t err, blargg_err_to_code_t const codes[])
{
	if (!err)
		return 0;

	while (codes->str && !blargg_is_err_type(err, codes->str))
		codes++;

	return codes->code;
}

blargg_err_t blargg_code_to_err(int code, blargg_err_to_code_t const codes[])
{
	if (!code)
		return blargg_ok;

	while (codes->str && codes->code != code)
		codes++;

	if (!codes->str)
		return blargg_err_generic;

	return codes->str;
}

blargg_err_t Data_Reader::read(void* p, long n)
{
	assert(n >= 0);

	if (n < 0)
		return blargg_err_caller;

	if (n <= 0)
		return blargg_ok;

	if (n > remain())
		return blargg_err_file_eof;

	blargg_err_t err = read_v(p, n);
	if (!err)
		remain_ -= n;

	return err;
}

blargg_err_t Data_Reader::read_avail(void* p, int* n_)
{
	assert(*n_ >= 0);

	long n = (long)min((BOOST::uint64_t)(*n_), remain());
	*n_ = 0;

	if (n < 0)
		return blargg_err_caller;

	if (n <= 0)
		return blargg_ok;

	blargg_err_t err = read_v(p, n);
	if (!err)
	{
		remain_ -= n;
		*n_ = (int)n;
	}

	return err;
}

blargg_err_t Data_Reader::read_avail(void* p, long* n)
{
	int i = STATIC_CAST(int, *n);
	blargg_err_t err = read_avail(p, &i);
	*n = i;
	return err;
}

blargg_err_t Data_Reader::skip_v(BOOST::uint64_t count)
{
	char buf[512];
	while (count)
	{
		BOOST::uint64_t n = min(count, (BOOST::uint64_t) sizeof buf);
		count -= n;
		RETURN_ERR(read_v(buf, (long)n));
	}
	return blargg_ok;
}

blargg_err_t Data_Reader::skip(long n)
{
	assert(n >= 0);

	if (n < 0)
		return blargg_err_caller;

	if (n <= 0)
		return blargg_ok;

	if (n > remain())
		return blargg_err_file_eof;

	blargg_err_t err = skip_v(n);
	if (!err)
		remain_ -= n;

	return err;
}


// File_Reader

blargg_err_t File_Reader::seek(BOOST::uint64_t n)
{
	assert(n >= 0);

	if (n == tell())
		return blargg_ok;

	if (n > size())
		return blargg_err_file_eof;

	blargg_err_t err = seek_v(n);
	if (!err)
		set_tell(n);

	return err;
}

blargg_err_t File_Reader::skip_v(BOOST::uint64_t n)
{
	return seek_v(tell() + n);
}


// Subset_Reader

Subset_Reader::Subset_Reader(Data_Reader* dr, BOOST::uint64_t size) :
	in(dr)
{
	set_remain(min(size, dr->remain()));
}

blargg_err_t Subset_Reader::read_v(void* p, long s)
{
	return in->read(p, s);
}


// Remaining_Reader

Remaining_Reader::Remaining_Reader(void const* h, int size, Data_Reader* r) :
	in(r)
{
	header = h;
	header_remain = size;

	set_remain(size + r->remain());
}

blargg_err_t Remaining_Reader::read_v(void* out, long count)
{
	long first = min(count, header_remain);
	if (first)
	{
		memcpy(out, header, first);
		header = STATIC_CAST(char const*, header) + first;
		header_remain -= first;
	}

	return in->read(STATIC_CAST(char*, out) + first, count - first);
}


// Mem_File_Reader

Mem_File_Reader::Mem_File_Reader(const void* p, long s) :
	begin(STATIC_CAST(const char*, p))
{
	set_size(s);
}

blargg_err_t Mem_File_Reader::read_v(void* p, long s)
{
	memcpy(p, begin + tell(), s);
	return blargg_ok;
}

blargg_err_t Mem_File_Reader::seek_v(BOOST::uint64_t)
{
	return blargg_ok;
}


// Callback_Reader

Callback_Reader::Callback_Reader(callback_t c, BOOST::uint64_t s, void* d) :
	callback(c),
	user_data(d)
{
	set_remain(s);
}

blargg_err_t Callback_Reader::read_v(void* out, long count)
{
	return callback(user_data, out, count);
}


// Callback_File_Reader

Callback_File_Reader::Callback_File_Reader(callback_t c, BOOST::uint64_t s, void* d) :
	callback(c),
	user_data(d)
{
	set_size(s);
}

blargg_err_t Callback_File_Reader::read_v(void* out, long count)
{
	return callback(user_data, out, count, tell());
}

blargg_err_t Callback_File_Reader::seek_v(BOOST::uint64_t)
{
	return blargg_ok;
}

static const BOOST::uint8_t mask_tab[6] = { 0x80,0xE0,0xF0,0xF8,0xFC,0xFE };

static const BOOST::uint8_t val_tab[6] = { 0,0xC0,0xE0,0xF0,0xF8,0xFC };

size_t utf8_char_len_from_header(char p_c)
{
	size_t cnt = 0;
	for (;;)
	{
		if ((p_c & mask_tab[cnt]) == val_tab[cnt]) break;
		if (++cnt >= 6) return 0;
	}

	return cnt + 1;
}

size_t utf8_decode_char(const char* p_utf8, unsigned& wide, size_t mmax)
{
	const BOOST::uint8_t* utf8 = (const BOOST::uint8_t*)p_utf8;

	if (mmax == 0)
	{
		wide = 0;
		return 0;
	}

	if (utf8[0] < 0x80)
	{
		wide = utf8[0];
		return utf8[0] > 0 ? 1 : 0;
	}
	if (mmax > 6) mmax = 6;
	wide = 0;

	unsigned res = 0;
	unsigned n;
	unsigned cnt = 0;
	for (;;)
	{
		if ((*utf8 & mask_tab[cnt]) == val_tab[cnt]) break;
		if (++cnt >= mmax) return 0;
	}
	cnt++;

	if (cnt == 2 && !(*utf8 & 0x1E)) return 0;

	if (cnt == 1)
		res = *utf8;
	else
		res = (0xFF >> (cnt + 1)) & *utf8;

	for (n = 1; n < cnt; n++)
	{
		if ((utf8[n] & 0xC0) != 0x80)
			return 0;
		if (!res && n == 2 && !((utf8[n] & 0x7F) >> (7 - cnt)))
			return 0;

		res = (res << 6) | (utf8[n] & 0x3F);
	}

	wide = res;

	return cnt;
}

size_t utf8_encode_char(unsigned wide, char* target)
{
	size_t count;

	if (wide < 0x80)
		count = 1;
	else if (wide < 0x800)
		count = 2;
	else if (wide < 0x10000)
		count = 3;
	else if (wide < 0x200000)
		count = 4;
	else if (wide < 0x4000000)
		count = 5;
	else if (wide <= 0x7FFFFFFF)
		count = 6;
	else
		return 0;

	if (target == 0)
		return count;

	switch (count)
	{
	case 6:
		target[5] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x4000000;
	case 5:
		target[4] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x200000;
	case 4:
		target[3] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x10000;
	case 3:
		target[2] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0x800;
	case 2:
		target[1] = 0x80 | (wide & 0x3F);
		wide = wide >> 6;
		wide |= 0xC0;
	case 1:
		target[0] = wide;
	}

	return count;
}

size_t utf16_encode_char(unsigned cur_wchar, blargg_wchar_t* out)
{
	if (cur_wchar < 0x10000)
	{
		if (out)* out = (blargg_wchar_t)cur_wchar; return 1;
	}
	else if (cur_wchar < (1 << 20))
	{
		unsigned c = cur_wchar - 0x10000;
		//MSDN:
		//The first (high) surrogate is a 16-bit code value in the range U+D800 to U+DBFF. The second (low) surrogate is a 16-bit code value in the range U+DC00 to U+DFFF. Using surrogates, Unicode can support over one million characters. For more details about surrogates, refer to The Unicode Standard, version 2.0.
		if (out)
		{
			out[0] = (blargg_wchar_t)(0xD800 | (0x3FF & (c >> 10)));
			out[1] = (blargg_wchar_t)(0xDC00 | (0x3FF & c));
		}
		return 2;
	}
	else
	{
		if (out)* out = '?'; return 1;
	}
}

size_t utf16_decode_char(const blargg_wchar_t* p_source, unsigned* p_out, size_t p_source_length)
{
	if (p_source_length == 0) return 0;
	else if (p_source_length == 1)
	{
		*p_out = p_source[0];
		return 1;
	}
	else
	{
		size_t retval = 0;
		unsigned decoded = p_source[0];
		if (decoded != 0)
		{
			retval = 1;
			if ((decoded & 0xFC00) == 0xD800)
			{
				unsigned low = p_source[1];
				if ((low & 0xFC00) == 0xDC00)
				{
					decoded = 0x10000 + (((decoded & 0x3FF) << 10) | (low & 0x3FF));
					retval = 2;
				}
			}
		}
		*p_out = decoded;
		return retval;
	}
}

// Converts wide-character path to UTF-8. Free result with free(). Only supported on Windows.
char* blargg_to_utf8(const blargg_wchar_t* wpath)
{
	if (wpath == NULL)
		return NULL;

	size_t needed = 0;
	size_t mmax = blargg_wcslen(wpath);
	if (mmax <= 0)
		return NULL;

	size_t ptr = 0;
	while (ptr < mmax)
	{
		unsigned wide = 0;
		size_t char_len = utf16_decode_char(wpath + ptr, &wide, mmax - ptr);
		if (!char_len) break;
		ptr += char_len;
		needed += utf8_encode_char(wide, 0);
	}
	if (needed <= 0)
		return NULL;

	char* path = (char*)calloc(needed + 1, 1);
	if (path == NULL)
		return NULL;

	ptr = 0;
	size_t actual = 0;
	while (ptr < mmax && actual < needed)
	{
		unsigned wide = 0;
		size_t char_len = utf16_decode_char(wpath + ptr, &wide, mmax - ptr);
		if (!char_len) break;
		ptr += char_len;
		actual += utf8_encode_char(wide, path + actual);
	}

	if (actual == 0)
	{
		free(path);
		return NULL;
	}

	assert(actual == needed);
	return path;
}

// Converts UTF-8 path to wide-character. Free result with free() Only supported on Windows.
blargg_wchar_t* blargg_to_wide(const char* path)
{
	if (path == NULL)
		return NULL;

	size_t mmax = strlen(path);
	if (mmax <= 0)
		return NULL;

	size_t needed = 0;
	size_t ptr = 0;
	while (ptr < mmax)
	{
		unsigned wide = 0;
		size_t char_len = utf8_decode_char(path + ptr, wide, mmax - ptr);
		if (!char_len) break;
		ptr += char_len;
		needed += utf16_encode_char(wide, 0);
	}
	if (needed <= 0)
		return NULL;

	blargg_wchar_t* wpath = (blargg_wchar_t*)calloc(needed + 1, sizeof * wpath);
	if (wpath == NULL)
		return NULL;

	ptr = 0;
	size_t actual = 0;
	while (ptr < mmax && actual < needed)
	{
		unsigned wide = 0;
		size_t char_len = utf8_decode_char(path + ptr, wide, mmax - ptr);
		if (!char_len) break;
		ptr += char_len;
		actual += utf16_encode_char(wide, wpath + actual);
	}
	if (actual == 0)
	{
		free(wpath);
		return NULL;
	}

	assert(actual == needed);
	return wpath;
}

#ifdef _WIN32

static FILE* blargg_fopen(const char path[], const char mode[])
{
	FILE* file = NULL;
	blargg_wchar_t* wmode = NULL;
	blargg_wchar_t* wpath = NULL;

	wpath = blargg_to_wide(path);
	if (wpath)
	{
		wmode = blargg_to_wide(mode);
		if (wmode)
#if _MSC_VER >= 1300
			errno = _wfopen_s(&file, wpath, wmode);
#else
			file = _wfopen(wpath, wmode);
#endif
	}

	// Save and restore errno in case free() clears it
	int saved_errno = errno;
	free(wmode);
	free(wpath);
	errno = saved_errno;

	return file;
}

#else

static inline FILE* blargg_fopen(const char path[], const char mode[])
{
	return fopen(path, mode);
}

#endif


// Std_File_Reader

Std_File_Reader::Std_File_Reader()
{
	file_ = NULL;
}

Std_File_Reader::~Std_File_Reader()
{
	close();
}

static blargg_err_t blargg_fopen(FILE** out, const char path[])
{
	errno = 0;
	*out = blargg_fopen(path, "rb");
	if (!*out)
	{
#ifdef ENOENT
		if (errno == ENOENT)
			return blargg_err_file_missing;
#endif
#ifdef ENOMEM
		if (errno == ENOMEM)
			return blargg_err_memory;
#endif
		return blargg_err_file_read;
	}

	return blargg_ok;
}

static blargg_err_t blargg_fsize(FILE* f, long* out)
{
	if (fseek(f, 0, SEEK_END))
		return blargg_err_file_io;

	*out = ftell(f);
	if (*out < 0)
		return blargg_err_file_io;

	if (fseek(f, 0, SEEK_SET))
		return blargg_err_file_io;

	return blargg_ok;
}

blargg_err_t Std_File_Reader::open(const char path[])
{
	close();

	FILE* f;
	RETURN_ERR(blargg_fopen(&f, path));

	long s;
	blargg_err_t err = blargg_fsize(f, &s);
	if (err)
	{
		fclose(f);
		return err;
	}

	file_ = f;
	set_size(s);

	return blargg_ok;
}

void Std_File_Reader::make_unbuffered()
{
#ifdef _WIN32
	BOOST::uint64_t offset = _ftelli64(STATIC_CAST(FILE*, file_));
#else
	BOOST::uint64_t offset = ftello(STATIC_CAST(FILE*, file_));
#endif
	if (setvbuf(STATIC_CAST(FILE*, file_), NULL, _IONBF, 0))
		check(false); // shouldn't fail, but OK if it does
#ifdef _WIN32
	_fseeki64(STATIC_CAST(FILE*, file_), offset, SEEK_SET);
#else
	fseeko(STATIC_CAST(FILE*, file_), offset, SEEK_SET);
#endif
}

blargg_err_t Std_File_Reader::read_v(void* p, long s)
{
	if ((size_t)s != fread(p, 1, s, STATIC_CAST(FILE*, file_)))
	{
		// Data_Reader's wrapper should prevent EOF
		check(!feof(STATIC_CAST(FILE*, file_)));

		return blargg_err_file_io;
	}

	return blargg_ok;
}

blargg_err_t Std_File_Reader::seek_v(BOOST::uint64_t n)
{
#ifdef _WIN32
	if (_fseeki64(STATIC_CAST(FILE*, file_), n, SEEK_SET))
#else
	if (fseeko(STATIC_CAST(FILE*, file_), n, SEEK_SET))
#endif
	{
		// Data_Reader's wrapper should prevent EOF
		check(!feof(STATIC_CAST(FILE*, file_)));

		return blargg_err_file_io;
	}

	return blargg_ok;
}

void Std_File_Reader::close()
{
	if (file_)
	{
		fclose(STATIC_CAST(FILE*, file_));
		file_ = NULL;
	}
}





// Gzip_File_Reader

#ifdef HAVE_ZLIB_H

#include "zlib.h"

static const char* get_gzip_eof(const char path[], long* eof)
{
	FILE* file;
	RETURN_ERR(blargg_fopen(&file, path));

	int const h_size = 4;
	unsigned char h[h_size];

	// read four bytes to ensure that we can seek to -4 later
	if (fread(h, 1, h_size, file) != (size_t)h_size || h[0] != 0x1F || h[1] != 0x8B)
	{
		// Not gzipped
		if (ferror(file))
			return blargg_err_file_io;

		if (fseek(file, 0, SEEK_END))
			return blargg_err_file_io;

		*eof = ftell(file);
		if (*eof < 0)
			return blargg_err_file_io;
	}
	else
	{
		// Gzipped; get uncompressed size from end
		if (fseek(file, -h_size, SEEK_END))
			return blargg_err_file_io;

		if (fread(h, 1, h_size, file) != (size_t)h_size)
			return blargg_err_file_io;

		*eof = get_le32(h);
	}

	if (fclose(file))
		check(false);

	return blargg_ok;
}

Gzip_File_Reader::Gzip_File_Reader()
{
	file_ = NULL;
}

Gzip_File_Reader::~Gzip_File_Reader()
{
	close();
}

blargg_err_t Gzip_File_Reader::open(const char path[])
{
	close();

	long s;
	RETURN_ERR(get_gzip_eof(path, &s));

	file_ = gzopen(path, "rb");
	if (!file_)
		return blargg_err_file_read;

	set_size(s);
	return blargg_ok;
}

static blargg_err_t convert_gz_error(gzFile file)
{
	int err;
	gzerror(file, &err);

	switch (err)
	{
	case Z_STREAM_ERROR:    break;
	case Z_DATA_ERROR:      return blargg_err_file_corrupt;
	case Z_MEM_ERROR:       return blargg_err_memory;
	case Z_BUF_ERROR:       break;
	}
	return blargg_err_internal;
}

blargg_err_t Gzip_File_Reader::read_v(void* p, long s)
{
	while (s > 0)
	{
		int s_i = (int)(s > INT_MAX ? INT_MAX : s);
		int result = gzread((gzFile)file_, p, s_i);
		if (result != s_i)
		{
			if (result < 0)
				return convert_gz_error((gzFile)file_);

			return blargg_err_file_corrupt;
		}
		p = (char*)p + result;
		s -= result;
	}

	return blargg_ok;
}

blargg_err_t Gzip_File_Reader::seek_v(BOOST::uint64_t n)
{
	if (gzseek((gzFile)file_, (long)n, SEEK_SET) < 0)
		return convert_gz_error((gzFile)file_);

	return blargg_ok;
}

void Gzip_File_Reader::close()
{
	if (file_)
	{
		if (gzclose((gzFile)file_))
			check(false);
		file_ = NULL;
	}
}

#endif
