#include "pch.h"
#include <atlstr.h>
#include <unordered_set>
#include <string>

class UPocketpairUserSubsystem;
class FSocialId;
class APlayerController;
class APalPlayerState;



CStringW gstrDllFileName;
CStringW gstrIniFileName;
HMODULE hDedicatedMod;
CStringW gstrHostSteamId;


std::unordered_set<ULONG_PTR> gFunctionList;

FSocialId* (*g_pfnGetSocialId)(UPocketpairUserSubsystem *pThis, FSocialId *result, const APlayerController *PlayerController);
void (*g_pfnSetupPlayerProcess_InServer)(APalPlayerState *pThis);
APalPlayerState *g_pPlayerState;
extern "C"
{
	void (*g_pfnAfterPatchSocialId)();
	void AsmAfterPatchSocialId();
};

template<typename InFromType, typename InToType = InFromType>
class TStringPointer
{
public:
	typedef InFromType FromType;
	typedef InToType   ToType;

	const ToType* Ptr;
	int32_t StringLength;
};

uint32_t( *g_pfnHashString )(const TStringPointer<wchar_t, unsigned short> *InStr);
DWORD g_dwSetupPlayerProcessThreadId;

DWORD g_dwLoginTryingPlayerUId_InServer;

BOOL g_bSetupPlayerProcess;
CRITICAL_SECTION g_cs;
std::wstring g_szSocialId;

struct FGuid
{
	uint32_t A;
	uint32_t B;
	uint32_t C;
	uint32_t D;
};


ULONG GetInstructionSize( PVOID pSrc )
{
	PVOID pDst;

	pDst = DetourCopyInstruction( NULL, NULL, pSrc, NULL, NULL );

	if ( pDst == NULL ) {
		return 0;
	}

	return (ULONG)((ULONG_PTR)pDst - (ULONG_PTR)pSrc);
}

uint32_t HashString( const TStringPointer<wchar_t, unsigned short> *InStr )
{
	if ( g_bSetupPlayerProcess && GetCurrentThreadId() == g_dwSetupPlayerProcessThreadId )
	{
		if ( InStr->StringLength == -1 )
		{
			int StringLength = (int)wcslen( (wchar_t*)InStr->Ptr );

			g_szSocialId = (wchar_t*)InStr->Ptr;
		}
		else
		{
			g_szSocialId = std::wstring( (wchar_t*)InStr->Ptr, InStr->StringLength );
		}
	}
	return g_pfnHashString( InStr );
}

//
// FSocialId UPocketpairUserSubsystem::GetSocialId(const APlayerController *PlayerController);
// 
FSocialId *GetSocialId( UPocketpairUserSubsystem *pThis, FSocialId *result, const APlayerController *PlayerController )
{
	result = g_pfnGetSocialId( pThis, result, PlayerController );
	return result;
}

void SetupPlayerProcess_InServer( APalPlayerState *pThis )
{
	EnterCriticalSection( &g_cs );
	g_szSocialId.clear();

	g_dwSetupPlayerProcessThreadId = GetCurrentThreadId();
	g_bSetupPlayerProcess = TRUE;

	g_pPlayerState = pThis;
	g_pfnSetupPlayerProcess_InServer( pThis );

	g_bSetupPlayerProcess = FALSE;

	LeaveCriticalSection( &g_cs );
}

extern "C" void AfterPatchSocialId()
{
	FGuid &Guid = *(FGuid *)((ULONG_PTR)g_pPlayerState + g_dwLoginTryingPlayerUId_InServer);

	if ( !g_szSocialId.empty() )
	{
		if ( wcscmp( g_szSocialId.c_str(), gstrHostSteamId ) == 0 )
		{
			Guid.A = 0;
			Guid.B = 0;
			Guid.C = 0;
			Guid.D = 1;
		}
	}

}

PVOID SearchUnicodeStringFunction( PUCHAR lpBase, PUCHAR pBaseRData, PUCHAR pEndOfRData, LPCWSTR lpString, DWORD dwLength, DWORD dwHeadOffset )
{
	for ( int i = 0; i < 0x400; i++ )
	{
		if ( !(lpBase[i] == 0x48 && lpBase[i + 1] == 0x8D) )
			continue;

		ULONG_PTR lpTarget = ((ULONG_PTR)&lpBase[i] + *(LONG *)&lpBase[i + 3] + 0x7);

		if ( !(lpTarget > ( ULONG_PTR )pBaseRData && lpTarget < (ULONG_PTR)pEndOfRData) )
			continue;

		if ( wcsncmp( (PWCHAR)lpTarget, lpString, dwLength ) == 0 )
		{
			PUCHAR pFuncHead = NULL;
			for ( PUCHAR address = &lpBase[i]; address >= lpBase; address-- )
			{
				if ( !(address[0] == 0x48 && address[1] == 0x81 && address[2] == 0xEC) )
					continue;

				pFuncHead = address;
				break;
			}

			for ( DWORD k = 0; k < dwHeadOffset; k++ )
			{
				if ( gFunctionList.find( (ULONG_PTR)(pFuncHead - k) ) != gFunctionList.end() )
				{
					return (PVOID)(ULONG_PTR)(pFuncHead - k);
				}
			}
		}
	}

	return NULL;
}

PVOID SearchHashStringFunction( PUCHAR pBase, PUCHAR pBaseRData, PUCHAR pEndOfRData )
{
	PUCHAR pPos1 = pBase;
	PUCHAR pPos2 = NULL;

	for ( int i = 0; i < 256; i++ )
	{

		if ( pPos1[0] == 0x48 && pPos1[1] == 0x8D )
		{
			ULONG_PTR lpTarget = ((ULONG_PTR)&pPos1[0] + *(LONG *)&pPos1[3] + 0x7);
			if ( lpTarget > ( ULONG_PTR )pBaseRData && lpTarget < (ULONG_PTR)pEndOfRData )
			{
				if ( wcsncmp( (PWCHAR)lpTarget, L"STEAM", 5 ) == 0 )
				{
					pPos2 = pPos1 + 7;
					break;
				}
			}

		}

		pPos1 += GetInstructionSize( pPos1 );
	}

	if ( !pPos2 )
		return NULL;

	int nCalls = 0;

	for ( int i = 0; i < 256; i++ )
	{
		if ( pPos2[0] == 0xE8 )
		{
			nCalls++;

			if ( nCalls == 3 )
			{
				ULONG_PTR lpTarget = ((ULONG_PTR)&pPos2[0] + *(LONG *)&pPos2[1] + 0x5);
				return (PVOID)lpTarget;
			}
		}

		pPos2 += GetInstructionSize( pPos2 );
	}

	return NULL;
}

void SearchAfterPatchSocialId()
{
	if ( !g_pfnSetupPlayerProcess_InServer || !g_pfnGetSocialId )
		return;

	if ( g_pfnAfterPatchSocialId )
		return;

	PUCHAR pBase = (PUCHAR)g_pfnSetupPlayerProcess_InServer;
	PUCHAR pPos1 = NULL;

	for ( int i = 0; i < 100; i++ )
	{
		if ( pBase[0] == 0xE8 )
		{
			ULONG_PTR pTarget = (ULONG_PTR)&pBase[0] + *(LONG *)&pBase[1] + 0x5;

			if ( pTarget == (ULONG_PTR)g_pfnGetSocialId )
			{
				pBase += 5;
				pPos1 = pBase;
				break;
			}
		}

		pBase += GetInstructionSize( pBase );
	}

	if ( !pPos1 )
		return;

	PUCHAR pPos2 = NULL;
	for ( int i = 0; i < 20; i++ )
	{
		if ( pPos1[0] == 0xE8 && pPos1[5] == 0x48 )
		{
			*(PVOID*)&g_pfnAfterPatchSocialId = (PVOID)pPos1;
			pPos2 = pPos1;
			break;
		}

		pPos1 += GetInstructionSize( pPos1 );
	}

	if ( !pPos2 )
		return;

	for ( int i = 0; i < 20; i++ )
	{
		if ( pBase[0] == 0x48 && pBase[1] == 0x8D )
		{
			g_dwLoginTryingPlayerUId_InServer = *(DWORD*)&pBase[3];
			break;
		}

		pBase += GetInstructionSize( pBase );
	}
}

void PalServerHostInit()
{
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;


	DosHeader = (PIMAGE_DOS_HEADER)hDedicatedMod;


	if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE )
	{
		return;
	}

	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)hDedicatedMod + DosHeader->e_lfanew);
	if ( NtHeader->Signature != IMAGE_NT_SIGNATURE ) {
		return;
	}

	SectionHeader = IMAGE_FIRST_SECTION( NtHeader );

	PIMAGE_DATA_DIRECTORY pExceptionDataDir = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
	if ( pExceptionDataDir->Size == 0 )
		return;

	PRUNTIME_FUNCTION pRuntimeFunction = (PRUNTIME_FUNCTION)((ULONG_PTR)hDedicatedMod + pExceptionDataDir->VirtualAddress);
	DWORD dwExceptionDataCount = (pExceptionDataDir->Size / sizeof( RUNTIME_FUNCTION ));

	for ( DWORD i = 0; i < dwExceptionDataCount; i++ ) {
		gFunctionList.insert( ((ULONG_PTR)hDedicatedMod + pRuntimeFunction[i].BeginAddress) );
	}

	PUCHAR pBaseRData = NULL, pEndOfRData = NULL;

	for ( WORD o = 0; o < NtHeader->FileHeader.NumberOfSections; o++ )
	{
		if ( strncmp( (const char *)&SectionHeader[o].Name, ".rdata", 6 ) == 0 )
		{
			pBaseRData = (PUCHAR)((ULONG_PTR)hDedicatedMod + SectionHeader[o].VirtualAddress);
			pEndOfRData = pBaseRData + SectionHeader[o].Misc.VirtualSize;
			break;
		}
	}

	if ( !pBaseRData )
		return;


	for ( WORD o = 0; o < NtHeader->FileHeader.NumberOfSections; o++ )
	{
		if ( !(SectionHeader[o].Characteristics & IMAGE_SCN_MEM_EXECUTE) )
			continue;

		PUCHAR lpBase = (PUCHAR)((ULONG_PTR)hDedicatedMod + SectionHeader[o].VirtualAddress);

		if ( SectionHeader[o].Misc.VirtualSize > 0x100 )
		{
			for ( DWORD i = 0; (i + 0x800) < SectionHeader[o].Misc.VirtualSize; i++ )
			{
				if ( !(lpBase[i] == 0x48 && lpBase[i + 1] == 0x81 && lpBase[i + 2] == 0xEC) )
					continue;

				if ( g_pfnGetSocialId == NULL )
				{
					*(PVOID *)&g_pfnGetSocialId = SearchUnicodeStringFunction( &lpBase[i], pBaseRData, pEndOfRData, L"DESKTOP-", 8, 16 );
					SearchAfterPatchSocialId();


					if ( g_pfnGetSocialId )
					{
						*(PVOID*)&g_pfnHashString = SearchHashStringFunction( (PUCHAR)g_pfnGetSocialId, pBaseRData, pEndOfRData );
					}
				}

				if ( g_pfnSetupPlayerProcess_InServer == NULL )
				{
					*(PVOID *)&g_pfnSetupPlayerProcess_InServer = SearchUnicodeStringFunction( &lpBase[i],
																							   pBaseRData,
																							   pEndOfRData,
																							   L"Start SetupPlayerProcess_InServer",
																							   (DWORD)wcslen( L"Start SetupPlayerProcess_InServer" ),
																							   64
					);

					SearchAfterPatchSocialId();
				}

			}
		}
	}

	InitializeCriticalSection( &g_cs );

	if ( !g_pfnGetSocialId ) {
		MessageBoxW( NULL, L"PalServerHost initialization failed, error code: 1", L"PalServerHost", MB_ICONERROR );
		ExitProcess( 0 );
	}

	if ( !g_pfnHashString ) {
		MessageBoxW( NULL, L"PalServerHost initialization failed, error code: 2", L"PalServerHost", MB_ICONERROR );
		ExitProcess( 0 );
	}

	if ( !g_pfnSetupPlayerProcess_InServer ) {
		MessageBoxW( NULL, L"PalServerHost initialization failed, error code: 3", L"PalServerHost", MB_ICONERROR );
		ExitProcess( 0 );
	}

	if ( !g_pfnAfterPatchSocialId ) {
		MessageBoxW( NULL, L"PalServerHost initialization failed, error code: 4", L"PalServerHost", MB_ICONERROR );
		ExitProcess( 0 );
	}

	if ( g_dwLoginTryingPlayerUId_InServer == 0 ) {
		MessageBoxW( NULL, L"PalServerHost initialization failed, error code: 5", L"PalServerHost", MB_ICONERROR );
		ExitProcess( 0 );
	}

	DetourTransactionBegin();
	DetourAttach( (PVOID*)&g_pfnGetSocialId, GetSocialId );
	DetourAttach( (PVOID*)&g_pfnHashString, HashString );
	DetourAttach( (PVOID*)&g_pfnSetupPlayerProcess_InServer, SetupPlayerProcess_InServer );
	DetourAttach( (PVOID*)&g_pfnAfterPatchSocialId, AsmAfterPatchSocialId );
	DetourTransactionCommit();


	gFunctionList.clear();
}


void PalServerHostShutDown( void )
{

}


BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  dwReason,
					   LPVOID lpReserved
)
{
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
		DisableThreadLibraryCalls( hModule );

		GetModuleFileNameW( hModule, gstrDllFileName.GetBuffer( MAX_PATH ), MAX_PATH );
		gstrDllFileName.ReleaseBuffer();

		gstrIniFileName = gstrDllFileName;

		LPWSTR lpwcs = gstrIniFileName.GetBuffer();
		int FileNameLen = gstrIniFileName.GetLength();
		lpwcs[FileNameLen - 3] = L'i';
		lpwcs[FileNameLen - 2] = L'n';
		lpwcs[FileNameLen - 1] = L'i';
		gstrIniFileName.ReleaseBuffer();


		GetPrivateProfileStringW( L"PalServerHost", L"HostSteamId", L"", gstrHostSteamId.GetBuffer( 128 ), 128, gstrIniFileName );
		gstrHostSteamId.ReleaseBuffer();


		hDedicatedMod = GetModuleHandle( NULL );

		PalServerHostInit();
	}

	if ( dwReason == DLL_PROCESS_DETACH )	//
	{
		PalServerHostShutDown();
	}

	return TRUE;
}

extern "C" __declspec(dllexport) void E()
{
}