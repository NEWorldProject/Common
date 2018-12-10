#pragma once
#include <mutex>
#include <vector>
#include <type_traits>

template <class T>
struct LastValue {
    using TargetType = T;

    void operator()(T& out, const T& last) noexcept { out = last; }
};

template <>
struct LastValue<void> {
    using TargetType = void;
};

template <class T>
struct Ignore {
    using TargetType = void;
};

namespace __Details {
    class DelegateBase {
    protected:
        struct A {
            virtual ~A() noexcept = default;
            std::shared_ptr<A> _Retain = nullptr;
        };

        template <class T>
        auto Add(std::shared_ptr<T> closure) {
            {
                std::lock_guard<std::mutex> lk(_Lock);
                _List.emplace_back(closure);
                ++_Count;
            }
            return Connection(std::move(closure));
        }

        auto ListValidsAndCompress() const {
            std::lock_guard<std::mutex> lk(_Lock);
            std::vector<std::shared_ptr<void>> ret {};
            ret.reserve(_Count);
            if (_Count < (_List.size() * 3 / 4)) {
                auto iter = _List.begin();
                auto write = iter;
                for(; iter != _List.end(); ++iter)
                    if (auto v = iter->lock(); v) {
                        ret.push_back(std::move(v));
                        if (iter != write)
                            *write = std::move(*iter);
                        ++write;
                    }
                if (iter != write)
                    _List.erase(write, iter);
            }
            else {
                for (const auto& x : _List)
                    if (auto v = x.lock(); v)
                        ret.push_back(std::move(v));
            }
            return ret;
        }

        template <class T>
        static const auto& As(const std::shared_ptr<const void>& cls) noexcept {
            return *reinterpret_cast<const T*>(cls.get());
        }
    private:
        uint64_t _Count = 0;
        mutable std::mutex _Lock;
        mutable std::vector<std::weak_ptr<void>> _List;
    public:
        auto Size() const noexcept { return _Count; }

        bool Empty() const noexcept { return !static_cast<bool>(_Count); }

        class Connection {
        public:
            constexpr Connection() noexcept = default;

            bool Connected() const noexcept { return !_H.expired(); }

            void Disconnect() noexcept { if (auto p = _H.lock(); p) p->_Retain.reset(); }
        private:
            friend class DelegateBase;
            explicit Connection(std::shared_ptr<A> ptr) noexcept
                    :_H(ptr) { ptr->_Retain = std::move(ptr); }
            std::weak_ptr<A> _H;
        };
    };
}

using Connection = __Details::DelegateBase::Connection;

class ScopedConnection {
public:
    ScopedConnection() = default;

    explicit ScopedConnection(Connection conn) noexcept : _Conn(std::move(conn)) {}

    ScopedConnection(const ScopedConnection&&) = delete;

    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& r) noexcept : _Conn(std::move(r._Conn)) { }

    ScopedConnection& operator=(ScopedConnection&& r) noexcept {
        if (this != std::addressof(r)) {
            _Conn = std::move(r._Conn);
        }
        return *this;
    }

    ~ScopedConnection() noexcept { Disconnect(); }

    bool Connected() const noexcept { return _Conn.Connected(); }

    void Disconnect() noexcept { _Conn.Disconnect(); }
private:
    Connection _Conn;
};

template <class T, template <class U> class Reduce = LastValue>
class Delegate;

template <class T, template<class U> class Reduce, class ...Args>
class Delegate<T(Args...), Reduce> : __Details::DelegateBase {
    struct B : A { virtual T Call(Args&& ... arg) const = 0; };

    template <class Func>
    struct P : B {
        explicit P(Func&& fn)
                :_Fc(std::move(fn)) { }
        T Call(Args&& ... arg) const override { return _Fc(std::forward<Args>(arg)...); }
        Func _Fc;
    };
public:
    template <class Func>
    Connection Connect(Func&& fn) {
        return Add<B>(std::make_shared<P<std::decay_t<Func>>>(std::forward<Func>(fn)));
    }

    auto operator()(Args... arg) const {
        auto locked = ListValidsAndCompress();
        if constexpr(std::is_same_v<typename Reduce<T>::TargetType, void>) {
            for (const auto& x : locked)
                As<B>(x).Call(std::forward<Args>(arg)...);
        }
        else {
            typename Reduce<T>::TargetType ret{};
            Reduce<T> reduce;
            for (const auto& x : locked)
                reduce(ret, As<B>(x).Call(std::forward<Args>(arg)...));
            return ret;
        }
    }
};

template <class Sender>
class GenericSignal : __Details::DelegateBase {
    template <class Message>
    struct B : A { virtual void Call(Sender& sender, const Message& message) const = 0; };

    template <class Message, class Func>
    struct P : B<Message> {
        explicit P(Func&& fn)
                :_Fc(std::move(fn)) { }
        void Call(Sender& sender, const Message& message) const override { _Fc(sender, message); }
        Func _Fc;
    };
public:
    template <class Message, class Func>
    Connection ConnectUnsafe(Func&& fn) {
        return Add<B<Message>>(std::make_shared<P<Message, std::decay_t<Func>>>(std::forward<Func>(fn)));
    }

    template <class Message>
    void CastUnsafe(Sender& sender, const Message& message) const {
        auto locked = ListValidsAndCompress();
        for (const auto& x : locked)
            As<B<Message>>(x).Call(sender, message);
    }
};

template <class Sender, class Message>
class Signal : public GenericSignal<Sender> {
public:
    template <class Func>
    Connection Connect(Func&& fn) {
        return GenericSignal<Sender>::template ConnectUnsafe<Message, Func>(std::forward<Func>(fn));
    }

    void operator()(Sender& sender, const Message& message) const {
        GenericSignal<Sender>::template CastUnsafe(sender, message);
    }
};