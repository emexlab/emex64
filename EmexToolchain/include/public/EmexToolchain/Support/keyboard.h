/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EMEX64_KEYBOARD_H
#define EMEX64_KEYBOARD_H

#include <EmexFoundation/EmexFoundation.h>

typedef enum: UInt8 {
    kEmexKeyPhysEsc,
    kEmexKeyPhysF1,
    kEmexKeyPhysF2,
    kEmexKeyPhysF3,
    kEmexKeyPhysF4,
    kEmexKeyPhysF5,
    kEmexKeyPhysF6,
    kEmexKeyPhysF7,
    kEmexKeyPhysF8,
    kEmexKeyPhysF9,
    kEmexKeyPhysF10,
    kEmexKeyPhysF11,
    kEmexKeyPhysF12,
    kEmexKeyPhysGrave,
    kEmexKeyPhys1,
    kEmexKeyPhys2,
    kEmexKeyPhys3,
    kEmexKeyPhys4,
    kEmexKeyPhys5,
    kEmexKeyPhys6,
    kEmexKeyPhys7,
    kEmexKeyPhys8,
    kEmexKeyPhys9,
    kEmexKeyPhys0,
    kEmexKeyPhysMinus,
    kEmexKeyPhysEqual,
    kEmexKeyPhysBackspace,
    kEmexKeyPhysTab,
    kEmexKeyPhysQ,
    kEmexKeyPhysW,
    kEmexKeyPhysE,
    kEmexKeyPhysR,
    kEmexKeyPhysT,
    kEmexKeyPhysY,
    kEmexKeyPhysU,
    kEmexKeyPhysI,
    kEmexKeyPhysO,
    kEmexKeyPhysP,
    kEmexKeyPhysLeftBracket,
    kEmexKeyPhysRightBracket,
    kEmexKeyPhysBackslash,
    kEmexKeyPhysCapsLock,
    kEmexKeyPhysA,
    kEmexKeyPhysS,
    kEmexKeyPhysD,
    kEmexKeyPhysF,
    kEmexKeyPhysG,
    kEmexKeyPhysH,
    kEmexKeyPhysJ,
    kEmexKeyPhysK,
    kEmexKeyPhysL,
    kEmexKeyPhysSemicolon,
    kEmexKeyPhysQuote,
    kEmexKeyPhysEnter,
    kEmexKeyPhysLeftShift,
    kEmexKeyPhysZ,
    kEmexKeyPhysX,
    kEmexKeyPhysC,
    kEmexKeyPhysV,
    kEmexKeyPhysB,
    kEmexKeyPhysN,
    kEmexKeyPhysM,
    kEmexKeyPhysComma,
    kEmexKeyPhysPeriod,
    kEmexKeyPhysSlash,
    kEmexKeyPhysRightShift,
    kEmexKeyPhysLeftCtrl,
    kEmexKeyPhysLeftGUI,
    kEmexKeyPhysLeftAlt,
    kEmexKeyPhysSpace,
    kEmexKeyPhysRightAlt,
    kEmexKeyPhysRightGUI,
    kEmexKeyPhysMenu,
    kEmexKeyPhysRightCtrl,
    kEmexKeyPhysInsert,
    kEmexKeyPhysDelete,
    kEmexKeyPhysHome,
    kEmexKeyPhysEnd,
    kEmexKeyPhysPageUp,
    kEmexKeyPhysPageDown,
    kEmexKeyPhysArrowUp,
    kEmexKeyPhysArrowLeft,
    kEmexKeyPhysArrowDown,
    kEmexKeyPhysArrowRight,
    kEmexKeyPhysNumLock,
    kEmexKeyPhysNumpadDivide,
    kEmexKeyPhysNumpadMultiply,
    kEmexKeyPhysNumpadMinus,
    kEmexKeyPhysNumpadPlus,
    kEmexKeyPhysNumpadEnter,
    kEmexKeyPhysNumpad1,
    kEmexKeyPhysNumpad2,
    kEmexKeyPhysNumpad3,
    kEmexKeyPhysNumpad4,
    kEmexKeyPhysNumpad5,
    kEmexKeyPhysNumpad6,
    kEmexKeyPhysNumpad7,
    kEmexKeyPhysNumpad8,
    kEmexKeyPhysNumpad9,
    kEmexKeyPhysNumpad0,
    kEmexKeyPhysNumpadDot,
    kEmexKeyPhysPrintScreen,
    kEmexKeyPhysScrollLock,
    kEmexKeyPhysPause,
    kEmexKeyPhysUnknown
} kEmexKeyPhys;

#endif /* EMEX64_KEYBOARD_H */
