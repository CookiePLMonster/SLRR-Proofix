#define WIN32_LEAN_AND_MEAN

#define NOMINMAX
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502

#include <windows.h>
#include <cstdint>
#include "MemoryMgr.h"

// ============= QPC spoof for verifying high timer issues =============
namespace FakeQPC
{
	static int64_t AddedTime;
	static BOOL WINAPI FakeQueryPerformanceCounter(PLARGE_INTEGER lpPerformanceCount)
	{
		const BOOL result = ::QueryPerformanceCounter( lpPerformanceCount );
		lpPerformanceCount->QuadPart += AddedTime;
		return result;
	}
}

static BOOL WINAPI PostMessageFix( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	auto defLayout = GetKeyboardLayout(0);

	auto result = ActivateKeyboardLayout( reinterpret_cast<HKL>(lParam), KLF_SETFORPROCESS );
	//result = ActivateKeyboardLayout( defLayout, 0 );
	return TRUE;
}
static auto* const pPostMessageFix = &PostMessageFix;

static BOOL WINAPI PostMessageFix2( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	return TRUE;
}
static auto* const pPostMessageFix2 = &PostMessageFix2;


void* memmoveAndTerminate (void* dest, const void* src, size_t size)
{
	void* result = memmove( dest, src, size-1 );
	uint8_t* lastByte = static_cast<uint8_t*>(dest);
	lastByte[size-1] = 0;
	return result;
}

char* strncpyAndTerminate (char* strTo, const char* strFrom, size_t size)
{
	strncpy( strTo, strFrom, size-1 );
	strTo[size-1] = '\0';
	return strTo;
}

static size_t (*orgTestedFunction)(char*, char*);
size_t testedFunction(char* a, char* b)
{
	size_t result = orgTestedFunction(a, b);
	return result;
}

uint32_t GetFourChars(const uint8_t* str)
{
	int32_t num = 0;

	uint8_t ch = *str++;
	if ( ch != 0 )
	{
		num |= ch;

		ch = *str++;
		if ( ch != 0 )
		{
			num |= (ch << 8);

			ch = *str++;
			if ( ch != 0 )
			{
				num |= (ch << 16);

				ch = *str++;
				if ( ch != 0 )
				{
					num |= (ch << 24);
				}
			}
		}
	}

	return num;
}

void __declspec(naked) asmGetFourChars()
{
	_asm
	{
		push	ebp
		call	GetFourChars
		add		esp, 4
		retn
	}

}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		using namespace Memory::VP;

		{
			using namespace FakeQPC;

			constexpr size_t QPCDays = 1000000;

			LARGE_INTEGER Freq;
			QueryPerformanceFrequency( &Freq );
			AddedTime = Freq.QuadPart * QPCDays * 60 * 60 * 24;

			Memory::VP::Patch( 0x5850B8, &FakeQueryPerformanceCounter );
		}

		InjectHook( 0x52385A, memmoveAndTerminate );
		InjectHook( 0x48EB9A, memmoveAndTerminate );
		InjectHook( 0x52437F, memmoveAndTerminate );
		InjectHook( 0x526B2B, memmoveAndTerminate );
		InjectHook( 0x52558C, memmoveAndTerminate );

		InjectHook( 0x4068AE, strncpyAndTerminate );

		ReadCall( 0x4D011B, orgTestedFunction );
		InjectHook( 0x4D011B, testedFunction );


		auto jmp = []( uintptr_t addr, uintptr_t dest )
		{
			const ptrdiff_t offset = dest - (addr+2);
			if ( offset >= INT8_MIN && offset <= INT8_MAX )
			{
				Patch( addr, { 0xEB, static_cast<uint8_t>(offset) } );
			}
			else
			{
				InjectHook( addr, dest, PATCH_JUMP );
			}
		};

		jmp( 0x523890, 0x523889 );
		InjectHook( 0x523889, asmGetFourChars, PATCH_CALL );
		jmp( 0x523889 + 5, 0x523893 );




		/*// Keyboard layout fixes	
	//	Patch( 0x564659 + 2, &pLoadKeyboardLayoutAFix );
		Patch( 0x56466B + 2, &pPostMessageFix );
		Patch( 0x564916 + 2, &pPostMessageFix2 );
	//	Patch( 0x4DD56A + 2, &pSetFocus_KeyboardLayout );
		Patch<int32_t>( 0x56464D + 1, 0 );*/
		
	}
	return TRUE;
}