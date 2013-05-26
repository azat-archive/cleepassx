#pragma once
#include <iosfwd>

#include <iostream>

namespace helpers {
namespace syntax {
    /// in-class initializable object
    template <typename T, const T default_value = 0>
    struct zero_inited {
        zero_inited(const T& _ = default_value) : value(_) {}
        operator T& () { return value; }
        operator const T&() const { return value; }
        T value;
    };
    typedef zero_inited<unsigned> zuint;
    typedef zero_inited<int> zint;
    typedef zero_inited<bool> zbool;
    
    /// just like std::auto_ptr, but invokes free upon destruction
/**
 * @brief auto pointer for C arrays.
 * 
 * It behaves like std::auto_ptr (and std::unique_ptr in C++11),
 * but cleans its data using free ().
 **/
template <typename T>
struct auto_free {
    auto_free(T* _) : data(_){}
    ~auto_free() { cleanup(); }
    operator T*() const { return data; }
    const T& operator[](size_t _) const { return data[_]; }
    T& operator[](size_t _) { return data[_]; }

    void reset (T* _new_buffer = NULL) {
        if (data == _new_buffer)
            return;
        cleanup ();
        data = _new_buffer;
    }
    T* release () {
        T* ptr = data;
        data = NULL;
        return ptr;
    }
private:
    void cleanup() { free (data); }
    T* data;
};
    
    /// function to avoid explicit template parameters
    template <typename T>
    auto_free<T> mk_auto_free(T* _) { return auto_free<T>(_); }
    
    /// Push a variable's value, assign a new value and pop back the original one upon the object destruction.
    template <typename T, typename conversible_to_T = T>
    struct hide_value {
        hide_value(T& _, const conversible_to_T& _new) 
        : _Target(_), _Old_value(_), _Relax(false) { _Target = _new; }
        const T& Release() { _Relax = true; return _Old_value; }
        void Reset() { _Target = _Old_value; }
        ~hide_value() { if (not _Relax) Reset(); }
      private:
        T& _Target;
        const T _Old_value;
        bool _Relax;
    };

    // I'm sorry, but I've read Alexandrescu and cannot help using it...
    // This type can be used to overload functions depending on their return type or to partially instantiate template functions via fake overloading
    template <typename T> struct Type{};
}
namespace qt {
    /// transform «~/» at the beginning to the path to home dir
    inline QString expandTilde (QString s) {
        if (s.startsWith ("~/"))
            s.replace (0, 1, QDir::homePath());
        return s;
    }
    inline std::ostream& operator<< (std::ostream& _str, const QString& _qstr) {
        return _str << qPrintable(_qstr);
    }
}
}
