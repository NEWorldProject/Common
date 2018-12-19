//
// Core: Intrusive.h
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

#include <new>
#include <atomic>
#include <limits>
#include <type_traits>

template <class IntrusiveType> class IntrusivePtr;

class IntrusiveVTBase {
    template <class T>
    friend class IntrusivePtr;
    template <class T>
    friend class WeakIntrusivePtr;
    template <class U, class ...Args>
    friend IntrusivePtr<U> MakeIntrusive(Args&&... args);

    // Get Control Counts
    uint32_t Count() const noexcept { return static_cast<uint32_t>(_Ctrl.load() >> 32); }

    // Acquire Control, increases strong count
    void Acquire() noexcept { _Ctrl.fetch_add(uint64_t(1) << 32); }

    // Acquire Reference, increases weak count
    void Reference() noexcept { _Ctrl.fetch_add(1); }

    // Try to lock up a Control Reference
    bool Lock() const noexcept {
        for(;;) {
            uint64_t val = _Ctrl.load();
            if (val >> 32) {
                if (_Ctrl.compare_exchange_strong(val, val + (uint64_t(1) << 32)))
                    return true;
            }
            else {
                return false;
            }
        }
    }

    // Release Control, decrease strong count and check if the object is good to release
    void TryRelease() noexcept {
        auto last = _Ctrl.fetch_sub(uint64_t(1) << 32);
        if (last >> 32 == 0)
            this->~IntrusiveVTBase();
        if (last == uint64_t(1) << 32)
            SelfDealloc();
    }
    // Release Reference, decrease weak count and check if the object is good to release
    void TryDereference() noexcept { if( _Ctrl.fetch_sub(1)==1) SelfDealloc(); }

    // Strong References uses the top 32 bits and weak on the lower 32 bits
    // In this case, we cannot manage more than 2^32 - 1 references on sync, but trust me that is absolutely more than
    // enough on most cases
    mutable std::atomic_uint64_t _Ctrl;

    // We need to know this to do actual deallocation
    // Yes this IS RTTI info, but we cannot do dynamic reflection with current standard
    int32_t _BaseOffset, _BaseAlign;

    void SelfDealloc() noexcept {
        operator delete(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + _BaseOffset),
                static_cast<std::align_val_t>(_BaseAlign));
    }
public:
    virtual ~IntrusiveVTBase() noexcept = default;
};

template <class IntrusiveType>
class IntrusivePtr {
public:
    using ElementType = IntrusiveType;

    constexpr IntrusivePtr() noexcept = default;

    IntrusivePtr(const IntrusivePtr& r) noexcept
            :_Ctrl(r._Ctrl), _Val(r._Val) { if (_Ctrl) _Ctrl->Acquire(); }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    explicit IntrusivePtr(const IntrusivePtr<Other>& r) noexcept
            : _Ctrl(r._Ctrl), _Val(r._Val) { if (_Ctrl) _Ctrl->Acquire(); }

    IntrusivePtr(IntrusivePtr&& r) noexcept
            :_Ctrl(r._Ctrl), _Val(r._Val) { r._Ctrl = nullptr; }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    explicit IntrusivePtr(IntrusivePtr<Other>&& r) noexcept
            : _Ctrl(r._Ctrl), _Val(r._Val) { r._Ctrl = nullptr; }

    IntrusivePtr& operator=(const IntrusivePtr& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            if (_Ctrl) _Ctrl->Acquire();
        }
        return *this;
    }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    IntrusivePtr& operator=(const IntrusivePtr<Other>& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            if (_Ctrl) _Ctrl->Acquire();
        }
        return *this;
    }

    IntrusivePtr& operator=(IntrusivePtr&& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            r._Ctrl = nullptr;
        }
        return *this;
    }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    IntrusivePtr& operator=(IntrusivePtr<Other>&& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            r._Ctrl = nullptr;
        }
        return *this;
    }

    ~IntrusivePtr() noexcept { if (_Ctrl) _Ctrl->TryRelease(); }

    explicit operator bool() const noexcept { return _Ctrl; }

    ElementType* Get() const noexcept { return _Val; }

    auto UseCount() const noexcept { return _Ctrl->Count(); }

    ElementType& operator*() const noexcept { return *_Val; }

    ElementType* operator->() const noexcept { return _Val; }
private:
    template <class U, class ...Args>
    friend IntrusivePtr<U> MakeIntrusive(Args&&... args);

    template <class T>
    friend class WeakIntrusivePointer;

    template <class = std::enable_if_t<std::is_convertible_v<IntrusiveType*, IntrusiveVTBase*>>>
    explicit IntrusivePtr(IntrusiveType* unmanaged) noexcept
            :_Ctrl(unmanaged), _Val(unmanaged) { _Ctrl->Acquire(); }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    explicit IntrusivePtr(Other* unmanaged) noexcept
            : _Ctrl(unmanaged), _Val(unmanaged) { _Ctrl->Acquire(); }

    IntrusiveVTBase* _Ctrl = nullptr;
    ElementType* _Val = nullptr;
};

template <class U, class... Args>
IntrusivePtr<U> MakeIntrusive(Args&&... args) {
    struct R {
        R() : Base(operator new(sizeof(U), std::align_val_t(alignof(U)))) {}
        void* Base = nullptr;
        ~R() noexcept {if (Base) operator delete(Base, std::align_val_t(alignof(U)), std::nothrow); }
    } _Base;
    U* _Ptr = new (_Base.Base) U(std::forward<Args>(args)...);
    IntrusiveVTBase* _VTBase = _Ptr;
    _VTBase->_BaseAlign = alignof(U);
    _VTBase->_BaseOffset = static_cast<int32_t>(
            reinterpret_cast<uintptr_t>(_Base.Base) - reinterpret_cast<uintptr_t>(_VTBase));
    _Base.Base = nullptr;
    return IntrusivePtr<U>(_Ptr);
}

template <class IntrusiveType>
class WeakIntrusivePtr {
public:
    using ElementType = IntrusiveType;

    constexpr WeakIntrusivePtr() noexcept = default;

    WeakIntrusivePtr(const WeakIntrusivePtr& r) noexcept
            :_Ctrl(r._Ctrl), _Val(r._Val) { if (_Ctrl) _Ctrl->Reference(); }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    explicit WeakIntrusivePtr(const WeakIntrusivePtr<Other>& r) noexcept
            : _Ctrl(r._Ctrl), _Val(r._Val) { if (_Ctrl) _Ctrl->Reference(); }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    explicit WeakIntrusivePtr(const IntrusivePtr<Other>& r) noexcept
            : _Ctrl(r._Ctrl), _Val(r._Val) { if (_Ctrl) _Ctrl->Reference(); }

    WeakIntrusivePtr(WeakIntrusivePtr&& r) noexcept
    :_Ctrl(r._Ctrl), _Val(r._Val) { r._Ctrl = nullptr; }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    explicit WeakIntrusivePtr(WeakIntrusivePtr<Other>&& r) noexcept
    : _Ctrl(r._Ctrl), _Val(r._Val) { r._Ctrl = nullptr; }

    WeakIntrusivePtr& operator=(const WeakIntrusivePtr& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            if (_Ctrl) _Ctrl->Reference();
        }
        return *this;
    }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    WeakIntrusivePtr& operator=(const WeakIntrusivePtr<Other>& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            if (_Ctrl) _Ctrl->Reference();
        }
        return *this;
    }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    WeakIntrusivePtr& operator=(const IntrusivePtr<Other>& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            if (_Ctrl) _Ctrl->Reference();
        }
        return *this;
    }

    WeakIntrusivePtr& operator=(WeakIntrusivePtr&& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            r._Ctrl = nullptr;
        }
        return *this;
    }

    template <class Other, class = std::enable_if_t<std::is_convertible_v<Other*, IntrusiveType*>>>
    WeakIntrusivePtr& operator=(WeakIntrusivePtr<Other>&& r) noexcept {
        if (this != std::addressof(r)) {
            _Ctrl = r._Ctrl;
            _Val = r._Val;
            r._Ctrl = nullptr;
        }
        return *this;
    }

    ~WeakIntrusivePtr() noexcept { if (_Ctrl) _Ctrl->TryDereference(); }

    auto UseCount() const noexcept { return _Ctrl->Count(); }

    bool Expired() const noexcept { return !UseCount(); }

    IntrusivePtr<IntrusiveType> Lock() const noexcept {
        IntrusivePtr<IntrusiveType> ret {};
        if (bool res = _Ctrl->Lock(); res) {
            ret._Ctrl = _Ctrl;
            ret._Val = _Val;
        }
        return ret;
    }
private:
    IntrusiveVTBase* _Ctrl;
    ElementType* _Val;
};

template <class T, class U>
bool operator < (const IntrusivePtr<T>& l, const IntrusivePtr<U>& r) noexcept { return l.Get() < r.Get(); }

template <class T, class U>
bool operator > (const IntrusivePtr<T>& l, const IntrusivePtr<U>& r) noexcept { return l.Get() > r.Get(); }

template <class T, class U>
bool operator <= (const IntrusivePtr<T>& l, const IntrusivePtr<U>& r) noexcept { return l.Get() <= r.Get(); }

template <class T, class U>
bool operator >= (const IntrusivePtr<T>& l, const IntrusivePtr<U>& r) noexcept { return l.Get() >= r.Get(); }

template <class T, class U>
bool operator == (const IntrusivePtr<T>& l, const IntrusivePtr<U>& r) noexcept { return l.Get() == r.Get(); }

template <class T, class U>
bool operator != (const IntrusivePtr<T>& l, const IntrusivePtr<U>& r) noexcept { return l.Get() != r.Get(); }
