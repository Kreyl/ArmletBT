#include "dispatcher.h"

#include "qhsm.h"
#include "signals.h"
#include "reasons.h"

enum Buttons
{
    A,
    B,
    C,
    L,
    E,
    R,
    X,
    Y,
    Z
};

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

    Screen_ctor(&screen_sm_,
                this);
    Character_ctor(&character_sm_,
                   this,
                   local_character_->dogan,
                   local_character_->manni,
                   local_character_->corrupted,
                   local_character_->dead);
    KaCounter_ctor(&ka_counter_sm_,
                   this,
                   &(local_character_->ka_tet_counters),
                   &(local_character_->ka_tet_links),
                   &(local_character_->near_characters));
    KaTet_ctor(&ka_tet_sm_,
               this,
               &(local_character_->ka_tet_links));

    QEvt init_event = {Q_INIT_SIG};

    QMSM_INIT(&(screen_sm_.super), &init_event);
    QMSM_INIT(&(character_sm_.super), &init_event);
    QMSM_INIT(&(ka_counter_sm_.super), &init_event);
    QMSM_INIT(&(ka_tet_sm_.super), &init_event);

    player_.init(emotion_table_);

    process_queue();

    ticks_ = 0;
}

void Dispatcher::handle_button(int id, bool long_press)
{
    ScreenQEvt screen_event = {{BTN_PRESSED_SIG}, 0, false, false};
    QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));

    (void)long_press;

    switch (id) {
    case Buttons::A:
        if (long_press) {
            screen_event = {{BTN_DEATH_LONG_SIG}, 0, false, false};
            QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        }
        break;
    case Buttons::B:
        if (long_press) {
            screen_event = {{BTN_HYPNOSIS_LONG_SIG}, 0, false, false};
            QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        }
        break;
    case Buttons::C:
        if (long_press) {
            screen_event = {{BTN_POWER_LONG_SIG}, 0, false, false};
            QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        }
        break;
    case Buttons::L:
        break;
    case Buttons::E:
        if (long_press) {
            screen_event = {{BTN_ENABLE_LONG_SIG}, 0, false, false};
            QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        }
        break;
    case Buttons::R:
        screen_event = {{BTN_DOGAN_SIG}, 0, false, false};
        QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        break;
    case Buttons::X:
        screen_event = {{BTN_UP_SIG}, 0, false, false};
        QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        break;
    case Buttons::Y:
        screen_event = {{BTN_DOWN_SIG}, 0, false, false};
        QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        break;
    case Buttons::Z:
        screen_event = {{BTN_NEXT_PICTURE_SIG}, 0, false, false};
        QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
        break;
    }

    process_queue();
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

    process_queue();
}

void Dispatcher::handle_nfc_packet(uint8_t influence_id)
{
    const InfluenceTable::Influence *influence;
    influence = influence_table_->influence(influence_id);
    if (!influence || !(influence->valid)) {
        return;
    }

    begin_influence(influence_id, 0, 0);

    process_queue();
}

void Dispatcher::handle_track_end(size_t track)
{
    player_.handle_track_end(track);
}

void Dispatcher::handle_battery_status(uint8_t level,
                                       bool charging,
                                       bool connected)
{
    ScreenQEvt screen_event = {{BATTERY_INFO_SIG}, level, charging, connected};
    QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));
}

void Dispatcher::begin_influence(size_t id,
                                 uint8_t parameter,
                                 int16_t timeout)
{
    if (!influence_active(id)) {
        QueueEntry entry = {id, QueueEntry::Action::Begin, parameter};
        queue_.push(entry);
    }

    influence_states_[id].remaining_time = timeout;
    if (influence_states_[id].remaining_time == 0) {
        end_influence(id);
    }
}

void Dispatcher::end_influence(size_t id)
{
    influence_states_[id].remaining_time = 0;

    QueueEntry entry = {id, QueueEntry::Action::End, 0};
    queue_.push(entry);
}

bool Dispatcher::influence_active(size_t id)
{
    return influence_states_[id].remaining_time != 0;
}

void Dispatcher::tick()
{
    static const int DEFAULT_DOGAN_EFFECT = -1;
    int dogan_effect = DEFAULT_DOGAN_EFFECT;

    for (size_t influence_id = 0;
         influence_id < influence_table_->influence_count();
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

    ScreenQEvt screen_event = {{TIME_TICK_1S_SIG}, 0, false, false};
    QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));

    CharacterQEvt character_event = {{INFLUENCE_AT_DOGAN_SIG}, dogan_effect};
    QMSM_DISPATCH(&(character_sm_.super), &(character_event.super));

    KaCounterQEvt ka_counter_event = {{TIME_TICK_1S_SIG}, 0};
    QMSM_DISPATCH(&(ka_counter_sm_.super), &(ka_counter_event.super));

    ticks_++;

    if (ticks_ >= 60) {
        ticks_ = 0;

        screen_event = {{TIME_TICK_1M_SIG}, 0, false, false};
        QMSM_DISPATCH(&(screen_sm_.super), &(screen_event.super));

        ka_counter_event = {{TIME_TICK_1M_SIG}, 0};
        QMSM_DISPATCH(&(ka_counter_sm_.super), &(ka_counter_event.super));
    }

    process_queue();
}

void Dispatcher::process_queue()
{
    while (queue_.size() != 0) {
        QueueEntry entry;
        queue_.pop(entry);

        uint8_t logical_id = static_cast<uint8_t>(entry.id);
        uint8_t character_id = CharacterTable::INVALID_CHARACTER;

        if (logical_id >= CharacterTable::FIRST_CHARACTER) {
            character_id = logical_id - CharacterTable::FIRST_CHARACTER;
            logical_id = PERSON_NEAR_ID;
        }

        QSignal signal = static_cast<QSignal>(BASE_SIG);
        switch (entry.action) {
        case QueueEntry::Action::Begin:
            signal += static_cast<QSignal>(BEGIN_BY_ID(logical_id));
            local_character_->near_characters.set(character_id, true);
            break;
        case QueueEntry::Action::End:
            signal += static_cast<QSignal>(END_BY_ID(logical_id));
            local_character_->near_characters.set(character_id, false);
            break;
        }

        CharacterQEvt character_event = {{signal}, 0};
        QMSM_DISPATCH(&(character_sm_.super), &(character_event.super));

        KaCounterQEvt ka_counter_event = {{signal}, character_id};
        QMSM_DISPATCH(&(ka_counter_sm_.super), &(ka_counter_event.super));

        KaTetQEvt ka_tet_event = {{signal}, character_id};
        QMSM_DISPATCH(&(ka_tet_sm_.super), &(ka_tet_event.super));

        const InfluenceTable::Influence *influence;
        influence = influence_table_->influence(logical_id);
        if (influence && influence->valid) {
            size_t emotion_id = influence->emotion;
            if (emotion_id != EmotionTable::INVALID_EMOTION) {
                switch (entry.action) {
                case QueueEntry::Action::Begin:
                    player_.begin_emotion(emotion_id);
                    break;
                case QueueEntry::Action::End:
                    player_.end_emotion(emotion_id);
                    break;
                }
            }
        }
    }
}
