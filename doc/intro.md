Intro
=====

Welcome to our documentation!

#### First, what is kangaru?

Kangaru is an inversion of control container for C++11 and later. C++14 features like generic lambdas are supported. Our goal is to create a container capable of automatic dependency injection that do most diagnostics at compile time, while keeping the simplest interface possible, and all that without modifying existing classes. Kangaru is a header only library because of it's extensive use of templates. The name kangaru comes from the container's feature that consists in injecting itself into a service as a dependency.

#### Inversion of control that puts you back in control

Indeed, the container does not impose a way to construct, allocate memory, contain or inject your classes. You are in full control of everything related to the classes of your project.

Getting Started
---------------

Getting started with kangaru is easy. First of all, you need to include the library:

    #include <kangaru/kangaru.hpp>

Take note that you will need to add the library to your include paths.

All declarations are made in the namespace `kgr`. Additionnaly, the namespace `kgr` contains the namespace `detail`, which itself contains implementation details.
Note that the `detail` namespace is not considered as a part of the API and its content might be subject to changes.

### Wording

In this documentation, many classes will be refered as services or service definitions.

_Services_ are classes that are either injected by the container or have other classes as dependencies.

_Service Definitions_ are classes that contain a service and tell the container how this particular service should behave within the container.

### Macros

This library does not make use of macros to prevent multiple inclusion.
Every macros that starts with `KGR_KANGARU_` is considered reserved.
Note that some features of this library are easier to use with macros, and we recommend you to use those that are defined in the documentation.

Macros defined by the library are not part of it's interface.

### Compiler Support

 - MSVC: 2015 update 3 or better
 - GCC: 5.1 or better
 - Clang: 3.5 or better

Index
-----
 * [Services](section1_services.md)
 * [Basic usage of the container](section2_container.md)
 * [Override Services](section3_override.md)
 * [Invoke](section4_invoke.md)
 * [Operator services](section5_operator.md)
 * [Autocall](section6_setters.md)
 * [Custom service definitions](section7_definitions.md)
 * [Debugging](section8_debug.md)
 * [Generic Services](section9_generic.md)
 * [Structure for large projects](section10_structure.md)
