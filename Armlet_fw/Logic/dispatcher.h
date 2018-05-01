#pragma once

#include <cstddef>
#include <cstdint>

#include "influencetable.h"
#include "emotiontable.h"
#include "charactertable.h"
#include "localcharacter.h"
#include "ringbuffer.h"

#include "character.h"
#include "kaCounter.h"
#include "kaTet.h"
#include "screen.h"

#define BEGIN_BY_ID(id) \
    (id)
#define END_BY_ID(id) \
    (BEGIN_BY_ID(id) + InfluenceTable::MAX_INFLUENCE_COUNT + 1)

#define BEGIN(name) (BEGIN_BY_ID(name ## _ID))
#define END(name) (END_BY_ID(name ## _ID))

#define DISPATCH_BEGIN_WITH_PARAM(name, param) \
    me->dispatcher->begin_influence((name ## _ID), (param))
#define DISPATCH_ONESHOT_WITH_PARAM(name, param) \
    me->dispatcher->begin_influence((name ## _ID), (param), 0)

#define DISPATCH_BEGIN(name) \
    me->dispatcher->begin_influence((name ## _ID))
#define DISPATCH_ONESHOT(name) \
    me->dispatcher->begin_influence((name ## _ID), 0, 0)
#define DISPATCH_END(name) \
    me->dispatcher->end_influence((name ## _ID))

class Dispatcher
{
public:
    struct InfluenceState
    {
        int16_t remaining_time;
    };

    struct QueueEntry
    {
        enum class Action
        {
            Begin,
            End
        };

        size_t id;
        Action action;
        uint8_t parameter;
    };

public:
    Dispatcher();

    void init(const InfluenceTable *influence_table,
              const EmotionTable *emotion_table,
              const CharacterTable *character_table,
              LocalCharacter *local_character);

    void handle_button(int id, bool long_press = false);

    void handle_radio_packet(uint8_t influence_id,
                             uint8_t parameter,
                             int8_t strength);

    void handle_nfc_packet(uint8_t influence_id);

    void begin_influence(size_t id,
                         uint8_t parameter = 0,
                         int16_t timeout = -1);
    void end_influence(size_t id);

    bool influence_active(size_t id);

    void tick();

private:
    void process_queue();

private:
    InfluenceState influence_states_[InfluenceTable::MAX_INFLUENCE_COUNT];

    const InfluenceTable *influence_table_;
    const EmotionTable *emotion_table_;
    const CharacterTable *character_table_;

    LocalCharacter *local_character_;

    RingBuffer<QueueEntry, 32> queue_;

    Character character_sm_;
    KaCounter ka_counter_sm_;
    KaTet ka_tet_sm_;
    Screen screen_sm_;

    int ticks_;
};
