#include <iostream>

#include "gameObject.hpp"
#include "component.hpp"

#include <stdexcept>


GameObject::GameObject(GameObject * parent = nullptr, string name, vec2f pos = {0, 0}) 
{
    this->parent = parent;
    this->name = name;
    this->pos = pos;
}

void GameObject::AddComponent(Component * component) 
{
    this->components.push_back(component);
    component->Print();
};

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
            return components[i];
        }
    }

    throw runtime_error(&"Could not find Component");

}


