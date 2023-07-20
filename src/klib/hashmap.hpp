#pragma once

#include <klib/cstdlib.hpp>

#define HASHMAP_DELETED_ENTRY (Entry*)(~(uptr)0)

namespace klib {
    inline u32 hash(const char *str) { // djb2a
        u32 hash = 5381;

        for (const char *c = str; *c; c++)
            hash = ((hash << 5) + hash) ^ *c; // hash * 33 ^ str[i]

        return hash;
    }

    template<typename V>
    class HashMap {
        struct Entry {
            const char *key;
            V value;
        };

        Entry **m_array;
        usize m_capacity, m_size;

    public:
        HashMap(usize initial_capacity = 16) : m_capacity(initial_capacity), m_size(0) {
            m_array = (Entry**)klib::calloc(sizeof(void*) * m_capacity);
        }

        ~HashMap() {
            for (usize i = 0; i < m_capacity; i++)
                if (m_array[i] && m_array[i] != HASHMAP_DELETED_ENTRY) delete m_array[i];
            klib::free(m_array);
        }

        // calculates the load factor but as a percentage because we cant use floating point
        usize load_percentage() {
            return (m_size * 100) / m_capacity;
        }

        void grow() {
            auto old_array = m_array;
            auto old_capacity = m_capacity;
            m_capacity *= 2;
            m_array = (Entry**)klib::calloc(sizeof(void*) * m_capacity);
            m_size = 0;
            for (usize i = 0; i < old_capacity; i++) {
                if (old_array[i] && old_array[i] != HASHMAP_DELETED_ENTRY) {
                    insert(old_array[i]->key, old_array[i]->value);
                    delete old_array[i];
                }
            }
            free(old_array);
        }

        void insert(const char *key, V value) {
            m_size++;
            if (load_percentage() >= 75) grow();
            auto first_index = hash(key) % m_capacity;
            for (usize i = 0; i < m_capacity; i++) {
                auto attempt = (i + first_index) % m_capacity;
                if (m_array[attempt] == HASHMAP_DELETED_ENTRY) continue;
                if (!m_array[attempt]) {
                    m_array[attempt] = new Entry(strdup(key), value);
                    break;
                }
            }
        }

        V* get(const char *key) {
            auto first_index = hash(key) % m_capacity;
            for (usize i = 0; i < m_capacity; i++) {
                auto attempt = (i + first_index) % m_capacity;
                if (!m_array[attempt] || m_array[attempt] == HASHMAP_DELETED_ENTRY) continue;
                if (strcmp(m_array[attempt]->key, key) == 0) {
                    return &m_array[attempt]->value;
                }
            }
            return nullptr;
        }
        
        void erase(const char *key) {
            auto first_index = hash(key) % m_capacity;
            for (usize i = 0; i < m_capacity; i++) {
                auto attempt = (i + first_index) % m_capacity;
                if (!m_array[attempt] || m_array[attempt] == HASHMAP_DELETED_ENTRY) continue;
                if (strcmp(m_array[attempt]->key, key) == 0) {
                    delete m_array[attempt];
                    m_array[attempt] = HASHMAP_DELETED_ENTRY;
                }
            }
        }
        
        template<typename F>
        void for_each(F func) {
            for (usize i = 0; i < m_capacity; i++)
                if (m_array[i] && m_array[i] != HASHMAP_DELETED_ENTRY)
                    func(&m_array[i]->key, &m_array[i]->value);
        }
    };
}
