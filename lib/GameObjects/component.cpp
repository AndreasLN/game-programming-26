#include <gameObjectsLib.hpp>


Component::Component(GameObject &gameObject)
: gameObject(gameObject)
{
    this->name = name;
}



void Component::Print(){
    std::cout << "This is game component";
}

void Component::Destroy(){
    delete this;
}


