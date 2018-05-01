#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "charactertable.h"

static const int MIN_DOGAN = -2;
static const int MAX_DOGAN = 2;

class KaTetLinks
{
public:
    static const size_t SIZE = CharacterTable::MAX_CHARACTER_COUNT;

public:
    KaTetLinks()
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

    void clear()
    {
        memset(data_, 0, sizeof(data_));
    }

private:
    uint32_t data_[(SIZE + 31) / 32];
};

class KaTetCounters
{
public:
    static const size_t SIZE = CharacterTable::MAX_CHARACTER_COUNT;

public:
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

struct LocalCharacter
{
    size_t id;
    KaTetLinks ka_tet_links;
    KaTetCounters ka_tet_counters;
    int dogan;
    bool corrupted;
    bool dead;
    bool manni;
};
