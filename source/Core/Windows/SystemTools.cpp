#include <Core/Windows/SystemTools.hpp>
#include <Core/Support/Tools.hpp>
#include <psapi.h>
#include <stdio.h>

namespace Core
{
    // public
    //-------------------------------------------------------------------------
    Bitness SystemTools::GetProcessBitness( HANDLE process )
    {
        SYSTEM_INFO system_info;

        // check if everything is 32bit
        GetNativeSystemInfo(&system_info);

        // select cpu architecture
        switch( system_info.wProcessorArchitecture )
        {
        // 64bit
        case PROCESSOR_ARCHITECTURE_AMD64:
        case PROCESSOR_ARCHITECTURE_IA64:
            {
                BOOL is_wow64;

                // get wow64 (emulated 32) status
                if( process==NULL || !IsWow64Process(process, &is_wow64) )
                    return BITNESS_UNKNOWN;

                // if emulated then 32 else 64
                return is_wow64 ? BITNESS_32 : BITNESS_64;
            }

        // 32bit
        case PROCESSOR_ARCHITECTURE_INTEL:
            return BITNESS_32;
        }

        return BITNESS_UNKNOWN;
    }

    //-------------------------------------------------------------------------
    OsVersion SystemTools::GetOsVersion()
    {
        // build comparison mask
        DWORD version = GetVersion();

        // extra major and minor
        return (OsVersion)((LOBYTE(LOWORD(version)) << 8) | HIBYTE(LOWORD(version)));
    }

    //-------------------------------------------------------------------------
    Bool SystemTools::EnableAutoStart( const WCHAR* name, const WCHAR* path, Bool enable )
    {
        const WCHAR* REGISTRY_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        Bool status = false;
        HKEY hkey;

        // if enabling
        if(enable)
        {
            // open/create key
            if(RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, 0, KEY_SET_VALUE, NULL, &hkey, NULL) == ERROR_SUCCESS)
            {
                // set value
                if(RegSetValueEx (hkey, name, 0, REG_SZ, (Byte*)path, (DWORD)(wcslen(path) * sizeof(WCHAR))) == ERROR_SUCCESS)
                    status = true;

                // close key
                RegCloseKey(hkey);
            }
        }
        // open key
        else if( RegOpenKey(HKEY_CURRENT_USER, REGISTRY_PATH, &hkey) == ERROR_SUCCESS )
        {
            // delete value
            if( RegDeleteValue(hkey, name) == ERROR_SUCCESS )
                status = true;

            // close key
            RegCloseKey(hkey);
        }

        return status;
    }

    //-------------------------------------------------------------------------
    Bool SystemTools::GetProcessDirectory( WCHAR* path, ULong limit )
    {
        if( GetModuleFileName( NULL, path, limit ) == 0 )
            return false;

        Tools::StripFileName(path);
        return true;
    }
}
