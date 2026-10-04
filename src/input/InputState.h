//
// Created by katzenlord on 04.10.26.
//

#ifndef GAMECATEMU_INPUTSTATE_H
#define GAMECATEMU_INPUTSTATE_H

#pragma once

struct InputState {
    bool right = false;
    bool left = false;
    bool up = false;
    bool down = false;

    bool a = false;
    bool b = false;
    bool start = false;
    bool select = false;
};

#endif //GAMECATEMU_INPUTSTATE_H
