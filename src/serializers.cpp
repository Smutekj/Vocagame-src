
#include "serializers.h"

#include "Factories.h"

static std::unordered_map<std::type_index, std::function<nlohmann::json(GameObjectSpec &)>>
    s_spec_serializers{};
static std::unordered_map<TypeId, std::function<std::shared_ptr<GameObjectSpec>(const nlohmann::json &data)>>
    s_spec_deserializers{};

nlohmann::json serializeSpec(GameObjectSpec &spec)
{
    if (!s_spec_serializers.contains(spec.rtti))
    {
        // std::cout << "Serializer for spec not registered!" << std::endl;
        return {};
    }
    return s_spec_serializers.at(spec.rtti)(spec);
}

std::shared_ptr<GameObjectSpec> deserializeSpec(TypeId rtti, const nlohmann::json &json)
{
    if (!s_spec_deserializers.contains(rtti))
    {
        // std::cout << "Deserializer for spec not registered!" << std::endl;
        return nullptr;
    }
    return s_spec_deserializers.at(rtti)(json);
}

template <class EntityType>
void registerSpecSerializer()
{
    using SpecType = EntityType::Spec;
    if constexpr (boost::describe::has_describe_members<SpecType>::value)
    {

        TypeId type_id = getTypeId<EntityType>();

        const auto &rtti = typeid(SpecType);
        if (!s_spec_serializers.contains(rtti))
        {
            s_spec_serializers[rtti] = [](GameObjectSpec &spec)
            {
                return serialize(static_cast<SpecType &>(spec));
            };

            s_spec_deserializers[type_id] = [type_id](const nlohmann::json &data)
            {
                auto spec = deserialize<SpecType>(data);
                spec.obj_type = type_id;
                return std::make_shared<SpecType>(spec);
            };
        }
    }
};

template <class... EntityTypes>
void registerTheSerializers()
{
    (registerSpecSerializer<EntityTypes>(), ...);
}

void registerSerializers()
{
    registerTheSerializers<TYPE_LIST>();
}