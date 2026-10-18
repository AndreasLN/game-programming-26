#ifndef RENDERED
#define RENDERED


#include <itu_engine.hpp>
#include "component.hpp"


class Rendered{
    
    public:
        Rendered();
        virtual void Render(EngineContext * context);
};

#endif
