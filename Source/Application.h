//
// Core: Application.h
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

#include "Config.h"
#include <argagg.hpp>

class NWCOREAPI Application {
public:
    virtual void run();
    static argagg::parser_results& args();
protected:
    Application();
    virtual ~Application();
};

struct NWCOREAPI CmdOption {
    CmdOption(argagg::definition def);
    argagg::definition def;
};

#define DECL_CMD_OPTION(x) namespace { CmdOption opt { std::move(x) }; }
#define DECL_APPLICATION(x) namespace { x appInstance {}; }
