#pragma once

#include <kstd/types.hpp>
#include <panic.hpp>

namespace kstd {
    template<class>
    class Function;

    template<class R, class ...Args>
    class Function<R(Args...)> {
        struct Callable {
            virtual ~Callable() = default;
            virtual R invoke(Args...) = 0;
        };

        template<class F>
        class CallableWrapper : public Callable {
            F f;
        public:
            CallableWrapper(const F &f) : f(kstd::move(f)) {}

            R invoke(Args ...args) override {
                return f(args...);
            }
        };

        Callable *callable;
    public:
        Function() = default;

        template<class F>
        Function(F f) : callable(new CallableWrapper<F>(f)) {}

        ~Function() {
            if (callable) delete callable;
        }

        template<class F>
        Function& operator =(F f) {
            if (this->callable) delete this->callable;
            this->callable = new CallableWrapper<F>(kstd::move(f));
            return *this;
        }

        Function& operator =(Function &f) {
            kstd::swap(this->callable, f.callable);
            return *this;
        }

        Function& operator =(nullptr_t) {
            if (callable) delete callable;
            return *this;
        }

        R operator ()(Args ...args) const {
            ASSERT(callable);
            return callable->invoke(args...);
        }

        explicit operator bool() const { return !!callable; }
    };
}
