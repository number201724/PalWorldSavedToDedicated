#include <Windows.h>
#include <atlstr.h>
#include <atlconv.h>
#include <detours.h>
#include <stdint.h>

#define CAPTION_TEXT  L"PalServerHost"

CString GetEnvironmentVariableString( VOID )
{
	CString szEnvironmentVariable;
	DWORD dwSize;
	LPTSTR lpszEnvironmentVariable;
	DWORD dwLen;


	dwSize = 1024;

	do
	{
		lpszEnvironmentVariable = szEnvironmentVariable.GetBuffer( dwSize );
		dwLen = GetEnvironmentVariable( TEXT( "PATH" ), lpszEnvironmentVariable, dwSize );
		szEnvironmentVariable.ReleaseBuffer();

		if ( dwLen < dwSize )
			break;

		dwSize = dwLen;

	} while ( TRUE );

	return szEnvironmentVariable;
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow )
{
	CStringW strCurDir;
	CStringW strDllFileName, strConfigFile;
	CStringW strExecDir, strExePath;
	CStringW strCommandLine;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	WCHAR wcsHostSteamIdStr[64];
	uint64_t steamid;
	USES_CONVERSION;

	LPWSTR lpszCurDir = strCurDir.GetBuffer( MAX_PATH );

	GetModuleFileNameW( (HMODULE)hInstance, lpszCurDir, MAX_PATH );

	LPWSTR lpTemp = wcsrchr( lpszCurDir, L'\\' );
	if ( lpTemp ) {
		*lpTemp = L'\0';
	}

	strCurDir.ReleaseBuffer();

	strDllFileName.Format( L"%s%s", strCurDir, L"\\PalHost.dll" );
	strConfigFile.Format( L"%s%s", strCurDir, L"\\PalHost.ini" );

	if ( !PathFileExistsW( strDllFileName ) ) {
		MessageBoxW( NULL, L"The PalHost.dll file does not exist!", CAPTION_TEXT, MB_ICONERROR );
		ExitProcess( 0 );
	}

	if ( !PathFileExistsW( strConfigFile ) ) {
		MessageBox( NULL, L"The PalHost.ini file does not exist!", CAPTION_TEXT, MB_ICONERROR );
		ExitProcess( 0 );
	}

	GetPrivateProfileStringW( L"PalServerHost", L"HostSteamId", L"", wcsHostSteamIdStr, ARRAYSIZE( wcsHostSteamIdStr ), strConfigFile );
	wcsHostSteamIdStr[ARRAYSIZE( wcsHostSteamIdStr ) - 1] = L'\0';

	steamid = wcstoull( wcsHostSteamIdStr, NULL, 10 );
	if ( steamid == 0 ) {
		MessageBox( NULL, L"Please fill in the Host SteamId in PalHost.ini.", CAPTION_TEXT, MB_ICONERROR );
		ExitProcess( 0 );
	}

	strExecDir.Format( L"%s%s", strCurDir, L"\\Pal\\Binaries\\Win64" );
	strExePath.Format( L"%s\\%s", strExecDir, L"PalServer-Win64-Shipping-Cmd.exe" );

	if ( !PathFileExistsW( strExePath ) ) {
		strExePath.Format( L"%s\\%s", strExecDir, L"PalServer-Win64-Shipping.exe" );
	}

	if ( !PathFileExistsW( strExePath ) ) {
		MessageBox( NULL, L"PalServerHost.exe and PalServerHost.dll are placed in the PalServer directory.", CAPTION_TEXT, MB_ICONERROR );
		ExitProcess( 0 );
	}

	RtlZeroMemory( &si, sizeof( si ) );
	RtlZeroMemory( &pi, sizeof( pi ) );
	si.cb = sizeof( si );

	LPWSTR lpCommandLine = GetCommandLineW();
	size_t nLen = _tcslen( lpCommandLine );

	if ( *lpCommandLine == '\"' )
	{
		LPWSTR lpTemp = wcschr( lpCommandLine + 1, '\"' );

		if ( !lpTemp )
		{
			MessageBoxW( NULL, L"unknown error 1.", CAPTION_TEXT, MB_ICONERROR );
			ExitProcess( 0 );
		}

		lpCommandLine = lpTemp + 1;
	}
	else
	{
		LPWSTR lpTemp = wcsstr( lpCommandLine, L".exe" );
		if ( !lpTemp )
		{
			MessageBoxW( NULL, L"unknown error 1.", CAPTION_TEXT, MB_ICONERROR );
			ExitProcess( 0 );
		}

		lpCommandLine = lpTemp + 4;
	}

	strCommandLine.Format( L"\"%s\" Pal %s", strExePath, lpCommandLine );

	// Steam Environment
	CString szEnvironmentVariable = GetEnvironmentVariableString();
	CString szTemp;
	szTemp.Format( TEXT( ";%s" ), strCurDir );
	szEnvironmentVariable.Append( szTemp );
	SetEnvironmentVariable( TEXT( "PATH" ), szEnvironmentVariable );

	LPCSTR lpDllFileName = T2A( (LPCTSTR)strDllFileName );

	if ( wcsstr( lpCommandLine, L"-nohostfix" ) != NULL )
	{
		if ( !CreateProcessW( strExePath, strCommandLine.GetBuffer(), NULL, NULL, FALSE, 0, NULL, strExecDir, &si, &pi ) )
		{
			MessageBox( NULL, L"Creating process failed.", CAPTION_TEXT, MB_ICONERROR );
			ExitProcess( 0 );
		}

	}

	else
	{
		if ( !DetourCreateProcessWithDllW( strExePath, strCommandLine.GetBuffer(), NULL, NULL, FALSE, 0, NULL, strExecDir, &si, &pi, lpDllFileName, CreateProcessW ) )
		{
			MessageBox( NULL, L"Creating process failed.", CAPTION_TEXT, MB_ICONERROR );
			ExitProcess( 0 );
		}
	}

	strCommandLine.ReleaseBuffer();

	WaitForSingleObject( pi.hProcess, INFINITE );

	return 0;
}