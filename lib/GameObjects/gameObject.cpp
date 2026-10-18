#include <iostream>

#include <gameObjectsLib.hpp>

#include <stdexcept>
#include <algorithm>


GameObject::GameObject(GameObject * parent, string name, Transform2D transform) 
{
    this->parent = parent;
    this->name = name;
}

GameObject::GameObject() 
{
    this->parent = parent;
    this->name = name;
}

void GameObject::AddChild(GameObject * gameObject) 
{
    this->children.push_back(gameObject);
    SDL_Log("Added child %s", gameObject->name.c_str());
};

void GameObject::AddComponent(MonoBehavior * component) 
{
    this->components.push_back(component);
};

bool operator==(const GameObject& lhs, const GameObject& rhs){
    return lhs.name == rhs.name;
}


void GameObject::Print(){
    std::cout << this->name;
}

void GameObject::Destroy(){

    for (size_t i = 0; i < this->children.size(); ++i)
    {
        children[i]->Destroy();
    }
    children.clear();

    components.clear();
    
    parent->children.erase(find(parent->children.begin(), parent->children.end(), this));
    
    std::cout << this->name << " is being deleted.." << std::endl;

    delete this;
}



template<typename T> T* GameObject::GetComponentofType(string name)
{
    for (size_t i = 0; i < components.size(); i++)
    {
        if(name.compare(components[i]->GetName()) == 0){
            return static_cast<T*>(components[i]);
        }
    }

    throw runtime_error("Could not find Component");

}


