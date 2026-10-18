#include <iostream>
#include <gameObjectsLib.hpp>

using namespace std;


MonoBehavior::MonoBehavior(GameObject &gameObject) : Component(gameObject)
{
    
}

void MonoBehavior::Awake(){
    gameObject.AddComponent(this);
}

void MonoBehavior::Start(){
}

void MonoBehavior::Update(float delta){
}

void MonoBehavior::Print(){
    std::cout << "This is a Monobehavior class" << endl;
}

void MonoBehavior::Destroy(){
    delete this;
}


