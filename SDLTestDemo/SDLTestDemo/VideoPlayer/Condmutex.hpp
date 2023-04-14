//
//  Condmutex.hpp
//  SDLTestDemo
//
//  Created by 尹东博 on 2023/4/14.
//

#ifndef Condmutex_hpp
#define Condmutex_hpp

#include "SDL.h"
#include <stdio.h>

class CondMutex {
    
    
public:
    CondMutex();
    ~CondMutex();

    void lock();
    void unlock();
    void signal();
    void broadcast();
    void wait();
    
private:
    SDL_mutex *_mutex = nullptr; /** 互斥锁 */ 
    SDL_cond *_cond = nullptr;/** 条件变量 */
};

#endif /* Condmutex_hpp */
