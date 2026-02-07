#pragma once

#include "nlohmann/json.hpp"
#include "boost/describe.hpp"
#include "boost/mp11.hpp"

#include <typeindex>
#include "ObjectRegistry.h"
#include "Components.h"
class GameObjectSpec;

void registerSerializers();
nlohmann::json serializeSpec(GameObjectSpec &spec);
std::shared_ptr<GameObjectSpec> deserializeSpec(TypeId rtti, const nlohmann::json& json);


template <typename T>
struct is_string
    : public std::disjunction<
          std::is_same<char *, std::decay_t<T>>,
          std::is_same<const char *, std::decay_t<T>>,
          std::is_same<std::string, std::decay_t<T>>>
{
};

template <template <typename...> class Template, typename T>
struct is_instantiation_of : std::false_type
{
};

template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template, Template<Args...>> : std::true_type
{
};

template <typename T>
struct get_inner;

// Partial specialization for class templates of the form Template<Arg>
template <template <typename> class C, typename A>
struct get_inner<C<A>>
{
    using type = A;
};

template <class ContainerType>
using get_inner_t = get_inner<ContainerType>::type;

template <class E>
struct enum_descriptor
{
    E value;
    char const *name;
};

template <class E, template <class... T> class L, class... T>
constexpr std::array<enum_descriptor<E>, sizeof...(T)>
describe_enumerators_as_array_impl(L<T...>)
{
    return {{{T::value, T::name}...}};
}

template <class E>
constexpr auto describe_enumerators_as_array()
{

    if constexpr (boost::describe::has_describe_enumerators<E>::value)
    {
        return describe_enumerators_as_array_impl<E>(boost::describe::describe_enumerators<E>());
    }
    else
    {
        // static_assert(false, "Enum not described!");
        return std::array<enum_descriptor<E>, 0>{};
    }
}

template <class EnumType>
char const *enumToString(EnumType e)
{
    char const *r = "(unnamed)";
    boost::mp11::mp_for_each<boost::describe::describe_enumerators<EnumType>>([&](auto D)
                                                                              {
                                                                                  if (e == D.value)
                                                                                      r = D.name; });
    return r;
}

[[noreturn]] inline void throw_invalid_name(char const *name, char const *type)
{
    throw std::runtime_error(
        std::string("Invalid enumerator name '") + name + "' for enum type '" + type + "'");
}

template <class EnumType>
EnumType stringToEnum(const std::string &name)
{
    bool found = false;
    EnumType r = {};
    boost::mp11::mp_for_each<boost::describe::describe_enumerators<EnumType>>([&](auto D)
                                                                              {
                                                                           if (!found && std::strcmp(D.name, name.c_str()) == 0)
                                                                           {
                                                                               found = true;
                                                                               r = D.value;
                                                                           } });
    if (found)
    {
        return r;
    }
    throw_invalid_name(name.c_str(), typeid(EnumType).name());
}

template <class DataType>
nlohmann::json &toJson(DataType &value, const std::string &key_name, nlohmann::json &data)
{
    if constexpr (is_instantiation_of<std::vector, DataType>::value)
    {
        nlohmann::json json_array = nlohmann::json::array();
        for (auto &el : value)
        {
            nlohmann::json json_array_el = {};
            toJson(el, "el", json_array_el);
            json_array.push_back(json_array_el["el"]);
        }
        data[key_name] = json_array;
    }
    else if constexpr (std::is_arithmetic_v<DataType> || is_string<DataType>::value)
    {
        data[key_name] = value;
    }
    else if constexpr (std::is_same_v<DataType, utils::Vector2f> || std::is_same_v<DataType, utils::Vector2i>)
    {
        data[key_name] = {value.x, value.y};
    }
    else if constexpr (std::is_same_v<DataType, Rectf> || std::is_same_v<DataType, Recti>)
    {
        data[key_name] = {value.pos_x, value.pos_y, value.width, value.height};
    }
    else if constexpr (std::is_same_v<DataType, SpriteSpec>)
    {
        auto &spec = static_cast<SpriteSpec &>(value);
        nlohmann::json spec_json = {};
        toJson(spec.tex_rect, "tex_rect", spec_json);
        toJson(spec.texture_id, "texture_id", spec_json);
        toJson(spec.tex_size, "tex_size", spec_json);
        toJson(spec.color, "color", spec_json);
        toJson(spec.shader_id, "shader_id", spec_json);
        data[key_name] = spec_json;
    }
    else if constexpr (std::is_enum_v<DataType>)
    {
        if constexpr (boost::describe::has_describe_enumerators<DataType>::value)
        {
            data[key_name] = enumToString(value);
        }
        else
        {
            data[key_name] = static_cast<int>(value);
        }
    }

    return data;
}

template <class DataType>
void fromJson(DataType &value, const std::string &key_name, const nlohmann::json &data)
{
    if (!data.contains(key_name))
    {
        return;
    }

    if constexpr (std::is_arithmetic_v<DataType> || is_string<DataType>::value)
    {
        value = data[key_name];
    }
    else if constexpr (std::is_same_v<DataType, utils::Vector2f> || std::is_same_v<DataType, utils::Vector2i>)
    {
        value = {data[key_name][0], data[key_name][1]};
    }
    else if constexpr (boost::mp11::mp_similar<DataType, Rect<float>>::value)
    {
        value = {data[key_name][0], data[key_name][1], data[key_name][2], data[key_name][3]};
    }
    else if constexpr (std::is_same_v<DataType, SpriteSpec>)
    {
        auto &spec = static_cast<SpriteSpec &>(value);
        fromJson(spec.tex_rect, "tex_rect", data[key_name]);
        fromJson(spec.texture_id, "texture_id", data[key_name]);
        fromJson(spec.tex_size, "tex_size", data[key_name]);
        fromJson(spec.color, "color", data[key_name]);
        fromJson(spec.shader_id, "shader_id", data[key_name]);
    }
    else if constexpr (std::is_enum_v<DataType>)
    {
        if constexpr (boost::describe::has_describe_enumerators<DataType>::value)
        {
            value = stringToEnum<DataType>(data[key_name].get<std::string>());
        }
        else
        {
            value = static_cast<DataType>(data.value<int>(key_name, 0));
        }
    }
}

template <class SpecType,
          class SpecMembersD = boost::describe::describe_members<SpecType, boost::describe::mod_any_access | boost::describe::mod_inherited>>
nlohmann::json serialize(SpecType &spec)
{
    nlohmann::json data;
    boost::mp11::mp_for_each<SpecMembersD>([&](auto spec_member)
                                           { 
                                               using MemberType = std::decay_t<decltype(spec.*spec_member.pointer)>;
                                               if constexpr (boost::describe::has_describe_members<MemberType>::value)
                                               {
                                                   data[spec_member.name] = serialize(spec.*spec_member.pointer);
                                                }
                                                else if constexpr (is_instantiation_of<std::vector, MemberType>::value)
                                                {
                                                    auto& values = spec.*spec_member.pointer;
                                                    data[spec_member.name] = nlohmann::json::array(); 
                                                    for(auto& el : values)
                                                    {
                                                        nlohmann::json el_data = {};
                                                        el_data = serialize(el);
                                                        data[spec_member.name].push_back(el_data);
                                                    }
                                                }
                                                else{
                                                    toJson(spec.*spec_member.pointer, spec_member.name, data); 
                                            } });
    return data;
}

template <class SpecType,
          class SpecMembersD = boost::describe::describe_members<SpecType, boost::describe::mod_any_access | boost::describe::mod_inherited>>
SpecType deserialize(const nlohmann::json &data)
{
    SpecType spec;
    boost::mp11::mp_for_each<SpecMembersD>([&](auto spec_member)
                                           {
                                               if(data.contains(spec_member.name))
                                               {
                                                    using MemberType = std::decay_t<decltype(spec.*spec_member.pointer)>;
                                                    if constexpr (boost::describe::has_describe_members<MemberType>::value)
                                                    {
                                                        spec.*spec_member.pointer = deserialize<MemberType>(data[spec_member.name]);
                                                    }
                                                    else if constexpr(is_instantiation_of<std::vector, MemberType>::value)
                                                    {
                                                        using inner_type = get_inner_t<MemberType>;
                                                         
                                                        const auto& array_json = data[spec_member.name];
                                                        assert(data[spec_member.name].is_array());

                                                        std::vector<inner_type>& vector_values = spec.*spec_member.pointer;
                                                        for(auto& el_json : array_json)
                                                        {
                                                            vector_values.emplace_back(deserialize<inner_type>(el_json));
                                                        }
                                                    }
                                                    else
                                                    {
                                                        fromJson(spec.*spec_member.pointer, spec_member.name, data);                                                
                                                    }
                                               } });
    return spec;
}

/*
#include <any>
#include <typeindex>
struct ComponentI
{
    std::any comp;
    std::type_index id;
};

std::unordered_map<std::type_index, std::function<nlohmann::json(ComponentI&)>> s_comp_serializers;
std::unordered_map<std::type_index, std::function<ComponentI(const nlohmann::json&)>> s_comp_deserializers;

template <class ComponentType>
void registerSerializer(int id)
{
    auto type_id = typeid(ComponentType);
    s_comp_serializers[type_id] = [type_id](const ComponentI& comp_holder){
        assert(type_id == comp_holder.id);
        const auto& the_comp = std::any_cast<ComponentType>(comp_holder.comp);
        return serialize(the_comp);
    };
}

template <class ComponentType>
void registerDeSerializer()
{
    auto type_id = typeid(ComponentType);
    s_comp_deserializers[type_id] = [type_id](const nlohmann::json& json_data){
        ComponentI comp_holder;
        comp_holder.id = type_id;
        comp_holder.comp = deserialize<ComponentType>(json_data);
        return comp_holder;
    };
} */