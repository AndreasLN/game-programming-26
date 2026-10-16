#include <iostream>
#include <itu_engine.hpp>

#include "monobehavior.hpp"

using namespace std;


MonoBehavior::MonoBehavior(GameObject &gameObject) : Component(gameObject)
{
    
}

void MonoBehavior::Print() 
{
    std::cout << "This is a monobehavior class";
}





