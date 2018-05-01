#include "dispatcher.h"

#include "qhsm.h"
#include "signals.h"
#include "reasons.h"

Dispatcher::Dispatcher()
{   
}

void Dispatcher::init(const InfluenceTable *influence_table,
                      const EmotionTable *emotion_table,
                      const CharacterTable *character_table,
                      LocalCharacter *local_character)
{
    influence_table_ = influence_table;
    emotion_table_ = emotion_table;
    character_table_ = character_table;

    local_character_ = local_character;

    Character_ctor(&character_sm_,
                   this,
                   local_character_->dogan,
                   local_character_->manni,
                   false,
                   local_character_->dead);
    KaCounter_ctor(&ka_counter_sm_,
                   this,
                   &(local_character_->ka_tet_counters),
                   &(local_character_->ka_tet_links));
    KaTet_ctor(&ka_tet_sm_,
               this,
               &(local_character_->ka_tet_links));
    Screen_ctor(&screen_sm_,
                this);

    QEvt init_event = {Q_INIT_SIG};

    QMSM_INIT(&(character_sm_.super), &init_event);
    QMSM_INIT(&(ka_counter_sm_.super), &init_event);
    QMSM_INIT(&(ka_tet_sm_.super), &init_event);
    QMSM_INIT(&(screen_sm_.super), &init_event);

    ticks_ = 0;
}

void Dispatcher::handle_button(int id, bool long_press)
{
    ScreenQEvt screen_event = {{BTN_PRESSED_SIG}, 0, false, false};
    QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));

    //TODO: dispatch different buttons
    (void)id;
    (void)long_press;
}

void Dispatcher::handle_radio_packet(uint8_t influence_id,
                                     uint8_t parameter,
                                     int8_t strength)
{
    const InfluenceTable::Influence *influence;
    influence = influence_table_->influence(influence_id);
    if (!influence || !(influence->valid)) {
        return;
    }

    if (strength < influence->strength) {
        return;
    }

    begin_influence(influence_id, parameter, influence->timeout);
}

void Dispatcher::handle_nfc_packet(uint8_t influence_id)
{
    const InfluenceTable::Influence *influence;
    influence = influence_table_->influence(influence_id);
    if (!influence || !(influence->valid)) {
        return;
    }

    begin_influence(influence_id, 0, 0);
}

void Dispatcher::begin_influence(size_t id,
                                 uint8_t parameter,
                                 int16_t timeout)
{
    (void)parameter;

    if (!influence_active(id)) {
        uint8_t logical_id = static_cast<uint8_t>(id);
        uint8_t character_id = CharacterTable::INVALID_CHARACTER;

        if (logical_id >= CharacterTable::FIRST_CHARACTER) {
            character_id = logical_id - CharacterTable::FIRST_CHARACTER;
            logical_id = PERSON_NEAR_ID;
        }

        QSignal signal = static_cast<QSignal>(BEGIN_BY_ID(logical_id) + BASE_SIG);

        CharacterQEvt character_event = {{signal}, 0};
        QMSM_DISPATCH(&(character_sm_.super), &(character_event.super));

        KaCounterQEvt ka_counter_event = {{signal}, character_id};
        QMSM_DISPATCH(&(ka_counter_sm_.super), &(ka_counter_event.super));

        KaTetQEvt ka_tet_event = {{signal}, character_id};
        QMSM_DISPATCH(&(ka_tet_sm_.super), &(ka_tet_event.super));
    }

    influence_states_[id].remaining_time = timeout;
    if (influence_states_[id].remaining_time == 0) {
        end_influence(id);
    }
}

void Dispatcher::end_influence(size_t id)
{
    uint8_t logical_id = static_cast<uint8_t>(id);
    uint8_t character_id = CharacterTable::INVALID_CHARACTER;

    if (logical_id >= CharacterTable::FIRST_CHARACTER) {
        character_id = logical_id - CharacterTable::FIRST_CHARACTER;
        logical_id = PERSON_NEAR_ID;
    }

    QSignal signal = static_cast<QSignal>(END_BY_ID(logical_id) + BASE_SIG);

    CharacterQEvt character_event = {{signal}, 0};
    QMSM_DISPATCH(&(character_sm_.super), &(character_event.super));

    KaCounterQEvt ka_counter_event = {{signal}, character_id};
    QMSM_DISPATCH(&(ka_counter_sm_.super), &(ka_counter_event.super));

    KaTetQEvt ka_tet_event = {{signal}, character_id};
    QMSM_DISPATCH(&(ka_tet_sm_.super), &(ka_tet_event.super));

    influence_states_[id].remaining_time = 0;
}

bool Dispatcher::influence_active(size_t id)
{
    return influence_states_[id].remaining_time != 0;
}

void Dispatcher::tick()
{
    const size_t influence_count = influence_table_->influence_count();

    int dogan_effect = 0;

    for (size_t influence_id = 0;
         influence_id < influence_count;
         influence_id++) {
        if (influence_states_[influence_id].remaining_time != 0) {
            const InfluenceTable::Influence *influence;
            influence = influence_table_->influence(influence_id);
            if (influence && influence->valid) {
                dogan_effect += influence->dogan_effect;
            }

            if (influence_states_[influence_id].remaining_time > 0) {
                influence_states_[influence_id].remaining_time--;
            }
            if (influence_states_[influence_id].remaining_time == 0) {
                end_influence(influence_id);
            }
        }
    }

    CharacterQEvt character_event = {{INFLUENCE_AT_DOGAN_SIG}, dogan_effect};
    QMSM_DISPATCH(&(character_sm_.super), &(character_event.super));

    KaCounterQEvt ka_counter_event = {{TIME_TICK_1S_SIG}, 0};
    QMSM_DISPATCH(&(ka_counter_sm_.super), &(ka_counter_event.super));

    ScreenQEvt screen_event = {{TIME_TICK_1S_SIG}, 0, false, false};
    QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));

    ticks_++;

    if (ticks_ >= 60) {
        ticks_ = 0;

        ka_counter_event = {{TIME_TICK_1M_SIG}, 0};
        QMSM_DISPATCH(&(ka_counter_sm_.super), &(ka_counter_event.super));

        screen_event = {{TIME_TICK_1M_SIG}, 0, false, false};
        QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
    }
}
