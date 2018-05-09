#pragma once

#include <cstddef>
#include <cstdint>

#include "emotiontable.h"

class Player
{
public:
    Player();

    void init(const EmotionTable *emotion_table);

    void handle_track_end(int track);

    void begin_emotion(size_t id);

    void end_emotion(size_t id);

private:
    const EmotionTable *emotion_table_;
};
