#include "FactoryBase.h"

GameObject &FactoryBase::create(const GameObjectSpec &spec)
{
    return createImpl(spec);
}