//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include "Windows.h"

// todo : do u see its need to get more upgrade and profensiol?

template<typename T>
class CSingletonT {
public:
    inline CSingletonT() {
        if (ptr != NULL)
            return;
        ptr = this;
    }

    inline ~CSingletonT() {
        ptr = NULL;
    }

    // TODO : MAKE IT PRIVATE FOR NOW TO AVOID USING IT!!!
private:
    static T *ptr;
};
