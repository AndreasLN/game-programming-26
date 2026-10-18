#ifndef MONOBEHAVIOR
#define MONOBEHAVIOR


#include <itu_engine.hpp>
#include "component.hpp"


class MonoBehavior : public Component{
    
    
    public:
        MonoBehavior(GameObject &gameObject);
        void Awake();
        void Start();
        void Update(float delta);
        void Print();
};

#endif
