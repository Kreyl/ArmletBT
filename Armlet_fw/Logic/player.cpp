#include "player.h"

#include "bsp.h"

static const uint16_t UNIT_VOLUME = 4096;

static const char *AMBIENT_NAME = "AMBIENT";

Player::Player()
    : active_emotions_{}
{
}

void Player::init(const EmotionTable *emotion_table)
{
    emotion_table_ = emotion_table;

    start_track(AMBIENT_TRACK, nullptr, AMBIENT_NAME);
    ambient_ = true;
}

void Player::handle_track_end(size_t track)
{
    if (track >= TRACKS) {
        return;
    }

    if (track == AMBIENT_TRACK) {
        if (ambient_) {
            start_track(AMBIENT_TRACK, nullptr, AMBIENT_NAME);
        }
    } else {
        const EmotionTable::Emotion *emotion = active_emotions_[track];
        if (!emotion || !(emotion->valid)) {
            return;
        }

        if (emotion->type == EmotionTable::Emotion::Type::Continuous) {
            start_track(track, emotion, emotion->name);
        } else {
            active_emotions_[track] = nullptr;

            if ((active_emotion_count() == 0) && !ambient_) {
                start_track(AMBIENT_TRACK, nullptr, AMBIENT_NAME);
                ambient_ = true;
            }
        }
    }
}

void Player::begin_emotion(size_t id)
{
    const EmotionTable::Emotion *emotion;
    emotion = emotion_table_->emotion(id);
    if (!emotion || !(emotion->valid)) {
        return;
    }

    for (size_t track = AMBIENT_TRACK + 1; track < TRACKS; track++) {
        if (emotion == active_emotions_[track]) {
            return;
        }
    }

    int8_t priority = emotion->priority;
    if (priority < 0) {
        if (active_emotions_[PRIORITY_TRACK] != nullptr) {
            stop_track(PRIORITY_TRACK);
        }
        start_track(PRIORITY_TRACK, emotion, emotion->name);
        return;
    }

    const EmotionTable::Emotion *first_emotion;
    const EmotionTable::Emotion *second_emotion;

    first_emotion = active_emotions_[FIRST_TRACK];
    second_emotion = active_emotions_[SECOND_TRACK];

    size_t track;

    switch (active_emotion_count()) {
    case 0:
        track = FIRST_TRACK;
        start_track(track, emotion, emotion->name);
        break;

    case 1:
        if (first_emotion) {
            track = SECOND_TRACK;
        } else {
            track = FIRST_TRACK;
        }
        start_track(track, emotion, emotion->name);
        break;

    case 2:
        if ((priority > first_emotion->priority) &&
            (priority > second_emotion->priority)) {
            break;
        }
        if (first_emotion->priority < second_emotion->priority) {
            track = SECOND_TRACK;
        } else {
            track = FIRST_TRACK;
        }
        stop_track(track);
        start_track(track, emotion, emotion->name);
        break;
    }

    if ((active_emotion_count() != 0) && ambient_) {
        stop_track(AMBIENT_TRACK);
        ambient_ = false;
    }
}

void Player::end_emotion(size_t id)
{
    const EmotionTable::Emotion *emotion;
    emotion = emotion_table_->emotion(id);
    if (!emotion || !(emotion->valid)) {
        return;
    }

    if (emotion->type == EmotionTable::Emotion::Type::Single) {
        return;
    }

    for (size_t track = AMBIENT_TRACK + 1; track < TRACKS; track++) {
        if (emotion == active_emotions_[track]) {
            stop_track(track);
        }
    }

    if ((active_emotion_count() == 0) && !ambient_) {
        start_track(AMBIENT_TRACK, nullptr, AMBIENT_NAME);
        ambient_ = true;
    }
}

void Player::start_track(size_t track,
                         const EmotionTable::Emotion *emotion,
                         const char *emotion_name)
{
    if (track >= TRACKS) {
        return;
    }

    PlayerStart(static_cast<uint8_t>(track),
                UNIT_VOLUME,
                emotion_name,
                false);
    active_emotions_[track] = emotion;
}

void Player::stop_track(size_t track)
{
    if (track >= TRACKS) {
        return;
    }

    PlayerStop(static_cast<uint8_t>(track));
    active_emotions_[track] = nullptr;
}
