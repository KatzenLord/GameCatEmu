//
// Created by katzenlord on 04.10.26.
//

#ifndef GAMECATEMU_INPUTMAPPER_H
#define GAMECATEMU_INPUTMAPPER_H

#pragma once
#include "InputHandler.h"
#include "InputState.h"

class InputMapper {
public:
    explicit InputMapper(const InputHandler& inputHandler);

    InputState getState() const;
private:
    const InputHandler& inputHandler;
};


#endif //GAMECATEMU_INPUTMAPPER_H
