#include "gameObject.hpp"
#include "component.hpp"
#include <iostream>



Component::Component(GameObject &gameObject)
: gameObject(gameObject)
{
    this->name = name;
    gameObject.AddComponent(this);
    
}

void Component::Print(){
    std::cout << "hi this is game component";
}
