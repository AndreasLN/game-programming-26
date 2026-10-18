
#ifndef GAMEOBJECT
#define GAMEOBJECT

#include <itu_engine.hpp>

using namespace std;
#include <vector>

class MonoBehavior;

class GameObject{
    private:
    Transform2D transform;
    vec2f pos;
    vector<MonoBehavior*> components;
    GameObject * parent; // can be null
    vector<GameObject*> children;
    string name;
    
    public:
    
    GameObject();
    GameObject(GameObject * parent, string name, vec2f pos);

    Transform2D GetTransform2D() {return transform; }
    GameObject * GetParent() {return parent; };
    vec2f GetPos() {return pos; };
    vector<GameObject*> GetChildren() {return children; };
    vector<MonoBehavior*> GetComponents() {return components; };
    
    template<typename T> T* GetComponentofType(string name);

    friend bool operator==(const GameObject& lhs, const GameObject& rhs);


    void AddComponent(MonoBehavior * component);
    void RemoveComponent(MonoBehavior * component);
    void AddChild(GameObject * gameObject);
    void RemoveChild(GameObject * gameObject);
    void Destroy();
    void Print();

    
};

#endif
