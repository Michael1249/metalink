# metalink
C++ template based util to easy link objects in runtime  

### Description

metalink provide three classes:

- lnk::require
- lnk::provide
- lnk::System

They are used to describe relations between modules.
Modules here is basic parts of a system, 
each module can provide or request some number of iterfaces, which it depends on.
System agregate and own modules, link them together and provide external access to them and their interfaces.

### Example

Here is basic example of metalink usage.
Imagine we have three Interfaces, three Implementations of these interfaces
and three modules which will agregate some of implementations, provide their interfaces 
and request interfaces of other modules.


```C++
#include <iostream>
#include "metalink/metalink.hpp"

// define some interfaces
struct IA{
    virtual std::string a() = 0;
};
struct IB{
    virtual std::string b() = 0;
};
struct IC{
    virtual std::string c() = 0;
};
```

```C++
//define some implementations
struct A : public IA
{
    std::string a() override {
        return "this is A!\n";
    };
};

struct B : public IB
{
    std::string b() override {
        return "this is B!\n";
    };
};

struct C : public IC
{
    std::string c() override {
        return "this is C!\n";
    };
};
```

```C++
struct System; // System modules which require interfaces should know System type
```

```C++
// Module which only require IA and IC Interfaces
struct Module1
//  : public lnk::require<System> - compile error, no interfaces requested
//  : public lnk::require<IA, IB> - compile error, wrong system type
    : public lnk::require<System, IA, IC> //Ok - IA and IC will be accessable
{
    Module1() // Modules must have default constructor
    {
        //std::cout << access<IA>().a(); - runtime error, modules not linked
        // No access in module constructor
    }

    void foo()
    {
        std::cout << "Access IA from Module1 - " << access<IA>().a();
    }

    void bar()
    {
        std::cout << "Access IC from Module1 - " << access<IC>().c();
    }
};

```


```C++
// Module which only provide IB interface
struct Module2
//  : public lnk::provide<> - compile error, no interfaces provided
    : public lnk::provide<IB> // Ok, Module2 will be marked as IB provider
{
    template<class T = IB>
    T& interface() { return b; } // template of interface method must be defined for providers 

    B b;
};
```

```C++
// Module which require IB and provide IA and IC
struct Module3
    : public lnk::require<System, IB>
//  , public lnk::provide<IB> - actually Ok, but has no sense since Module1 require IB and provide IB
    , public lnk::provide<IA, IC>
{
    void baz()
    {
        std::cout << "Access IB from Module3 - " << access<IB>().b();
    }

    template<class T> T& interface();
    template<> IA& interface() { return a; }
    template<> IC& interface() { return c; }

    A a;
    C c;
};
```

```C++
struct System : public lnk::System<Module1, Module2, Module3>{};
```

```C++
int main(int, char**) {
    System sys;

    // sys.access<IA>().a(); - runtime error, modules not linked.

    sys.linkModules();

    std::cout << "Hello, metalink!\n";

    sys.getModule<Module1>().foo(); // Access IA from Module1 - this is A!
    sys.getModule<Module1>().bar(); // Access IC from Module1 - this is C!
    sys.getModule<Module3>().baz(); // Access IB from Module3 - this is B!
}

```
