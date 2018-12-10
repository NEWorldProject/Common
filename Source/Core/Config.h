// 
// Core: Config.h
// NEWorld: A Free Game with Similar Rules to Minecraft.
// Copyright (C) 2015-2018 NEWorld Team
// 
// NEWorld is free software: you can redistribute it and/or modify it 
// under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or 
// (at your option) any later version.
// 
// NEWorld is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General 
// Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with NEWorld.  If not, see <http://www.gnu.org/licenses/>.
// 

#pragma once

#include <boost/predef/compiler.h>
#include <boost/predef/platform.h>

// Compiler flags
#if BOOST_COMP_MSVC
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef _DEBUG
#define NEWORLD_DEBUG // Main debug flag
#endif

// NWAPICALL
#if BOOST_COMP_MSVC
#define NWAPICALL __cdecl
#elif defined(__i386__) || defined(__i386)
    #define NWAPICALL __attribute__((__cdecl__))
#else
    #undef NWAPICALL
    #define NWAPICALL
#endif

// NWAPIEXPORT
#if (BOOST_OS_CYGWIN || BOOST_OS_WINDOWS)
#if  BOOST_COMP_MSVC
#define NWAPIENTRY __declspec(dllimport)
#define NWAPIEXPORT __declspec(dllexport)
#else
#define NWAPIENTRY __attribute__((dllimport))
#define NWAPIEXPORT __attribute__((dllexport))
#endif
#else
#define NWAPIENTRY __attribute__((visibility("default")))
#define NWAPIEXPORT __attribute__((visibility("default")))
#endif
#ifdef NW_CORE_EXPORTS
#define NWCOREAPI NWAPIEXPORT
#else
#define NWCOREAPI NWAPIENTRY
#endif

#define _DO_JOIN( symbol1, symbol2 ) symbol1##symbol2
#define JOIN( symbol1, symbol2 ) _DO_JOIN( symbol1, symbol2 )
#define LINE_NAME( prefix ) JOIN( prefix, __LINE__ )

#define BGEIN_STATIC_INIT_RUN() namespace { struct LINE_NAME(NWSIR) { LINE_NAME(NWSAR) noexcept () {

#define END_STATIC_INIT_RUN }} LINE_NAME(NWSARE); }