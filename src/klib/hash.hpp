#include "types.hpp"
#include "string.hpp"

namespace klib {
    template<typename K> u32 hash(K &k);
    template<> inline u32 hash(String &str) { // djb2a
        u32 hash = 5381;

        for (const char *c = str.c_str(); *c; c++)
            hash = ((hash << 5) + hash) ^ *c; // hash * 33 ^ str[i]

        return hash;
    }
};
