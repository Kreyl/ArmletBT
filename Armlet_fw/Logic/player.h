#pragma once

#include <cstddef>
#include <cstdint>

#include "emotiontable.h"

class Player
{
public:
    static const size_t TRACKS = 4;

    static const size_t AMBIENT_TRACK = 0;
    static const size_t FIRST_TRACK = 1;
    static const size_t SECOND_TRACK = 2;
    static const size_t PRIORITY_TRACK = 3;

public:
    Player();

    void init(const EmotionTable *emotion_table);

    void handle_track_end(size_t track);

    void begin_emotion(size_t id);

    void end_emotion(size_t id);

    size_t active_emotion_count()
    {
        size_t count = 0;

        if (active_emotions_[FIRST_TRACK]) {
            count++;
        }

        if (active_emotions_[SECOND_TRACK]) {
            count++;
        }

        return count;
    }

private:
    void start_track(size_t track,
                     const EmotionTable::Emotion *emotion,
                     const char *emotion_name);

    void stop_track(size_t track);

private:
    const EmotionTable *emotion_table_;

    const EmotionTable::Emotion *active_emotions_[TRACKS];

    bool ambient_;
};
