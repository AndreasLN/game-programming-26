
#include <itu_engine.hpp>

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


