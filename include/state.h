#ifndef STATE_H
#define STATE_H

class State
{
public:
    static State &getInstance()
    {
        static State instance;
        return instance;
    }

    volatile bool play_next_song_flag = false;
    volatile bool play_previous_song_flag = false;
    volatile bool play_song_flag = false;
    volatile bool pause_song_flag = false;

private:
    State() = default;
    ~State() = default;

    State(const State &) = delete;
    State &operator=(const State &) = delete;
};

#endif // STATE_H
