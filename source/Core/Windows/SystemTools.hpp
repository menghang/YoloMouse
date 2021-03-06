#pragma once
#include <Core/Constants.hpp>

namespace Core
{
    /**/
    class SystemTools
    {
    public:
        /**/
        static Bitness GetProcessBitness( HANDLE process );

        /**/
        static OsVersion GetOsVersion();

        /**/
        static Bool EnableAutoStart( const WCHAR* name, const WCHAR* path, Bool enable );

        /**/
        static Bool GetProcessDirectory( WCHAR* path, ULong limit );
    };
}
