// Copyright (c) ZeroC, Inc.
[["suppress-warning:deprecated"]]

module and
{

enum continue
{
    let,
    var
}

struct guard
{
    int default;
}

struct defer
{
    string else;
}

interface break
{
    ["amd"] void case(int catch, out int try);
}

interface func
{
    void public();
}

class switch
{
    int if;
    func* export;
    int volatile;
}

interface do extends func, break
{
}

sequence<guard> fileprivate;

dictionary<string,guard> for;

exception return
{
    int Int32;
}

exception as extends return
{
    int static; int switch;
}

/* TODO: reenable when #1617 is fixed
interface friend
{
    guard goto(continue if, guard d, defer inline, switch private, do mutable, break* namespace,
              func* new, switch* not, do* operator, int or, int protected, int public, int register)
        throws return, as;
}
*/

const int is = 0;
const int self = 0;
const int throw = 0;
const int typealias = 0;
const int internal = 0;
const int while = 0;
const int import = 0;

}
