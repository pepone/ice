//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

[[suppress-warning(invalid-metadata)]]

[[cpp:header-ext(hh)]]
[[cpp:header-ext(hh)]]

[[cpp:source-ext(cc)]]
[[cpp:source-ext(cc)]]

[[cpp:dll-export(Test)]]
[[cpp:dll-export(Test)]]

[[cpp:header-ext]]
[[cpp:header-ext()]]

[[cpp:source-ext]]
[[cpp:source-ext()]]

[[cpp:dll-export]]
[[cpp:dll-export()]]

[[cpp:include]]
[[cpp:include()]]

module Test
{

interface I
{
    [cpp:foo]
    void op();

    [cpp:type(std::list< ::std::string>)]
    void op1();

    [cpp:view-type(std::experimental::string_view)]
    void op2();

    [cpp:array]
    void op3();

    [cpp:range]
    void op4();
}

[cpp:foo] [cpp:bar]
class C
{
}

[cpp:const] [cpp:ice_print]
struct S
{
    int i;
}

[cpp:virtual]
exception E
{
}

}
