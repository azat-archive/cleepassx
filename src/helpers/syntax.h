#pragma once

namespace helpers {
namespace syntax {
    template <typename T, const T default_value = 0>
    struct zero_inited {
        zero_inited(const T& _ = default_value) : value(_) {}
        operator T& () { return value; }
        operator const T&() const { return value; }
        T value;
    };
    template <typename T>
    struct auto_free {
        auto_free(T* _) : data(_){}
        ~auto_free() { free(data); }
        operator const T*() const { return data; }
        operator T*() { return data; }
        T operator[](size_t _) const { return data[_]; }
        T& operator[](size_t _) { return data[_]; }
        T* data;
    };
    template <typename T>
    auto_free<T> mk_auto_free(T* _) { return auto_free<T>(_); }
    typedef zero_inited<unsigned> zuint;
    typedef zero_inited<int> zint;
    typedef zero_inited<bool> zbool;
    template <typename T, typename conversible_to_T = T>
    struct hide_value {
        hide_value(T& _, const conversible_to_T& _new) 
        : _Target(_), _Old_value(_), _Relax(false) { _Target = _new; }
        const T& Release() { _Relax = true; return _Old_value; }
        void Reset() { _Target = _Old_value; }
        ~hide_value() { if (not _Relax) Reset(); }
      private:
        bool _Relax;
        T& _Target;
        const T _Old_value;
    };

    // I'm sorry, but I've read Alexandrescu and cannot help using it...
    // This type can be used to overload functions depending on on their return type or to partially instantiate template functions via fake overloading
    template <typename T> struct Type{};
}
}
