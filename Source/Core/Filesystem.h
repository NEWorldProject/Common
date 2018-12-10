//
// Core: Filesystem.h
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
#if __has_include(<filesystem>)
#include <filesystem>
namespace std::filesystem {
    using error_code = std::error_code;
}
namespace filesystem = std::filesystem;
#else
#include <boost/filesystem.hpp>
#define NW_FS_IS_BOOST
namespace boost::filesystem {
    inline void recursive_copy(const path& src, const path& dst) {
        if (exists(dst)) {
            throw std::runtime_error(dst.generic_string()+" exists");
        }

        if (is_directory(src)) {
            create_directories(dst);
            for (directory_entry& item : directory_iterator(src)) {
                recursive_copy(item.path(), dst/item.path().filename());
            }
        }
        else if (is_regular_file(src)) {
            copy(src, dst);
        }
        else {
            throw std::runtime_error(dst.generic_string()+" not dir or file");
        }
    }
    using error_code = system::error_code;
}
namespace filesystem = boost::filesystem;
#endif
