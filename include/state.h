#ifndef STATE_H
#define STATE_H

enum colors {
    Blue,
    Green,
    Red,
};

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
    volatile bool stop_song_flag = false;
    volatile bool switch_cyw43_mode = false;
    volatile enum colors color_picked;

private:
    State() = default;
    ~State() = default;

    State(const State &) = delete;
    State &operator=(const State &) = delete;
};

#endif // STATE_H
