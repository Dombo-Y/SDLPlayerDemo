//
//  Condmutex.cpp
//  SDLTestDemo
//
//  Created by 尹东博 on 2023/4/14.
//

#include "Condmutex.hpp" 

CondMutex::CondMutex() {
    _mutex = SDL_CreateMutex();//创建互斥锁
    _cond = SDL_CreateCond(); //创建条件变量
}
CondMutex::~CondMutex() {
    
  SDL_DestroyMutex(_mutex);
  SDL_DestroyCond(_cond);
    
}
void CondMutex::lock(){
    
   SDL_LockMutex(_mutex);
}

void CondMutex::unlock(){
    SDL_UnlockMutex(_mutex);
}

void CondMutex::signal(){
    SDL_CondSignal(_cond);
}

void CondMutex::broadcast(){
   SDL_CondBroadcast(_cond);
}

void CondMutex::wait(){
    SDL_CondWait(_cond,_mutex);
}
