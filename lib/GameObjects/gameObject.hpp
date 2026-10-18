
#ifndef GAMEOBJECT
#define GAMEOBJECT

#include <itu_engine.hpp>

using namespace std;
#include <vector>

class MonoBehavior;

class GameObject{
    private:
    vector<MonoBehavior*> components;
    GameObject * parent; // can be null
    vector<GameObject*> children;
    string name;
    
    public:
    Transform2D transform;
    
    GameObject();
    GameObject(GameObject * parent, string name, Transform2D transform);

    Transform2D GetTransform2D() {return transform; }
    GameObject * GetParent() {return parent; };
    vector<GameObject*> GetChildren() {return children; };
    vector<MonoBehavior*> GetComponents() {return components; };
    


    template<typename T> T* GetComponentofType(string name);

    friend bool operator==(const GameObject& lhs, const GameObject& rhs);

    void SetScale(vec2f scale);
    void SetRotation(float rotation);
    void SetPosition(vec2f pos);

    void AddComponent(MonoBehavior * component);
    void RemoveComponent(MonoBehavior * component);
    void AddChild(GameObject * gameObject);
    void RemoveChild(GameObject * gameObject);
    void Destroy();
    void Print();

    
};

#endif
