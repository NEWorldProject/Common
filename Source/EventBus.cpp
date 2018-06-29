// 
// Core: EventBus.cpp
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

#include "Core/EventBus.h"
#include "Core/Logger.h"

NWCOREAPI EventBus eventBus;

std::vector<EventBus::FunctionPointer> &EventBus::getSubscribers(const std::string &funcName, const std::type_info &typeId) {
    return mSubscribers[std::to_string(typeId.hash_code()) + "!" + funcName];
}

void EventBus::registerImpl(const std::string &funcName, FunctionPointer func, const std::type_info &typeId) {
    auto& list = getSubscribers(funcName, typeId);
    list.emplace_back(func);
    if (list.size() != 1)
        warningstream << "Multiple(" << list.size() << ") functions with name" << funcName << " and type " <<
                      typeId.name() << " (hash: " << typeId.hash_code() << ") registered.";
}

EventBus::FunctionPointer EventBus::callGet(const std::string &funcName, const std::type_info &typeId) {
    auto& list = getSubscribers(funcName, typeId);
    if (list.size() == 0) {
        warningstream << "Failed to call function " << funcName
                      << " with type " << typeId.name() << " (hash: " << typeId.hash_code() << "): "
                      << (list.empty()
                          ? "No such function registered"
                          : "Multiple(" + std::to_string(list.size()) + ") functions registered.");
        throw std::runtime_error(funcName + " with type " + typeId.name()
                                 + " (hash: " + std::to_string(typeId.hash_code()) + ") does not exist");
    }
    return list[0];
}
