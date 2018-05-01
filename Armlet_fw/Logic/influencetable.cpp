#include "influencetable.h"

#include <cstring>
#include <cstdlib>

InfluenceTable::InfluenceTable()
    : influence_count_(0)
{
}

enum class Strength
{
    None = 0,
    Near = 1,
    Medium = 2,
    Far = 3
};

static const int8_t STRENGTH_LEVELS[] = {127, 0, -64, -96};

void InfluenceTable::init(void *file_context,
                          CsvTable *table,
                          const EmotionTable *emotion_table)
{
    if (!table->open(file_context)) {
        return;
    }

    memset(influences_, 0, sizeof(influences_));
    influence_count_ = 0;

    while (table->parse_line()) {
        size_t id;
        const char *name;
        int8_t strength;
        int16_t timeout;
        int8_t dogan_effect;
        uint8_t emotion = EmotionTable::INVALID_EMOTION;

        const char *id_field = table->field("rID");
        if (!id_field) {
            continue;
        }

        int id_value = atoi(id_field);
        if (id_value < 0) {
            continue;
        }

        id = static_cast<size_t>(id_value);
        if (id >= MAX_INFLUENCE_COUNT) {
            continue;
        }

        name = table->field("rName");
        if (!name) {
            continue;
        }
        if (name[0] == '\0') {
            continue;
        }

        const char *strength_field = table->field("level");
        if (!strength_field) {
            continue;
        }

        int strength_value = atoi(strength_field);
        switch (strength_value) {
        case static_cast<int>(Strength::None):
        case static_cast<int>(Strength::Near):
        case static_cast<int>(Strength::Medium):
        case static_cast<int>(Strength::Far):
            strength = STRENGTH_LEVELS[strength_value];
            break;
        default:
            continue;
        }

        const char *timeout_field = table->field("timeout");
        if (!timeout_field) {
            continue;
        }

        int timeout_value = atoi(timeout_field);
        if (timeout_value < -1) {
            continue;
        }
        if (timeout_value > INT16_MAX) {
            continue;
        }

        timeout = static_cast<int16_t>(timeout_value);

        const char *dogan_effect_field = table->field("doganAmount");
        if (!dogan_effect_field) {
            continue;
        }

        int dogan_effect_value = atoi(dogan_effect_field);
        if (dogan_effect_value < INT8_MIN) {
            continue;
        }
        if (dogan_effect_value > INT8_MAX) {
            continue;
        }

        dogan_effect = static_cast<int8_t>(dogan_effect_value);

        const char *emotion_name = table->field("eName");
        if (emotion_name) {
            if (emotion_name[0] != '\0') {
                emotion = emotion_table->find(emotion_name);
            }
        }

        influences_[id].valid = true;
        influences_[id].strength = strength;
        influences_[id].timeout = timeout;
        influences_[id].dogan_effect = dogan_effect;
        influences_[id].emotion = emotion;
        memset(influences_[id].name, 0, MAX_INFLUENCE_NAME_LENGTH + 1);
        strncpy(influences_[id].name, name, MAX_INFLUENCE_NAME_LENGTH);
        if (id > influence_count_) {
            influence_count_ = id + 1;
        }
    }

    table->close();
}

uint8_t InfluenceTable::find(const char *name) const
{
    for (uint8_t id = 0; id < influence_count_; id++) {
        if (!influences_[id].valid) {
            continue;
        }

        if (strcmp(name, influences_[id].name) == 0) {
            return id;
        }
    }

    return INVALID_INFLUENCE;
}
