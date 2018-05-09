#include "emotiontable.h"

#include <cstring>
#include <cstdlib>

EmotionTable::EmotionTable()
    : emotion_count_(0)
{
}

void EmotionTable::init(void *file_context,
                        CsvTable *table)
{
    if (!table->open(file_context)) {
        return;
    }

    memset(emotions_, 0, sizeof(emotions_));
    emotion_count_ = 0;

    while (table->parse_line()) {
        size_t id;
        const char *name;
        Emotion::Type type;
        int8_t priority;

        const char *id_field = table->field("eID");
        if (!id_field) {
            continue;
        }

        int id_value = atoi(id_field);
        if (id_value < 0) {
            continue;
        }

        id = static_cast<size_t>(id_value);
        if (id >= MAX_EMOTION_COUNT) {
            continue;
        }

        name = table->field("eName");
        if (!name) {
            continue;
        }
        if (name[0] == '\0') {
            continue;
        }

        const char *type_field = table->field("eType");
        if (!type_field) {
            continue;
        }

        int type_value = atoi(type_field);
        switch (type_value) {
        case static_cast<int>(Emotion::Type::Single):
            type = Emotion::Type::Single;
            break;
        case static_cast<int>(Emotion::Type::Continuous):
            type = Emotion::Type::Continuous;
            break;
        default:
            continue;
        }

        const char *priority_field = table->field("ePriority");
        if (!priority_field) {
            continue;
        }

        int priority_value = atoi(priority_field);
        if (priority_value < INT8_MIN) {
            continue;
        }
        if (priority_value > INT8_MAX) {
            continue;
        }

        priority = static_cast<int8_t>(priority_value);

        emotions_[id].valid = true;
        emotions_[id].type = type;
        emotions_[id].priority = priority;
        memset(emotions_[id].name, 0, MAX_EMOTION_NAME_LENGTH + 1);
        strncpy(emotions_[id].name, name, MAX_EMOTION_NAME_LENGTH);
        if (id > emotion_count_) {
            emotion_count_ = id + 1;
        }
    }

    table->close();
}

uint8_t EmotionTable::find(const char *name) const
{
    for (uint8_t id = 0; id < emotion_count_; id++) {
        if (!emotions_[id].valid) {
            continue;
        }

        if (strcmp(name, emotions_[id].name) == 0) {
            return id;
        }
    }

    return INVALID_EMOTION;
}
