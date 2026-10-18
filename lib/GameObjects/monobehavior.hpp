#ifndef MONOBEHAVIOR
#define MONOBEHAVIOR


#include <itu_engine.hpp>
#include "component.hpp"


class MonoBehavior : public Component{
    
    
    public:
        MonoBehavior(GameObject &gameObject);
        void Awake();
        void Start();
        void Update(SDL_Time time_elapsed_frame);
        void Print();
};

#endif
