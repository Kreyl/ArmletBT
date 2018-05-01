#pragma once

#include <cstddef>
#include <cstdint>

#include "csvtable.h"

class EmotionTable
{
public:
    static const size_t MAX_EMOTION_COUNT = 63;
    static const uint8_t INVALID_EMOTION = 255;

    static const size_t MAX_EMOTION_NAME_LENGTH = 31;

    struct Emotion
    {
        enum class Type : uint8_t
        {
            Single = 0,
            Continuous = 1
        };

        bool valid;
        Type type;
        uint8_t priority;
        char name[MAX_EMOTION_NAME_LENGTH + 1];
    };

public:
    EmotionTable();

    void init(void *file_context,
              CsvTable *table);

    size_t emotion_count() const
    {
        return emotion_count_;
    }

    const Emotion *emotion(size_t id) const
    {
        if (id >= emotion_count_) {
            return nullptr;
        }

        if (!emotions_[id].valid) {
            return nullptr;
        }

        return &emotions_[id];
    }

    const Emotion *emotion(const char *name) const
    {
        return emotion(find(name));
    }

    uint8_t find(const char *name) const;

private:
    Emotion emotions_[MAX_EMOTION_COUNT];
    size_t emotion_count_;
};
