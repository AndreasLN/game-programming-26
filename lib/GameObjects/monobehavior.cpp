#include <iostream>
#include <gameObjectsLib.hpp>

using namespace std;


MonoBehavior::MonoBehavior(GameObject &gameObject) : Component(gameObject)
{
    
}

void MonoBehavior::Awake(){
    gameObject.AddComponent(this);
}

void MonoBehavior::Print(){
    std::cout << "This is a Monobehavior class" << endl;
}




