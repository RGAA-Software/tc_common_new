//
// Created by RGAA on 11/08/2024.
//

#include "key_helper.h"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace tc
{

    bool KeyHelper::IsKeyPressed(int vk) {
#ifdef WIN32
        return GetKeyState(vk) < 0;
#else
        return false;
#endif
    }

    bool KeyHelper::IsShiftPressed() {
        return IsKeyPressed(VK_LSHIFT) || IsKeyPressed(VK_RSHIFT);
    }

    bool KeyHelper::IsControlPressed() {
        return IsKeyPressed(VK_LCONTROL) || IsKeyPressed(VK_RCONTROL);
    }

    bool KeyHelper::IsAltPressed() {
        return IsKeyPressed(VK_LMENU) || IsKeyPressed(VK_RMENU);
    }

    bool KeyHelper::IsWinPressed() {
        return IsKeyPressed(VK_LWIN) || IsKeyPressed(VK_RWIN);
    }

    bool KeyHelper::IsCapsLockPressed() {
        return IsKeyPressed(VK_CAPITAL);
    }

    bool KeyHelper::IsNumLockPressed() {
        return IsKeyPressed(VK_NUMLOCK);
    }


}