// Sets up common environment for Shay Green's libraries.
// To change configuration options, modify blargg_config.h, not this file.

// File_Extractor 1.0.0
#ifndef BLARGG_COMMON_H
#define BLARGG_COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include <tchar.h>

typedef const char* blargg_err_t; // 0 on success, otherwise error string

#ifdef _WIN32
typedef wchar_t blargg_wchar_t;
#else
typedef uint16_t blargg_wchar_t;
#endif

inline size_t blargg_wcslen( const blargg_wchar_t* str )
{
	size_t length = 0;
	while ( *str++ ) length++;
	return length;
}

// Success; no error
blargg_err_t const blargg_ok = 0;

// BLARGG_RESTRICT: equivalent to C99's restrict, where supported
#if __GNUC__ >= 3 || _MSC_VER >= 1100
	#define BLARGG_RESTRICT __restrict
#else
	#define BLARGG_RESTRICT
#endif

#if __cplusplus >= 199711
	#define BLARGG_MUTABLE mutable
#else
	#define BLARGG_MUTABLE
#endif

/* BLARGG_4CHAR('a','b','c','d') = 'abcd' (four character integer constant).
I don't just use 'abcd' because that's implementation-dependent. */
#define BLARGG_4CHAR( a, b, c, d ) \
	((a&0xFF)*0x1000000 + (b&0xFF)*0x10000 + (c&0xFF)*0x100 + (d&0xFF))

/* BLARGG_STATIC_ASSERT( expr ): Generates compile error if expr is 0.
Can be used at file, function, or class scope. */
#ifdef _MSC_VER
	// MSVC6 (_MSC_VER < 1300) __LINE__ fails when /Zl is specified
	#define BLARGG_STATIC_ASSERT( expr )    \
		void blargg_failed_( int (*arg) [2 / (int) !!(expr) - 1] )
#else
	// Others fail when declaring same function multiple times in class,
	// so differentiate them by line
	#define BLARGG_STATIC_ASSERT( expr )    \
		void blargg_failed_( int (*arg) [2 / !!(expr) - 1] [__LINE__] )
#endif

/* Pure virtual functions cause a vtable entry to a "called pure virtual"
error handler, requiring linkage to the C++ runtime library. This macro is
used in place of the "= 0", and simply expands to its argument. During
development, it expands to "= 0", allowing detection of missing overrides. */
#define BLARGG_PURE( def ) def

/* My code depends on ASCII anywhere a character or string constant is
compared with data read from a file, and anywhere file data is read and
treated as a string. */
#if '\n'!=0x0A || ' '!=0x20 || '0'!=0x30 || 'A'!=0x41 || 'a'!=0x61
	#error "ASCII character set required"
#endif

/* My code depends on int being at least 32 bits. Almost everything these days
uses at least 32-bit ints, so it's hard to even find a system with 16-bit ints
to test with. The issue can't be gotten around by using a suitable blargg_int
everywhere either, because int is often converted to implicitly when doing
arithmetic on smaller types. */
#if UINT_MAX < 0xFFFFFFFF
	#error "int must be at least 32 bits"
#endif

// In case compiler doesn't support these properly. Used rarely.
#define STATIC_CAST(T,expr) static_cast<T> (expr)
#define CONST_CAST( T,expr) const_cast<T> (expr)


/* BLARGG_DEPRECATED [_TEXT] for any declarations/text to be removed in a
future version. In GCC, we can let the compiler warn. In other compilers,
we strip it out unless BLARGG_LEGACY is true. */
#if BLARGG_LEGACY
	// Allow old client code to work without warnings
	#define BLARGG_DEPRECATED_TEXT( text ) text
	#define BLARGG_DEPRECATED(      text ) text
#elif __GNUC__ >= 4
	// In GCC, we can mark declarations and let the compiler warn
	#define BLARGG_DEPRECATED_TEXT( text ) text
	#define BLARGG_DEPRECATED(      text ) __attribute__ ((deprecated)) text
#else
	// By default, deprecated items are removed, to avoid use in new code
	#define BLARGG_DEPRECATED_TEXT( text )
	#define BLARGG_DEPRECATED(      text )
#endif

/* BOOST::int8_t, BOOST::int32_t, etc.
I used BOOST since I originally was going to allow use of the boost library
for prividing the definitions. If I'm defining them, they must be scoped or
else they could conflict with the standard ones at global scope. Even if
HAVE_STDINT_H isn't defined, I can't assume the typedefs won't exist at
global scope already. */
#if defined (HAVE_STDINT_H) || \
		UCHAR_MAX != 0xFF || USHRT_MAX != 0xFFFF || UINT_MAX != 0xFFFFFFFF
	#include <stdint.h>
	#define BOOST
#else
	struct BOOST
	{
		typedef signed char        int8_t;
		typedef unsigned char     uint8_t;
		typedef short             int16_t;
		typedef unsigned short   uint16_t;
		typedef int               int32_t;
		typedef unsigned int     uint32_t;
		typedef __int64           int64_t;
		typedef unsigned __int64 uint64_t;
	};
#endif

/* My code is not written with exceptions in mind, so either uses new (nothrow)
OR overrides operator new in my classes. The former is best since clients
creating objects will get standard exceptions on failure, but that causes it
to require the standard C++ library. So, when the client is using the C
interface, I override operator new to use malloc. */

// BLARGG_DISABLE_NOTHROW is put inside classes
#ifndef BLARGG_DISABLE_NOTHROW
	// throw spec mandatory in ISO C++ if NULL can be returned
	#if __cplusplus >= 199711 || __GNUC__ >= 3 || _MSC_VER >= 1300
		#define BLARGG_THROWS_NOTHING throw ()
	#else
		#define BLARGG_THROWS_NOTHING
	#endif

	#define BLARGG_DISABLE_NOTHROW \
		void* operator new ( size_t s ) BLARGG_THROWS_NOTHING { return malloc( s ); }\
		void operator delete( void* p ) BLARGG_THROWS_NOTHING { free( p ); }

	#define BLARGG_NEW new
#else
	// BLARGG_NEW is used in place of new in library code
	#include <new>
	#define BLARGG_NEW new (std::nothrow)
#endif

	class blargg_vector_ {
	protected:
		void* begin_;
		size_t size_;
		void init();
		blargg_err_t resize_( size_t n, size_t elem_size );
	public:
		size_t size() const { return size_; }
		void clear();
	};

// Very lightweight vector for POD types (no constructor/destructor)
template<class T>
class blargg_vector : public blargg_vector_ {
	union T_must_be_pod { T t; }; // fails if T is not POD
public:
	blargg_vector()         { init(); }
	~blargg_vector()        { clear(); }
	
	blargg_err_t resize( size_t n ) { return resize_( n, sizeof (T) ); }
	
		  T* begin()       { return static_cast<T*> (begin_); }
	const T* begin() const { return static_cast<T*> (begin_); }
	
		  T* end()         { return static_cast<T*> (begin_) + size_; }
	const T* end()   const { return static_cast<T*> (begin_) + size_; }
	
	T& operator [] ( size_t n )
	{
		assert( n < size_ );
		return static_cast<T*> (begin_) [n];
	}
	
	const T& operator [] ( size_t n ) const
	{
		assert( n < size_ );
		return static_cast<T*> (begin_) [n];
	}
};

// Callback function with user data.
// blargg_callback<T> set_callback; // for user, this acts like...
// void set_callback( T func, void* user_data = NULL ); // ...this
// To call function, do set_callback.f( .. set_callback.data ... );
template<class T>
struct blargg_callback
{
	T f;
	void* data;
	blargg_callback() { f = NULL; }
	void operator () ( T callback, void* user_data = NULL ) { f = callback; data = user_data; }
};

BLARGG_DEPRECATED( typedef signed   int blargg_long; )
BLARGG_DEPRECATED( typedef unsigned int blargg_ulong; )
#if BLARGG_LEGACY
	#define BOOST_STATIC_ASSERT BLARGG_STATIC_ASSERT
#endif

class Data_Reader {
public:

	// Reads min(*n,remain()) bytes and sets *n to this number, thus trying to read more
	// tham remain() bytes doesn't result in error, just *n being set to remain().
	blargg_err_t read_avail(void* p, int* n);
	blargg_err_t read_avail(void* p, long* n);

	// Reads exactly n bytes, or returns error if they couldn't ALL be read.
	// Reading past end of file results in blargg_err_file_eof.
	blargg_err_t read(void* p, long n);

	// Number of bytes remaining until end of file
	BOOST::uint64_t remain() const { return remain_; }

	// Reads and discards n bytes. Skipping past end of file results in blargg_err_file_eof.
	blargg_err_t skip(long n);

	virtual ~Data_Reader() { }

private:
	// noncopyable
	Data_Reader(const Data_Reader&);
	Data_Reader& operator = (const Data_Reader&);

	// Derived interface
protected:
	Data_Reader() : remain_(0) { }

	// Sets remain
	void set_remain(BOOST::uint64_t n) { assert(n >= 0); remain_ = n; }

	// Do same as read(). Guaranteed that 0 < n <= remain(). Value of remain() is updated
	// AFTER this call succeeds, not before. set_remain() should NOT be called from this.
	virtual blargg_err_t read_v(void*, long n)     BLARGG_PURE({ (void)n; return blargg_ok; })

		// Do same as skip(). Guaranteed that 0 < n <= remain(). Default just reads data
		// and discards it. Value of remain() is updated AFTER this call succeeds, not
		// before. set_remain() should NOT be called from this.
		virtual blargg_err_t skip_v(BOOST::uint64_t n);

	// Implementation
public:
	BLARGG_DISABLE_NOTHROW

private:
	BOOST::uint64_t remain_;
};


// Supports seeking in addition to Data_Reader operations
class File_Reader : public Data_Reader {
public:

	// Size of file
	BOOST::uint64_t size() const { return size_; }

	// Current position in file
	BOOST::uint64_t tell() const { return size_ - remain(); }

	// Goes to new position
	blargg_err_t seek(BOOST::uint64_t);

	// Derived interface
protected:
	// Sets size and resets position
	void set_size(BOOST::uint64_t n) { size_ = n; Data_Reader::set_remain(n); }
	void set_size(int n) { set_size(STATIC_CAST(BOOST::uint64_t, n)); }
	void set_size(long n) { set_size(STATIC_CAST(BOOST::uint64_t, n)); }

	// Sets reported position
	void set_tell(BOOST::uint64_t i) { assert(0 <= i && i <= size_); Data_Reader::set_remain(size_ - i); }

	// Do same as seek(). Guaranteed that 0 <= n <= size().  Value of tell() is updated
	// AFTER this call succeeds, not before. set_* functions should NOT be called from this.
	virtual blargg_err_t seek_v(BOOST::uint64_t n) BLARGG_PURE({ (void)n; return blargg_ok; })

		// Implementation
protected:
	File_Reader() : size_(0) { }

	virtual blargg_err_t skip_v(BOOST::uint64_t);

private:
	BOOST::uint64_t size_;

	void set_remain(); // avoid accidental use of set_remain
};


// Reads from file on disk
class Std_File_Reader : public File_Reader {
public:

	// Opens file
	blargg_err_t open(const char path[]);

	// Closes file if one was open
	void close();

	// Switches to unbuffered mode. Useful if buffering is already being
	// done at a higher level.
	void make_unbuffered();

	// Implementation
public:
	Std_File_Reader();
	virtual ~Std_File_Reader();

protected:
	virtual blargg_err_t read_v(void*, long);
	virtual blargg_err_t seek_v(BOOST::uint64_t);

private:
	void* file_;
};


// Treats range of memory as a file
class Mem_File_Reader : public File_Reader {
public:

	Mem_File_Reader(const void* begin, long size);

	// Implementation
protected:
	virtual blargg_err_t read_v(void*, long);
	virtual blargg_err_t seek_v(BOOST::uint64_t);

private:
	const char* const begin;
};


// Allows only count bytes to be read from reader passed
class Subset_Reader : public Data_Reader {
public:

	Subset_Reader(Data_Reader*, BOOST::uint64_t count);

	// Implementation
protected:
	virtual blargg_err_t read_v(void*, long);

private:
	Data_Reader* const in;
};


// Joins already-read header and remaining data into original file.
// Meant for cases where you've already read header and don't want
// to seek and re-read data (for efficiency).
class Remaining_Reader : public Data_Reader {
public:

	Remaining_Reader(void const* header, int header_size, Data_Reader*);

	// Implementation
protected:
	virtual blargg_err_t read_v(void*, long);

private:
	Data_Reader* const in;
	void const* header;
	long header_remain;
};


// Invokes callback function to read data
extern "C" { // necessary to be usable from C
	typedef const char* (*callback_reader_func_t)(
		void* user_data,    // Same value passed to constructor
		void* out,          // Buffer to place data into
		long count           // Number of bytes to read
		);
}
class Callback_Reader : public Data_Reader {
public:
	typedef callback_reader_func_t callback_t;
	Callback_Reader(callback_t, BOOST::uint64_t size, void* user_data);

	// Implementation
protected:
	virtual blargg_err_t read_v(void*, long);

private:
	callback_t const callback;
	void* const user_data;
};


// Invokes callback function to read data
extern "C" { // necessary to be usable from C
	typedef const char* (*callback_file_reader_func_t)(
		void* user_data,    // Same value passed to constructor
		void* out,          // Buffer to place data into
		long count,          // Number of bytes to read
		BOOST::uint64_t pos             // Position in file to read from
		);
}
class Callback_File_Reader : public File_Reader {
public:
	typedef callback_file_reader_func_t callback_t;
	Callback_File_Reader(callback_t, BOOST::uint64_t size, void* user_data);

	// Implementation
protected:
	virtual blargg_err_t read_v(void*, long);
	virtual blargg_err_t seek_v(BOOST::uint64_t);

private:
	callback_t const callback;
	void* const user_data;
};

#ifdef _WIN32
typedef wchar_t blargg_wchar_t;
#elif defined(HAVE_STDINT_H)
#include <stdint.h>
typedef uint16_t blargg_wchar_t;
#else
typedef unsigned short blargg_wchar_t;
#endif

char* blargg_to_utf8(const blargg_wchar_t*);
blargg_wchar_t* blargg_to_wide(const char*);

class Data_Writer {
public:
	Data_Writer() { }
	virtual ~Data_Writer() { }

	typedef blargg_err_t error_t;

	// Write 'n' bytes. NULL on success, otherwise error string.
	virtual error_t write(const void*, long n) = 0;

	void satisfy_lame_linker_();
private:
	// noncopyable
	Data_Writer(const Data_Writer&);
	Data_Writer& operator = (const Data_Writer&);
};

class Std_File_Writer : public Data_Writer {
public:
	Std_File_Writer();
	~Std_File_Writer();

	error_t open(const char*);

	FILE* file() const { return file_; }

	// Forward writes to file. Caller must close file later.
	//void forward( FILE* );

	error_t write(const void*, long);

	void close();

protected:
	void reset(FILE* f) { file_ = f; }
private:
	FILE* file_;
	error_t open(const char* path, int ignored) { return open(path); }
	friend class Auto_File_Writer;
};

// Write data to memory
class Mem_Writer : public Data_Writer {
	char* data_;
	long size_;
	long allocated;
	enum { expanding, fixed, ignore_excess } mode;
public:
	// Keep all written data in expanding block of memory
	Mem_Writer();

	// Write to fixed-size block of memory. If ignore_excess is false, returns
	// error if more than 'size' data is written, otherwise ignores any excess.
	Mem_Writer(void*, long size, int ignore_excess = 0);

	error_t write(const void*, long);

	// Pointer to beginning of written data
	char* data() { return data_; }

	// Number of bytes written
	long size() const { return size_; }

	~Mem_Writer();
};

// Written data is ignored
class Null_Writer : public Data_Writer {
public:
	error_t write(const void*, long);
};

// Auto_File to use in place of Data_Reader&/Data_Writer&, allowing a normal
// file path to be used in addition to a Data_Reader/Data_Writer.

class Auto_File_Reader {
public:
	Auto_File_Reader() : data(0), path(0) { }
	Auto_File_Reader(Data_Reader& r) : data(&r), path(0) { }
#ifndef DISABLE_AUTO_FILE
	Auto_File_Reader(const char* path_) : data(0), path(path_) { }
#endif
	Auto_File_Reader(Auto_File_Reader const&);
	Auto_File_Reader& operator = (Auto_File_Reader const&);
	~Auto_File_Reader();
	const char* open();

	int operator ! () const { return !data; }
	Data_Reader* operator -> () const { return  data; }
	Data_Reader& operator *  () const { return *data; }
private:
	/* mutable */ Data_Reader* data;
	const char* path;
};

class Auto_File_Writer {
public:
	Auto_File_Writer() : data(0), path(0) { }
	Auto_File_Writer(Data_Writer& r) : data(&r), path(0) { }
#ifndef DISABLE_AUTO_FILE
	Auto_File_Writer(const char* path_) : data(0), path(path_) { }
#endif
	Auto_File_Writer(Auto_File_Writer const&);
	Auto_File_Writer& operator = (Auto_File_Writer const&);
	~Auto_File_Writer();
	const char* open();
	const char* open_comp(int level = -1); // compress output if possible

	int operator ! () const { return !data; }
	Data_Writer* operator -> () const { return  data; }
	Data_Writer& operator *  () const { return *data; }
private:
	/* mutable */ Data_Writer* data;
	const char* path;
};

inline Auto_File_Reader& Auto_File_Reader::operator = (Auto_File_Reader const& r)
{
	data = r.data;
	path = r.path;
	((Auto_File_Reader*)& r)->data = 0;
	return *this;
}
inline Auto_File_Reader::Auto_File_Reader(Auto_File_Reader const& r) { *this = r; }

inline Auto_File_Writer& Auto_File_Writer::operator = (Auto_File_Writer const& r)
{
	data = r.data;
	path = r.path;
	((Auto_File_Writer*)& r)->data = 0;
	return *this;
}
inline Auto_File_Writer::Auto_File_Writer(Auto_File_Writer const& r) { *this = r; }


class Std_File_Reader_u : public Std_File_Reader
{
public:
	blargg_err_t open(const TCHAR* path);
};

class Std_File_Writer_u : public Std_File_Writer
{
public:
	blargg_err_t open(const TCHAR* path);
};

// BLARGG_CPU_CISC: Defined if CPU has very few general-purpose registers (< 16)
#if defined (__i386__) || defined (__x86_64__) || defined (_M_IX86) || defined (_M_X64)
#define BLARGG_CPU_X86 1
#define BLARGG_CPU_CISC 1
#endif

#if defined (__powerpc__) || defined (__ppc__) || defined (__ppc64__) || \
		defined (__POWERPC__) || defined (__powerc)
#define BLARGG_CPU_POWERPC 1
#define BLARGG_CPU_RISC 1
#endif

// BLARGG_BIG_ENDIAN, BLARGG_LITTLE_ENDIAN: Determined automatically, otherwise only
// one may be #defined to 1. Only needed if something actually depends on byte order.
#if !defined (BLARGG_BIG_ENDIAN) && !defined (BLARGG_LITTLE_ENDIAN)
#ifdef __GLIBC__
	// GCC handles this for us
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define BLARGG_LITTLE_ENDIAN 1
#elif __BYTE_ORDER == __BIG_ENDIAN
#define BLARGG_BIG_ENDIAN 1
#endif
#else

#if defined (LSB_FIRST) || defined (__LITTLE_ENDIAN__) || BLARGG_CPU_X86 || \
		(defined (LITTLE_ENDIAN) && LITTLE_ENDIAN+0 != 1234)
#define BLARGG_LITTLE_ENDIAN 1
#endif

#if defined (MSB_FIRST)     || defined (__BIG_ENDIAN__) || defined (WORDS_BIGENDIAN) || \
	defined (__sparc__)     ||  BLARGG_CPU_POWERPC || \
	(defined (BIG_ENDIAN) && BIG_ENDIAN+0 != 4321)
#define BLARGG_BIG_ENDIAN 1
#elif !defined (__mips__)
	// No endian specified; assume little-endian, since it's most common
#define BLARGG_LITTLE_ENDIAN 1
#endif
#endif
#endif

#if BLARGG_LITTLE_ENDIAN && BLARGG_BIG_ENDIAN
#undef BLARGG_LITTLE_ENDIAN
#undef BLARGG_BIG_ENDIAN
#endif

inline void blargg_verify_byte_order()
{
#ifndef NDEBUG
#if BLARGG_BIG_ENDIAN
	volatile int i = 1;
	assert(*(volatile char*)& i == 0);
#elif BLARGG_LITTLE_ENDIAN
	volatile int i = 1;
	assert(*(volatile char*)& i != 0);
#endif
#endif
}

inline unsigned get_le16(void const* p)
{
	return  (unsigned)((unsigned char const*)p)[1] << 8 |
		(unsigned)((unsigned char const*)p)[0];
}

inline unsigned get_be16(void const* p)
{
	return  (unsigned)((unsigned char const*)p)[0] << 8 |
		(unsigned)((unsigned char const*)p)[1];
}

inline unsigned get_le32(void const* p)
{
	return  (unsigned)((unsigned char const*)p)[3] << 24 |
		(unsigned)((unsigned char const*)p)[2] << 16 |
		(unsigned)((unsigned char const*)p)[1] << 8 |
		(unsigned)((unsigned char const*)p)[0];
}

inline unsigned get_be32(void const* p)
{
	return  (unsigned)((unsigned char const*)p)[0] << 24 |
		(unsigned)((unsigned char const*)p)[1] << 16 |
		(unsigned)((unsigned char const*)p)[2] << 8 |
		(unsigned)((unsigned char const*)p)[3];
}

inline void set_le16(void* p, unsigned n)
{
	((unsigned char*)p)[1] = (unsigned char)(n >> 8);
	((unsigned char*)p)[0] = (unsigned char)n;
}

inline void set_be16(void* p, unsigned n)
{
	((unsigned char*)p)[0] = (unsigned char)(n >> 8);
	((unsigned char*)p)[1] = (unsigned char)n;
}

inline void set_le32(void* p, unsigned n)
{
	((unsigned char*)p)[0] = (unsigned char)n;
	((unsigned char*)p)[1] = (unsigned char)(n >> 8);
	((unsigned char*)p)[2] = (unsigned char)(n >> 16);
	((unsigned char*)p)[3] = (unsigned char)(n >> 24);
}

inline void set_be32(void* p, unsigned n)
{
	((unsigned char*)p)[3] = (unsigned char)n;
	((unsigned char*)p)[2] = (unsigned char)(n >> 8);
	((unsigned char*)p)[1] = (unsigned char)(n >> 16);
	((unsigned char*)p)[0] = (unsigned char)(n >> 24);
}

#if BLARGG_NONPORTABLE
// Optimized implementation if byte order is known
#if BLARGG_LITTLE_ENDIAN
#define GET_LE16( addr )        (*(BOOST::uint16_t const*) (addr))
#define GET_LE32( addr )        (*(BOOST::uint32_t const*) (addr))
#define SET_LE16( addr, data )  (void) (*(BOOST::uint16_t*) (addr) = (data))
#define SET_LE32( addr, data )  (void) (*(BOOST::uint32_t*) (addr) = (data))
#elif BLARGG_BIG_ENDIAN
#define GET_BE16( addr )        (*(BOOST::uint16_t const*) (addr))
#define GET_BE32( addr )        (*(BOOST::uint32_t const*) (addr))
#define SET_BE16( addr, data )  (void) (*(BOOST::uint16_t*) (addr) = (data))
#define SET_BE32( addr, data )  (void) (*(BOOST::uint32_t*) (addr) = (data))

#if BLARGG_CPU_POWERPC
	// PowerPC has special byte-reversed instructions
#if defined (__MWERKS__)
#define GET_LE16( addr )        (__lhbrx( addr, 0 ))
#define GET_LE32( addr )        (__lwbrx( addr, 0 ))
#define SET_LE16( addr, in )    (__sthbrx( in, addr, 0 ))
#define SET_LE32( addr, in )    (__stwbrx( in, addr, 0 ))
#elif defined (__GNUC__)
#define GET_LE16( addr )        ({unsigned short ppc_lhbrx_; __asm__ volatile( "lhbrx %0,0,%1" : "=r" (ppc_lhbrx_) : "r" (addr) : "memory" ); ppc_lhbrx_;})
#define GET_LE32( addr )        ({unsigned short ppc_lwbrx_; __asm__ volatile( "lwbrx %0,0,%1" : "=r" (ppc_lwbrx_) : "r" (addr) : "memory" ); ppc_lwbrx_;})
#define SET_LE16( addr, in )    ({__asm__ volatile( "sthbrx %0,0,%1" : : "r" (in), "r" (addr) : "memory" );})
#define SET_LE32( addr, in )    ({__asm__ volatile( "stwbrx %0,0,%1" : : "r" (in), "r" (addr) : "memory" );})
#endif
#endif
#endif
#endif

#ifndef GET_LE16
#define GET_LE16( addr )        get_le16( addr )
#define SET_LE16( addr, data )  set_le16( addr, data )
#endif

#ifndef GET_LE32
#define GET_LE32( addr )        get_le32( addr )
#define SET_LE32( addr, data )  set_le32( addr, data )
#endif

#ifndef GET_BE16
#define GET_BE16( addr )        get_be16( addr )
#define SET_BE16( addr, data )  set_be16( addr, data )
#endif

#ifndef GET_BE32
#define GET_BE32( addr )        get_be32( addr )
#define SET_BE32( addr, data )  set_be32( addr, data )
#endif

// auto-selecting versions

inline void set_le(BOOST::uint16_t* p, unsigned n) { SET_LE16(p, n); }
inline void set_le(BOOST::uint32_t* p, unsigned n) { SET_LE32(p, n); }
inline void set_be(BOOST::uint16_t* p, unsigned n) { SET_BE16(p, n); }
inline void set_be(BOOST::uint32_t* p, unsigned n) { SET_BE32(p, n); }
inline unsigned get_le(BOOST::uint16_t const* p) { return GET_LE16(p); }
inline unsigned get_le(BOOST::uint32_t const* p) { return GET_LE32(p); }
inline unsigned get_be(BOOST::uint16_t const* p) { return GET_BE16(p); }
inline unsigned get_be(BOOST::uint32_t const* p) { return GET_BE32(p); }

typedef const char blargg_err_def_t[];

// Basic errors
extern blargg_err_def_t blargg_err_generic;
extern blargg_err_def_t blargg_err_memory;
extern blargg_err_def_t blargg_err_caller;
extern blargg_err_def_t blargg_err_internal;
extern blargg_err_def_t blargg_err_limitation;

// File low-level
extern blargg_err_def_t blargg_err_file_missing; // not found
extern blargg_err_def_t blargg_err_file_read;
extern blargg_err_def_t blargg_err_file_write;
extern blargg_err_def_t blargg_err_file_io;
extern blargg_err_def_t blargg_err_file_full;
extern blargg_err_def_t blargg_err_file_eof;

// File high-level
extern blargg_err_def_t blargg_err_file_type;   // wrong file type
extern blargg_err_def_t blargg_err_file_feature;
extern blargg_err_def_t blargg_err_file_corrupt;

// C string describing error, or "" if err == NULL
const char* blargg_err_str(blargg_err_t err);

// True iff error is of given type, or false if err == NULL
bool blargg_is_err_type(blargg_err_t, const char type[]);

// Details of error without describing main cause, or "" if err == NULL
const char* blargg_err_details(blargg_err_t err);

// Converts error string to integer code using mapping table. Calls blargg_is_err_type()
// for each str and returns code on first match. Returns 0 if err == NULL.
struct blargg_err_to_code_t {
	const char* str;
	int code;
};
int blargg_err_to_code(blargg_err_t err, blargg_err_to_code_t const []);

// Converts error code back to string. If code == 0, returns NULL. If not in table,
// returns blargg_err_generic.
blargg_err_t blargg_code_to_err(int code, blargg_err_to_code_t const []);

// Generates error string literal with details of cause
#define BLARGG_ERR( type, str ) (type "; " str)

// Extra space to make it clear when blargg_err_str() isn't called to get
// printable version of error. At some point, I might prefix error strings
// with a code, to speed conversion to a code.
#define BLARGG_ERR_TYPE( str )  " " str

// Error types to pass to BLARGG_ERR macro
#define BLARGG_ERR_GENERIC      BLARGG_ERR_TYPE( "operation failed" )
#define BLARGG_ERR_MEMORY       BLARGG_ERR_TYPE( "out of memory" )
#define BLARGG_ERR_CALLER       BLARGG_ERR_TYPE( "internal usage bug" )
#define BLARGG_ERR_INTERNAL     BLARGG_ERR_TYPE( "internal bug" )
#define BLARGG_ERR_LIMITATION   BLARGG_ERR_TYPE( "exceeded limitation" )

#define BLARGG_ERR_FILE_MISSING BLARGG_ERR_TYPE( "file not found" )
#define BLARGG_ERR_FILE_READ    BLARGG_ERR_TYPE( "couldn't open file" )
#define BLARGG_ERR_FILE_WRITE   BLARGG_ERR_TYPE( "couldn't modify file" )
#define BLARGG_ERR_FILE_IO      BLARGG_ERR_TYPE( "read/write error" )
#define BLARGG_ERR_FILE_FULL    BLARGG_ERR_TYPE( "disk full" )
#define BLARGG_ERR_FILE_EOF     BLARGG_ERR_TYPE( "truncated file" )

#define BLARGG_ERR_FILE_TYPE    BLARGG_ERR_TYPE( "wrong file type" )
#define BLARGG_ERR_FILE_FEATURE BLARGG_ERR_TYPE( "unsupported file feature" )
#define BLARGG_ERR_FILE_CORRUPT BLARGG_ERR_TYPE( "corrupt file" )

#endif
