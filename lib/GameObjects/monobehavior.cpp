#include <iostream>
#include <gameObjectsLib.hpp>

using namespace std;


MonoBehavior::MonoBehavior(GameObject &gameObject) : Component(gameObject)
{
    
}

void MonoBehavior::Print() 
{
    std::cout << "This is a monobehavior class";
}





