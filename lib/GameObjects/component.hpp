#ifndef COMPONENT
#define COMPONENT


#include <itu_engine.hpp>
#include <iostream>

using namespace std;

class GameObject;

class Component{
    protected: 
    string name;
    
    public:
    GameObject &gameObject; // the gameobject this component is attached to. Cannot be null

    Component(GameObject &gameObject);
    GameObject GetGameObject();
    string GetName() { return name; };
    void Print();
};

#endif


