#ifndef __METALINK_H__
#define __METALINK_H__

#include <tuple>
#include <type_traits>

namespace lnk::details {

template<class ... Modules>
class System;

class InterfaceAccesorLabel{};
class InterfaceProviderLabel{};

template<class Provider, class ... Interfaces>
class InterfaceAccesor;


template<class T1, class... Ts>
constexpr bool is_one_of() {
    return (std::is_same<T1, Ts>::value || ...);
}

template<class T1, class... Ts>
constexpr bool is_inherit_all() {
    return (std::is_base_of<Ts, T1>::value && ...);
} 

template<bool C, class T1, class T2>
struct alternative;

template<class T1, class T2>
struct alternative<true, T1, T2> {
    using type = T1;
};

template<class T1, class T2>
struct alternative<false, T1, T2> {
    using type = T2;
};


template<class System, class ... Interfaces>
class InterfaceAccesor: InterfaceAccesorLabel {
    static_assert(sizeof...(Interfaces) > 0, "No interfaces requested!");

    template<class T1, class ... Ts, class T2>
    friend void makeLink(T1& provider, T2& accesor);
    
    public:

    template<class Interface>
    inline Interface& access()
    {
        static_assert(is_one_of<Interface, Interfaces...>(), "Not allowed Interface requested!");
        return m_provider->template access<Interface>();
    }

    private:

    template<class T>
    inline void linkTo(T& p) { m_provider = static_cast<System*>(&p); } //compiler can't resolve System, use T and static_cast.

    System* m_provider;
};

template<class ... Interfaces>
class InterfaceProvider: InterfaceProviderLabel {
    static_assert(sizeof...(Interfaces) > 0, "No interfaces provided!");

    template<class Interface, class CurrentProvider, class ... Providers>
    friend struct resolveProvider_impl;

    template<class Interface>
    struct is_lnk_provider_for
    {
        static constexpr bool value = is_one_of<Interface, Interfaces...>(); 
    };
};

template<class Interface, class CurrentProvider, class ... Providers>
struct resolveProvider_impl {
    using result = typename alternative<
        CurrentProvider::template is_lnk_provider_for<Interface>::value, 
        CurrentProvider, 
        typename resolveProvider_impl<Interface, Providers...>::result
    >::type;
};

template<class Interface, class CurrentProvider>
struct resolveProvider_impl<Interface, CurrentProvider> {
    using result = typename alternative<
        CurrentProvider::template is_lnk_provider_for<Interface>::value, 
        CurrentProvider, 
        void // error type, provider not found
    >::type;
};

template<class Provider, class ... Interfaces, class Accesor = InterfaceAccesor<Provider, Interfaces...>>
inline void makeLink(Provider& provider, Accesor& accesor)
{
    accesor.linkTo(provider);
}

template<class ... Ts>
struct AccessorsList
{
    template <class T> 
    using next = AccessorsList<Ts ..., T>;

    template<class Provider, class Tuple>
    inline static void makeLinksTo(Provider& provider, Tuple& tuple)
    {
        (makeLink(provider, std::get<Ts>(tuple)),...);
    }
    
};

template<>
struct AccessorsList<>
{
    template <class T> 
    using next = AccessorsList<T>;
};

template<class ... Ts>
struct ProvidersList
{
    template <class T> 
    using next = ProvidersList<Ts ..., T>;

    template<class Interface>
    using resolveProvider = resolveProvider_impl<Interface, Ts...>;
    
};

template<>
struct ProvidersList<>
{
    template <class T> 
    using next = ProvidersList<T>;    
};

template<class ListA, class ListP, class T, class ... Ts>
struct filterModules_impl {
    using accessors = typename filterModules_impl<typename alternative<
        std::is_base_of<InterfaceAccesorLabel, T>::value,
        typename ListA::template next<T>, 
        ListA
    >::type, ListP,  Ts ...>::accessors;

    using providers = typename filterModules_impl<ListA, typename alternative<
        std::is_base_of<InterfaceProviderLabel, T>::value,
        typename ListP::template next<T>, 
        ListP
    >::type, Ts ...>::providers;
};

template<class ListA, class ListP, class T>
struct filterModules_impl<ListA, ListP, T> {
    using accessors = typename alternative<
        std::is_base_of<InterfaceAccesorLabel, T>::value,
        typename ListA::template next<T>, 
        ListA
    >::type; 

    using providers = typename alternative<
        std::is_base_of<InterfaceProviderLabel, T>::value,
        typename ListP::template next<T>, 
        ListP
    >::type; 
};

template<class ... Ts>
using filterModules = filterModules_impl<AccessorsList<>, ProvidersList<>, Ts ...>;

template<class ... Modules>
class System
{
    public:
    
    void linkModules()
    {
        filterModules<Modules ...>::accessors::makeLinksTo(*this, m_modules);
    }

    template<class T>
    T& getModule()
    {
        return std::get<T>(m_modules);
    }

    template<class Interface>
    inline Interface& access()
    {
        
        using resolvedModule = typename filterModules<Modules ...>::providers::template resolveProvider<Interface>::result;
        return std::get<resolvedModule>(m_modules).template interface<Interface>();
    }

    private:

    std::tuple<Modules...> m_modules;
};

} // lnk::details

namespace lnk {

template<class ... Interfaces>
using provide = details::InterfaceProvider<Interfaces ...>;

template<class Provider, class ... Interfaces>
using require = details::InterfaceAccesor<Provider, Interfaces ...>;

template<class ... Modules>
using System = details::System<Modules...> ;

} // lnk

#endif // __METALINK_H__