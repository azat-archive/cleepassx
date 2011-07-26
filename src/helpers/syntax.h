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
    typedef zero_inited<unsigned> zuint;
    typedef zero_inited<int> zint;
    typedef zero_inited<bool> zbool;
}
}
