#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "charactertable.h"

static const int MIN_DOGAN = -2;
static const int MAX_DOGAN = 2;

class Flags
{
public:
    static const size_t SIZE = CharacterTable::MAX_CHARACTER_COUNT;

public:
    Flags()
        : data_{}
    {
    }

    bool get(size_t index) const
    {
        if (index >= SIZE) {
            return 0;
        }

        return (data_[index / 32] >> (index % 32)) & 1;
    }

    void set(size_t index, bool value)
    {
        if (index >= SIZE) {
            return;
        }

        uint32_t mask = 1 << (index % 32);

        if (value) {
            data_[index / 32] |= mask;
        } else {
            data_[index / 32] &= ~mask;
        }
    }

    bool empty() const
    {
        uint32_t accum = 0;

        for (size_t i = 0; i < sizeof(data_) / 4; i++) {
            accum |= data_[i];
        }

        return accum == 0;
    }

    size_t count_ones() const
    {
        size_t count = 0;

        for (size_t i = 0; i < sizeof(data_) / 4; i++) {
            count += static_cast<size_t>(__builtin_popcountl(data_[i]));
        }

        return count;
    }

    size_t find_first_one() const
    {
        for (size_t i = 0; i < sizeof(data_) / 4; i++) {
            int ffs = __builtin_ffsl(data_[i]);

            if (ffs != 0) {
                return i * 32 + static_cast<size_t>(ffs) - 1;
            }
        }

        return SIZE;
    }

    void clear()
    {
        memset(data_, 0, sizeof(data_));
    }

private:
    uint32_t data_[(SIZE + 31) / 32];
};

typedef Flags KaTetLinks;
typedef Flags NearCharacters;

class Counters
{
public:
    static const size_t SIZE = CharacterTable::MAX_CHARACTER_COUNT;

public:
    Counters()
        : counters_{}
    {
    }

    uint16_t &operator[](size_t index)
    {
        if (index >= SIZE) {
            return counters_[SIZE];
        }

        return counters_[index];
    }

    uint16_t operator[](size_t index) const
    {
        if (index >= SIZE) {
            return counters_[SIZE];
        }

        return counters_[index];
    }

private:
    uint16_t counters_[SIZE + 1];
};

typedef Counters KaTetCounters;

struct LocalCharacter
{
    size_t id;
    KaTetLinks ka_tet_links;
    KaTetCounters ka_tet_counters;
    NearCharacters near_characters;
    int dogan;
    bool corrupted;
    bool dead;
    bool manni;
};
