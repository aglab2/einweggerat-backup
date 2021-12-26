#pragma once
#include <Windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

#pragma comment(lib, "crypt32.lib") 

class Mud_Base64
{
public:
	static std::wstring encode(const BYTE* buf, unsigned int buflen, unsigned * outlen)
	{
		
		DWORD out_sz =0;
		if (CryptBinaryToStringW(buf, buflen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &out_sz)) {
			if (!out_sz) return {};
			std::wstring out(out_sz, 0);
			if (!CryptBinaryToStringW(buf, buflen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0], &out_sz))
			{
				*outlen = 0;
				return {};
			}
			*outlen = out_sz;
			return out;	
		}
		*outlen = 0;
		return {};
	

	}
	static std::vector<BYTE> decode(std::wstring string, int inlen, unsigned int * outlen)
	{
		DWORD out_sz = 0;
		if (CryptStringToBinaryW(string.c_str(), inlen, CRYPT_STRING_BASE64, NULL, &out_sz, NULL, NULL))
		{
			std::vector<BYTE> result(out_sz,0);
			CryptStringToBinaryW(string.c_str(), inlen, CRYPT_STRING_BASE64, (BYTE*)result.data(), &out_sz, NULL, NULL);
			*outlen = out_sz;
			return result;
		}
		else
			return {};
	}
};

class Mud_String
{
public:
	static std::string utf8toansi(std::string s)
	{
		return	utf16toansi(utf8toutf16(s));
	}

	static std::string ansitouf8(std::string s)
	{
		return	utf16toansi(ansitoutf16(s));
	}

	static std::string utf16toansi(const std::wstring &wstr)
	{
		if (wstr.empty()) return std::string();
		int size_needed = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	static std::wstring ansitoutf16(const std::string &str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstrTo[0], size_needed);
		return wstrTo;
	}

	static std::string utf16toutf8(const std::wstring &wstr)
	{
		if (wstr.empty()) return std::string();
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	static std::wstring utf8toutf16(const std::string &str)
	{
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}
};

class Mud_Timing
{
private:
	LARGE_INTEGER start;                   //
	LARGE_INTEGER end;                     //
	double perf_inv;
	void init_perfinv()
	{
		LARGE_INTEGER freq = {};
		if (!::QueryPerformanceFrequency(&freq) || freq.QuadPart == 0)
			return;
		perf_inv = 1000000. / (double)freq.QuadPart;
	}
public:
	void timer_start()
	{
		QueryPerformanceCounter(&start);
	}

	long long timer_elapsedusec()
	{
		QueryPerformanceCounter(&end);
		long long starttime = start.QuadPart * perf_inv;
		long long endtime = end.QuadPart * perf_inv;
		return endtime - starttime;
	}

	long long milliseconds_now() {
		return microseconds_now() * 0.001;
	}

	long long seconds_now()
	{
		return microseconds_now() *0.000001;
	}

	long long microseconds_now() {
		LARGE_INTEGER ts = {};
		if (!::QueryPerformanceCounter(&ts))
			return 0;
		return (uint64_t)(perf_inv * ts.QuadPart);
	}

	Mud_Timing()
	{
		perf_inv = 0;
		init_perfinv();
	}

	~Mud_Timing()
	{

	}
};

class Mud_FileAccess
{
public:
	static unsigned get_filesize(FILE* fp)
	{
		unsigned size = 0;
		unsigned pos = ftell(fp);
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, pos, SEEK_SET);
		return size;
	}

	static unsigned get_filesize(const TCHAR *path)
	{
		auto input = unique_ptr<FILE, int(*)(FILE*)>(_wfopen(path, L"rb"), &fclose);
		if (!input)return NULL;
		unsigned size = get_filesize(input.get());
		return size;
	}

	static bool save_data(unsigned char* data, unsigned size, TCHAR* path)
	{
		auto input = unique_ptr<FILE, int(*)(FILE*)>(_wfopen(path, L"wb"), &fclose);
		if (!input)return false;
		fwrite(data, 1, size, input.get());
		return true;
	}

	static std::vector<BYTE> load_data(TCHAR* path, unsigned * size)
	{
		auto input = unique_ptr<FILE, int(*)(FILE*)>(_wfopen(path, L"rb"), &fclose);
		if (!input)return {};
		unsigned Size = get_filesize(input.get());
		*size = Size;
		std::vector<BYTE> Memory(Size,0);
		int res = fread((BYTE*)Memory.data(), 1, Size, input.get());
		return Memory;
	}
};

class Mud_Misc
{
public:
	static void vector_appendbytes(std::vector<uint8_t> &vec ,uint8_t* bytes, size_t len)
	{
		vec.insert(vec.end(), bytes, bytes + len);
	}


	unsigned int crc32(const void* data, unsigned int length)
	{
		static const unsigned int tinf_crc32tab[16] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190,
		0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344,
		0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278,
		0xbdbdf21c
		};

		const unsigned char* buf = (const unsigned char*)data;
		unsigned int crc = 0xffffffff;
		unsigned int i;

		if (length == 0) return 0;

		for (i = 0; i < length; ++i)
		{
			crc ^= buf[i];
			crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
			crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
		}

		return crc ^ 0xffffffff;
	}



	static std::wstring ExePath() {
		TCHAR buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);
		wstring::size_type pos = wstring(buffer).find_last_of(L"\\/");
		return wstring(buffer).substr(0, pos);
	}

	static __forceinline uint32_t pow2up(uint32_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	static void redirectiotoconsole()
	{
		FILE *file = nullptr;
		freopen_s(&file, "CONOUT$", "w", stdout);
		freopen_s(&file, "CONOUT$", "w", stderr);
	}

	static LPSTR* cmdlinetoargANSI(LPSTR lpCmdLine, INT *pNumArgs)
	{
		int retval;
		retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, NULL, 0);
		if (!SUCCEEDED(retval))
			return NULL;

		LPWSTR lpWideCharStr = (LPWSTR)malloc(retval * sizeof(WCHAR));
		if (lpWideCharStr == NULL)
			return NULL;

		retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, lpWideCharStr, retval);
		if (!SUCCEEDED(retval))
		{
			free(lpWideCharStr);
			return NULL;
		}

		int numArgs;
		LPWSTR* args;
		args = CommandLineToArgvW(lpWideCharStr, &numArgs);
		free(lpWideCharStr);
		if (args == NULL)
			return NULL;

		int storage = numArgs * sizeof(LPSTR);
		for (int i = 0; i < numArgs; ++i)
		{
			BOOL lpUsedDefaultChar = FALSE;
			retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, &lpUsedDefaultChar);
			if (!SUCCEEDED(retval))
			{
				LocalFree(args);
				return NULL;
			}

			storage += retval;
		}

		LPSTR* result = (LPSTR*)LocalAlloc(LMEM_FIXED, storage);
		if (result == NULL)
		{
			LocalFree(args);
			return NULL;
		}

		int bufLen = storage - numArgs * sizeof(LPSTR);
		LPSTR buffer = ((LPSTR)result) + numArgs * sizeof(LPSTR);
		for (int i = 0; i < numArgs; ++i)
		{

			BOOL lpUsedDefaultChar = FALSE;
			retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, buffer, bufLen, NULL, &lpUsedDefaultChar);
			if (!SUCCEEDED(retval))
			{
				LocalFree(result);
				LocalFree(args);
				return NULL;
			}

			result[i] = buffer;
			buffer += retval;
			bufLen -= retval;
		}

		LocalFree(args);

		*pNumArgs = numArgs;
		return result;
	}
};
