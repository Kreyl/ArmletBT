#include "charactertable.h"

#include <cstring>
#include <cstdlib>

#include "localcharacter.h"

CharacterTable::CharacterTable()
    : character_count_(0)
{
}

void CharacterTable::init(void *file_context,
                          CsvTable *table,
                          const char *local_character_name,
                          LocalCharacter *local_character)
{
    if (!table->open(file_context)) {
        return;
    }

    memset(characters_, 0, sizeof(characters_));
    character_count_ = 0;

    memset(local_character, 0, sizeof(LocalCharacter));

    while (table->parse_line()) {
        size_t id;
        const char *name;

        const char *id_field = table->field("rID");
        if (!id_field) {
            continue;
        }

        int id_value = atoi(id_field);
        if (id_value < FIRST_CHARACTER) {
            continue;
        }

        id = static_cast<size_t>(id_value) - FIRST_CHARACTER;
        if (id >= MAX_CHARACTER_COUNT) {
            continue;
        }

        name = table->field("shortName");
        if (!name) {
            continue;
        }
        if (name[0] == '\0') {
            continue;
        }

        if (strcmp(name, local_character_name) == 0) {
            local_character->id = id;

            local_character->corrupted = false;

            const char *dogan_field = table->field("dogan");
            if (dogan_field) {
                int dogan_value = atoi(dogan_field);
                if (dogan_value > MAX_DOGAN) {
                    dogan_value = MAX_DOGAN;
                } else if (dogan_value < MIN_DOGAN) {
                    dogan_value = MIN_DOGAN;
                }

                local_character->dogan = dogan_value;
            }

            const char *manni_field = table->field("isManni");
            if (manni_field) {
                int manni_value = atoi(manni_field);

                local_character->manni = manni_value != 0;
            }
        }

        characters_[id].valid = true;
        memset(characters_[id].name, 0, MAX_CHARACTER_NAME_LENGTH + 1);
        strncpy(characters_[id].name, name, MAX_CHARACTER_NAME_LENGTH);
        if (id > character_count_) {
            character_count_ = id + 1;
        }
    }

    //TODO: ka-tet linking

    table->close();
}

uint8_t CharacterTable::find(const char *name) const
{
    for (uint8_t id = 0; id < character_count_; id++) {
        if (!characters_[id].valid) {
            continue;
        }

        if (strcmp(name, characters_[id].name) == 0) {
            return id;
        }
    }

    return INVALID_CHARACTER;
}
