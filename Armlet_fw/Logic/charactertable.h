#pragma once

#include <cstddef>
#include <cstdint>

#include "csvtable.h"

struct LocalCharacter;

class CharacterTable
{
public:
    static const size_t MAX_CHARACTER_COUNT = 127;
    static const uint8_t FIRST_CHARACTER = 128;
    static const uint8_t INVALID_CHARACTER = 255;

    static const size_t MAX_CHARACTER_NAME_LENGTH = 31;
    static const size_t MAX_KA_TET_LIST_LENGTH = 127;

    struct Character
    {
        bool valid;
        char name[MAX_CHARACTER_NAME_LENGTH + 1];
    };

public:
    CharacterTable();

    void init(void *file_context,
              CsvTable *table,
              const char *local_character_name,
              LocalCharacter *local_character);

    size_t character_count() const
    {
        return character_count_;
    }

    const Character *character(size_t id) const
    {
        if (id >= character_count_) {
            return nullptr;
        }

        if (!characters_[id].valid) {
            return nullptr;
        }

        return &characters_[id];
    }

    const Character *character(const char *name) const
    {
        return character(find(name));
    }

    uint8_t find(const char *name) const;

private:
    Character characters_[MAX_CHARACTER_COUNT];
    size_t character_count_;
};
