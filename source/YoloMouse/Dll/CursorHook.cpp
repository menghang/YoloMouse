#include <YoloMouse/Dll/CursorHook.hpp>
#include <YoloMouse/Share/SharedTools.hpp>

namespace YoloMouse
{
    // private
    //-------------------------------------------------------------------------
    Bool            CursorHook::_active         (false);
    CursorBindings  CursorHook::_bindings;
    HCURSOR         CursorHook::_last_cursor    (NULL);
    Char            CursorHook::_target_id      [STRING_PATH_SIZE];
    HCURSOR         CursorHook::_replace_cursor (NULL);
    Bool            CursorHook::_assign_ready   (false);
    Index           CursorHook::_assign_index   (INVALID_INDEX);
    Hook            CursorHook::_hook           (SetCursor, CursorHook::_OnSetCursor);
    SharedState&    CursorHook::_state =        SharedState::Instance();

     // public
    //-------------------------------------------------------------------------
    void CursorHook::Load( HWND hwnd )
    {
        // if valid index
        if( !_active )
        {
            // load state
            if(_state.Open(false))
            {
                // enable hook
                if(_hook.Init())
                {
                    // activate
                    _active = true;

                    // build id string
                    if(SharedTools::BuildTargetId(_target_id, sizeof(_target_id), hwnd))
                    {
                        // enable hook
                        _hook.Enable();

                        // load cursor map from file
                        _bindings.Load(_target_id);

                        // refresh cursor
                        _RefreshCursor(hwnd);
                    }
                }
            }
        }
    }

    void CursorHook::Unload()
    {
        // disable hook
        if( _active )
        {
            // unload state
            _state.Close();

            // disable hook
            _hook.Disable();

            // deactivate
            _assign_index = INVALID_INDEX;
            _assign_ready = false;
            _active = false;
        }
    }

    //-------------------------------------------------------------------------
    void CursorHook::Assign( HWND hwnd, Index cursor_index )
    {
        xassert(_active);

        // mark for update
        _assign_index = cursor_index;
        _assign_ready = true;

        // refresh cursor
        _RefreshCursor(hwnd);
    }

    // private
    //-------------------------------------------------------------------------
    void CursorHook::_RefreshCursor( HWND hwnd )
    {
        // get refresh cursor
        HCURSOR refresh_cursor = _last_cursor;

        // if does not exist
        if( refresh_cursor == NULL )
        {
            // get active windows cursor
            refresh_cursor = GetCursor();

            // cannot be yolomouse cursor
            if( _state.FindCursor(refresh_cursor) != INVALID_INDEX )
                refresh_cursor = NULL;
        }
            
        // set current cursor to force update
        SetCursor(refresh_cursor);

        // then trigger application to call SetCursor with its own cursor
        PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
    }

    //-------------------------------------------------------------------------
    Bool CursorHook::_OnSetCursorAssign( HCURSOR hcursor, Index cursor_index )
    {
        // cannot be yolomouse cursor
        if( _state.FindCursor(hcursor) != INVALID_INDEX )
            return false;

        // calculate hash of cursor
        Hash cursor_hash = SharedTools::CalculateCursorHash(hcursor);

        // fail if invalid
        if( cursor_hash == 0 )
            return false;

        // find cursor mapping
        Index mapping_index = _bindings.Find(cursor_hash);

        // if removing
        if( cursor_index == INVALID_INDEX )
        {
            // require mapping exist
            if( mapping_index == INVALID_INDEX )
                return false;

            // remove mapping
            _bindings.Remove(mapping_index);
        }
        // else if adding
        else
        {
            // require cursor exists
            if( _state.GetCursor(cursor_index) == NULL )
                return false;

            // remove mapping if already exists
            if( mapping_index != INVALID_INDEX )
                _bindings.Remove(mapping_index);
     
            // add mapping
            if( _bindings.Add(cursor_hash, cursor_index) == INVALID_INDEX )
                return false;
        }

        // save cursor map to file
        _bindings.Save(_target_id);

        return true;
    }

    //-------------------------------------------------------------------------
    Bool CursorHook::_OnSetCursorChange( HCURSOR hcursor )
    {
        // cannot be yolomouse cursor
        if( _state.FindCursor(hcursor) != INVALID_INDEX )
            return false;

        // calculate hash of cursor
        Hash cursor_hash = SharedTools::CalculateCursorHash(hcursor);

        // fail if invalid
        if( cursor_hash == 0 )
            return false;

        // reset state
        _replace_cursor = NULL;

        // find cursor mapping
        Index mapping_index = _bindings.Find(cursor_hash);

        // if found
        if( mapping_index != INVALID_INDEX )
        {
            // update replacement cursor
            _replace_cursor = _state.GetCursor(_bindings.Get(mapping_index)._index);
        }

        return true;
    }

    //-------------------------------------------------------------------------
    VOID HOOK_CALL CursorHook::_OnSetCursor( x86::Registers registers )
    {
        xassert(_active);
        HCURSOR hcursor = *(HCURSOR*)(registers.esp + 4);
        Bool    assign_ready = _assign_ready;
        Index   assign_index = _assign_index;

        // ignore null
        if( hcursor == NULL )
            return;

        // reset assign state
        _assign_ready = false;

        // if assigning new cursor
        if( assign_ready )
        {
            // handle cursor assign
            if(!_OnSetCursorAssign(hcursor, assign_index))
                return;

            // reset last cursor to force upate
            _last_cursor = NULL;
        }

        // if cursor changing
        if( hcursor != _last_cursor )
        {
            // handle cursor change
            if(!_OnSetCursorChange(hcursor))
                return;

            // set new last cursor
            _last_cursor = hcursor;
        }

        // replace hcursor parameter before call to SetCursor
        if( _replace_cursor )
            *(HCURSOR*)(registers.esp + 4) = _replace_cursor;
    }
}