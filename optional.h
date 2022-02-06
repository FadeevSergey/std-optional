#pragma once

#include <utility>

class nullopt_t {
    constexpr nullopt_t() = default;
};

inline constexpr nullopt_t nullopt{};

class in_place_t {
    constexpr in_place_t() = default;
};

inline constexpr in_place_t in_place{};

template <class T, bool = std::is_trivially_destructible_v<T>>
class base_destructor {
public:
    constexpr base_destructor() noexcept
            : is_empty(true)
            , nothing() {
    }

    constexpr base_destructor(nullopt_t) noexcept
            : is_empty(true)
            , nothing() {
    }

    template <typename... Args>
    constexpr explicit base_destructor(in_place_t, Args&&... args)
            : is_empty(false)
            , value(std::forward<Args>(args)...) {
    }

    ~base_destructor() {
        if (!is_empty) {
            value.~T();
        }
    }

protected:
    bool is_empty;

    union {
        char* nothing;
        T value;
    };
};

template <class T>
class base_destructor<T, true> {
public:
    constexpr base_destructor() noexcept
            : is_empty(true)
            , nothing() {
    }

    constexpr base_destructor(nullopt_t) noexcept
            : is_empty(true)
            , nothing() {
    }

    template <typename... Args>
    constexpr explicit base_destructor(in_place_t, Args&&... args)
            : is_empty(false)
            , value(std::forward<Args>(args)...) {
    }

protected:
    bool is_empty;

    union {
        char* nothing;
        T value;
    };
};

template <class T, bool = std::is_trivially_copyable_v<T>>
class base_copy : public base_destructor<T> {
public:
    constexpr base_copy(const base_copy& copy_class) {
        if(!copy_class.is_empty) {
            this->is_empty = copy_class.is_empty;
            new (&this->value) T(copy_class.value);
        }
    }

    constexpr base_copy(base_copy&& copy_class) noexcept(std::is_nothrow_move_constructible<T>::value) {
        if(!copy_class.is_empty) {
            this->is_empty = copy_class.is_empty;
            new (&this->value) T(std::move(copy_class.value));
        }
    }

    base_copy& operator=(const base_copy& copy_class) {
        if (this->is_empty) {
            if (!copy_class.is_empty) {
                new (&(this->value)) T(copy_class.value);
            }
        } else {
            if (copy_class.is_empty) {
                this->value.~T();
            } else {
                this->value = copy_class.value;
            }
        }
        this->is_empty = copy_class.is_empty;
        return *this;
    }

    base_copy& operator=(base_copy&& copy_class) noexcept(std::is_nothrow_move_constructible<T>::value &&
                                                          std::is_nothrow_move_assignable<T>::value) {
        if (this->is_empty) {
            if (!copy_class.is_empty) {
                new (&(this->value)) T(std::move(copy_class.value));
            }
        } else {
            if (copy_class.is_empty) {
                this->value.~T();
            } else {
                this->value = std::move(copy_class.value);
            }
        }
        this->is_empty = copy_class.is_empty;
        return *this;
    }
protected:
    using base = base_destructor<T>;
    using base::base;
};

template <class T>
class base_copy<T, true> : public base_destructor<T> {
public:
    constexpr base_copy(const base_copy& copy_class) = default;
    constexpr base_copy(base_copy&& copy_class) = default;

    base_copy& operator=(const base_copy& copy_class) = default;
    base_copy& operator=(base_copy&& copy_class) = default;
protected:
    using base = base_destructor<T>;
    using base::base;
};

template <typename T>
class optional : base_copy<T> {
public:
    constexpr optional(optional const&) = default;
    constexpr optional(optional&&) = default;

    optional& operator=(optional const&) = default;
    optional& operator=(optional&&) = default;

    constexpr optional(T value)
        : base(in_place, std::move(value)) {
    }

    optional& operator=(nullopt_t) noexcept {
        reset();
        return *this;
    }

    constexpr explicit operator bool() const noexcept {
        return !this->is_empty;
    }

    constexpr T& operator*() noexcept {
        return this->value;
    }
    constexpr T const& operator*() const noexcept {
        return this->value;
    }

    constexpr T* operator->() noexcept {
        return &this->value;
    }
    constexpr T const* operator->() const noexcept {
        return &this->value;
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        reset();
        new (&this->value) T(std::forward<Args>(args)...);
        this->is_empty = false;
    }

    void reset() {
        if (!this->is_empty) {
            this->value.~T();
        }
        this->is_empty = true;
    }
private:
    using base = base_copy<T>;
    using base::base;
};

template<typename T>
constexpr bool operator==(optional<T> const &a, optional<T> const &b) {
    if (bool(a) != bool(b)) {
        return false;
    } else if (bool(a) == bool(b) && !bool(a)) {
        return true;
    } else {
        return *a == *b;
    }
}

template<typename T>
constexpr bool operator!=(optional<T> const &a, optional<T> const &b) {
    return !(a == b);
}

template<typename T>
constexpr bool operator<(optional<T> const &a, optional<T> const &b) {
    if (!bool(b)) {
        return false;
    } else if (!bool(a)) {
        return true;
    } else {
        return *a < *b;
    }
}

template<typename T>
constexpr bool operator<=(optional<T> const &a, optional<T> const &b) {
    return a < b || a == b;
}

template<typename T>
constexpr bool operator>(optional<T> const &a, optional<T> const &b) {
    return !(a <= b);
}

template<typename T>
constexpr bool operator>=(optional<T> const &a, optional<T> const &b) {
    return !(a < b);
}
