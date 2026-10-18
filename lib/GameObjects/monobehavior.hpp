#ifndef MONOBEHAVIOR
#define MONOBEHAVIOR


#include <itu_engine.hpp>
#include "component.hpp"


class MonoBehavior : public Component{
    
    
    public:
        MonoBehavior(GameObject &gameObject);
        virtual void Awake();
        virtual void Start();
        virtual void Update(float delta);
        void Print();
};

#endif
