#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <tuple>
#include <type_traits>
#include <map>
#include <folly/dynamic.h>
#include <folly/json.h>

template<typename Class, typename T>
struct PropertyImpl{
    constexpr PropertyImpl(T Class::* member, const char* name):member_(member), name_(name){
    }
    using Type = T;
        
    T Class::* member_;
    const char* name_;
};


template<typename Class, typename T>
constexpr auto property(T Class::* member, const char* name){
    return PropertyImpl<Class, T>(member, name);
}

#define PROPERTY(CLASS, MEMBER) property(&CLASS::MEMBER, #MEMBER)
#define PROPERTIES1(CLASS, MEMBER1) constexpr static auto PROPERTIES = std::make_tuple(PROPERTY(CLASS, MEMBER1))
#define PROPERTIES2(CLASS, MEMBER1, MEMBER2) constexpr static auto PROPERTIES = std::make_tuple(PROPERTY(CLASS, MEMBER1), PROPERTY(CLASS, MEMBER2))
#define PROPERTIES3(CLASS, MEMBER1, MEMBER2, MEMBER3) constexpr static auto PROPERTIES = std::make_tuple(PROPERTY(CLASS, MEMBER1), PROPERTY(CLASS, MEMBER2), PROPERTY(CLASS, MEMBER3))
#define PROPERTIES4(CLASS, MEMBER1, MEMBER2, MEMBER3, MEMBER4) constexpr static auto PROPERTIES = std::make_tuple(PROPERTY(CLASS, MEMBER1), PROPERTY(CLASS, MEMBER2), PROPERTY(CLASS, MEMBER3), PROPERTY(CLASS, MEMBER4))

template< class T >
struct remove_cvref {
    using type = typename std::remove_cv<std::remove_reference_t<T>>::type;
};

template <typename T, T... S, typename F>
constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f) {
    (void)std::vector<bool>{(static_cast<void>(f(std::integral_constant<T, S>{})), true)..., true};
}

template<typename T>
struct HAS_PROPERTIES
{
private:
    using Yes = std::true_type;
    using No  = std::false_type;
 
    template<typename U> static auto test(int) -> decltype((static_cast<void>(U::PROPERTIES), true), Yes());
    template<typename> static No test(...);
public:
    static constexpr bool value = std::is_same<decltype(test<T>(0)),Yes>::value;
};

namespace impl{

template<typename T, typename Enable = void>
struct dynamic_converter_impl{
};

template<>
struct dynamic_converter_impl<std::string>{
    static void to(folly::dynamic& output, const std::string& input){
        output = input;
    }
    static void to(folly::dynamic& output, std::string&& input){
        output = std::move(input);
    }
    static void from(std::string& output, const folly::dynamic& input){
        output = input.getString();
    }
    static void from(std::string& output, folly::dynamic&& input){
        output = std::move(input).getString();
    }
};

template<typename T>
struct dynamic_converter_impl<T, typename std::enable_if<std::is_floating_point<T>::value, void>::type>{
    static void to(folly::dynamic& output, T input){
        output = input;
    }
    static void to(folly::dynamic& output, T&& input){
        output = std::move(input);
    }
    static void from(T output, const folly::dynamic& input){
        output = input.asDouble();
    }
    static void from(T output, folly::dynamic&& input){
        output = std::move(input).getDouble();
    }
};

template<typename T>
struct dynamic_converter_impl<T, typename std::enable_if<std::is_integral<T>::value, void>::type>{
    static void to(folly::dynamic& output, T input){
        output = input;
    }
    static void to(folly::dynamic& output, T&& input){
        output = std::move(input);
    }
    static void from(T output, const folly::dynamic& input){
        output = input.asInt();
    }
    static void from(T output, folly::dynamic&& input){
        output = std::move(input).getInt();
    }
};


template<typename T>
struct dynamic_converter_impl<std::vector<T>>{
    static void to(folly::dynamic& output, const std::vector<T>& input){
        output = folly::dynamic::array;
        for (const auto& data : input){
            folly::dynamic value(folly::dynamic::object);
            impl::dynamic_converter_impl<T>::to(value, data);
            output.push_back(std::move(value));            
        }
    }
    static void to(folly::dynamic& output, std::vector<T>&& input){
        output = folly::dynamic::array;
        for (auto& data : input){
            folly::dynamic value(folly::dynamic::object);
            impl::dynamic_converter_impl<T>::to(value, std::move(data));
            output.push_back(std::move(value));            
        }
    }
    static void from(std::vector<T>& output, const folly::dynamic& input){
        for (const auto& data : input){
            output.emplace_back();
            impl::dynamic_converter_impl<T>::from(output.back(), data);
        }
    }
    static void from(std::vector<T>& output, folly::dynamic&& input){
        for (auto& data : input){
            output.emplace_back();
            impl::dynamic_converter_impl<T>::from(output.back(), std::move(data));
        }
    }
};

template<typename T>
struct dynamic_converter_impl<std::list<T>>{
    static void to(folly::dynamic& output, const std::list<T>& input){
        output = folly::dynamic::array;
        for (const auto& data : input){
            folly::dynamic value(folly::dynamic::object);
            impl::dynamic_converter_impl<T>::to(value, data);
            output.push_back(std::move(value));            
        }
    }
    static void to(folly::dynamic& output, std::list<T>&& input){
        output = folly::dynamic::array;
        for (auto& data : input){
            folly::dynamic value(folly::dynamic::object);
            impl::dynamic_converter_impl<T>::to(value, std::move(data));
            output.push_back(std::move(value));            
        }
    }
    static void from(std::list<T>& output, const folly::dynamic& input){
        for (const auto& data : input){
            output.emplace_back();
            impl::dynamic_converter_impl<T>::from(output.back(), data);
        }
    }
    static void from(std::list<T>& output, folly::dynamic&& input){
        for (auto& data : input){
            output.emplace_back();
            impl::dynamic_converter_impl<T>::from(output.back(), std::move(data));
        }
    }
};

template<typename K, typename V>
struct dynamic_converter_impl<std::unordered_map<K,V>>{
    static void to(folly::dynamic& output, const std::unordered_map<K,V>& input){
        output = folly::dynamic::object;
        for (const auto& data : input){
            impl::dynamic_converter_impl<V>::to(output[data.first], data.second);
        }
    }
    static void to(folly::dynamic& output, std::unordered_map<K,V>&& input){
        output = folly::dynamic::object;
        for (auto& data : input){
            impl::dynamic_converter_impl<V>::to(output[std::move(data.first)], std::move(data.second));
        }
    }
    static void from(std::unordered_map<K,V>& output, const folly::dynamic& input){
        for (const auto& data : input.items()){
            K key;
            impl::dynamic_converter_impl<K>::from(key, data.first);
            V value;
            impl::dynamic_converter_impl<V>::from(value, data.second);
            output.emplace(std::move(key), std::move(value));
        }
    }
    static void from(std::unordered_map<K,V>& output, folly::dynamic&& input){
        for (auto& data : input.items()){
            K key;
            impl::dynamic_converter_impl<K>::from(key, std::move(data.first));
            V value;
            impl::dynamic_converter_impl<V>::from(value, std::move(data.second));
            output.emplace(std::move(key), std::move(value));
        }
    }
};

template<typename K, typename V>
struct dynamic_converter_impl<std::map<K,V>>{
    static void to(folly::dynamic& output, const std::map<K,V>& input){
        output = folly::dynamic::object;
        for (const auto& data : input){
            impl::dynamic_converter_impl<V>::to(output[data.first], data.second);
        }
    }
    static void to(folly::dynamic& output, std::map<K,V>&& input){
        output = folly::dynamic::object;
        for (auto& data : input){
            impl::dynamic_converter_impl<V>::to(output[std::move(data.first)], std::move(data.second));
        }
    }
    static void from(std::map<K,V>& output, const folly::dynamic& input){
        for (const auto& data : input.items()){
            K key;
            impl::dynamic_converter_impl<K>::from(key, data.first);
            V value;
            impl::dynamic_converter_impl<V>::from(value, data.second);
            output.emplace(std::move(key), std::move(value));
        }
    }
    static void from(std::map<K,V>& output, folly::dynamic&& input){
        for (auto& data : input.items()){
            K key;
            impl::dynamic_converter_impl<K>::from(key, std::move(data.first));
            V value;
            impl::dynamic_converter_impl<V>::from(value, std::move(data.second));
            output.emplace(std::move(key), std::move(value));
        }
    }
};


template<typename T>
struct dynamic_converter_impl<T, typename std::enable_if<std::is_class<T>::value && HAS_PROPERTIES<T>::value, void>::type>{
    static void to(folly::dynamic& output, const T& input){
        output = folly::dynamic::object;
        constexpr auto nbProperties = std::tuple_size<decltype(T::PROPERTIES)>::value;
        for_sequence(std::make_index_sequence<nbProperties>{}, [&](auto i) {
            constexpr auto property = std::get<i>(T::PROPERTIES);
            using Type = typename decltype(property)::Type;
            impl::dynamic_converter_impl<Type>::to(output[property.name_], input.*(property.member_));
        });
    }
    static void to(folly::dynamic& output, T&& input){
        output = folly::dynamic::object;
        constexpr auto nbProperties = std::tuple_size<decltype(T::PROPERTIES)>::value;
        for_sequence(std::make_index_sequence<nbProperties>{}, [&](auto i) {
            constexpr auto property = std::get<i>(T::PROPERTIES);
            using Type = typename decltype(property)::Type;
            impl::dynamic_converter_impl<Type>::to(output[property.name_], input.*(property.member_));
        });
    }
    static void from(T& output, const folly::dynamic& input){
        constexpr auto nbProperties = std::tuple_size<decltype(T::PROPERTIES)>::value;
        for_sequence(std::make_index_sequence<nbProperties>{}, [&](auto i) {
            constexpr auto property = std::get<i>(T::PROPERTIES);
            using Type = typename decltype(property)::Type;
            if (input.count(property.name_) > 0){
                return;
            }
            impl::dynamic_converter_impl<Type>::from(output.*(property.member_), input[property.name_]);
        });
    }
    static void from(T& output, folly::dynamic&& input){
        constexpr auto nbProperties = std::tuple_size<decltype(T::PROPERTIES)>::value;
        for_sequence(std::make_index_sequence<nbProperties>{}, [&](auto i) {
            constexpr auto property = std::get<i>(T::PROPERTIES);
            using Type = typename decltype(property)::Type;
            if (input.count(property.name_) == 0){
                return;
            }
            impl::dynamic_converter_impl<Type>::from(output.*(property.member_), std::move(input[property.name_]));
        });
    }
};

}

template<typename T>
static T from_dynamic(const folly::dynamic& data){
    T value;    
    impl::dynamic_converter_impl<typename remove_cvref<T>::type>::from(value, data);
    return value;
}

template<typename T>
static T from_dynamic(folly::dynamic&& data){
    T value;    
    impl::dynamic_converter_impl<typename remove_cvref<T>::type>::from(value, std::move(data));
    return value;
}

template<typename T>
static folly::dynamic to_dynamic(const T& data){
    folly::dynamic value;    
    impl::dynamic_converter_impl<typename remove_cvref<T>::type>::to(value, data);
    return value;
}

template<typename T>
static folly::dynamic to_dynamic(T&& data){
    folly::dynamic value;    
    impl::dynamic_converter_impl<typename remove_cvref<T>::type>::to(value, std::move(data));
    return value;
}


struct Person2{
    std::string name;
    int age = 100;
    double money = 3.1415926;
    std::vector<std::string> telphones;
    PROPERTIES4(Person2, name, age, telphones, money);
};

struct Person{
    std::string name;
    int age = 100;
    std::vector<std::string> telphones;
    std::unordered_map<std::string, Person2> relationships;

    PROPERTIES4(Person, name, age, telphones, relationships);
};


int main(int argc, char** argv){
    Person p;
    p.name = "name", 
    p.telphones = {"123","456", "789"};

    Person2 father;
    father.name = "父亲";
    father.telphones = {"abc","efg", "xyz"};

    Person2 mother;
    mother.name = "母亲";
    mother.telphones = {"111","222", "333"};

    p.relationships = { {"父亲", father}, {"母亲",mother}};
    auto d = to_dynamic(p);
    std::string output1 = folly::toJson(d);
    std::cout<<output1<<std::endl;

    Person p2 = from_dynamic<Person>(std::move(d));
    auto d2 = to_dynamic(p2);
    std::string output2 = folly::toJson(d2);
    std::cout<<output2<<std::endl;
    std::string output3 = folly::toJson(d);
    std::cout<<output3<<std::endl;
}
