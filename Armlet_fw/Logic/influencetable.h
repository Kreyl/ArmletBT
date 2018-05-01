#pragma once

#include <cstddef>
#include <cstdint>

#include "csvtable.h"
#include "emotiontable.h"

class InfluenceTable
{
public:
    static const size_t MAX_INFLUENCE_COUNT = 255;
    static const uint8_t INVALID_INFLUENCE = 255;

    static const size_t MAX_INFLUENCE_NAME_LENGTH = 31;

    struct Influence
    {
        bool valid;
        int8_t strength;
        int16_t timeout;
        int8_t dogan_effect;
        uint8_t emotion;
        char name[MAX_INFLUENCE_NAME_LENGTH + 1];
    };

public:
    InfluenceTable();

    void init(void *file_context,
              CsvTable *table,
              const EmotionTable *emotion_table);

    size_t influence_count() const
    {
        return influence_count_;
    }

    const Influence *influence(size_t id) const
    {
        if (id >= influence_count_) {
            return nullptr;
        }

        if (!influences_[id].valid) {
            return nullptr;
        }

        return &influences_[id];
    }

    const Influence *influence(const char *name) const
    {
        return influence(find(name));
    }

    uint8_t find(const char *name) const;

private:
    Influence influences_[MAX_INFLUENCE_COUNT];
    size_t influence_count_;
};
