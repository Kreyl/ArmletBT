#include "player.h"

#include "bsp.h"

Player::Player()
{
}

void Player::init(const EmotionTable *emotion_table)
{
    emotion_table_ = emotion_table;
}

void Player::handle_track_end(int track)
{

}

void Player::begin_emotion(size_t id)
{
    const EmotionTable::Emotion *emotion;
    emotion = emotion_table_->emotion(id);
    if (!emotion || !(emotion->valid)) {
        return;
    }
}

void Player::end_emotion(size_t id)
{
    const EmotionTable::Emotion *emotion;
    emotion = emotion_table_->emotion(id);
    if (!emotion || !(emotion->valid)) {
        return;
    }
}
