//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "CsUtil.h"
#include "../Slice/Util.h"
#include "DotNetNames.h"
#include "IceUtil/StringUtil.h"

#include <algorithm>
#include <cassert>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#    include <direct.h>
#else
#    include <unistd.h>
#endif

using namespace std;
using namespace Slice;
using namespace IceUtil;
using namespace IceUtilInternal;

namespace
{
    string lookupKwd(const string& name, unsigned int baseTypes, bool mangleCasts = false)
    {
        //
        // Keyword list. *Must* be kept in alphabetical order.
        //
        static const string keywordList[] = {
            "abstract", "as",        "async",     "await",      "base",      "bool",     "break",    "byte",
            "case",     "catch",     "char",      "checked",    "class",     "const",    "continue", "decimal",
            "default",  "delegate",  "do",        "double",     "else",      "enum",     "event",    "explicit",
            "extern",   "false",     "finally",   "fixed",      "float",     "for",      "foreach",  "goto",
            "if",       "implicit",  "in",        "int",        "interface", "internal", "is",       "lock",
            "long",     "namespace", "new",       "null",       "object",    "operator", "out",      "override",
            "params",   "private",   "protected", "public",     "readonly",  "ref",      "return",   "sbyte",
            "sealed",   "short",     "sizeof",    "stackalloc", "static",    "string",   "struct",   "switch",
            "this",     "throw",     "true",      "try",        "typeof",    "uint",     "ulong",    "unchecked",
            "unsafe",   "ushort",    "using",     "virtual",    "void",      "volatile", "while"};
        bool found = binary_search(
            &keywordList[0],
            &keywordList[sizeof(keywordList) / sizeof(*keywordList)],
            name,
            Slice::CICompare());
        if (found)
        {
            return "@" + name;
        }
        if (mangleCasts && (name == "checkedCast" || name == "uncheckedCast"))
        {
            return string(DotNet::manglePrefix) + name;
        }
        return Slice::DotNet::mangleName(name, baseTypes);
    }
}

string
Slice::CsGenerator::getNamespacePrefix(const ContainedPtr& cont)
{
    //
    // Traverse to the top-level module.
    //
    ModulePtr m;
    ContainedPtr p = cont;
    while (true)
    {
        if (dynamic_pointer_cast<Module>(p))
        {
            m = dynamic_pointer_cast<Module>(p);
        }

        ContainerPtr c = p->container();
        p = dynamic_pointer_cast<Contained>(c); // This cast fails for Unit.
        if (!p)
        {
            break;
        }
    }

    assert(m);

    static const string prefix = "cs:namespace:";

    string q;
    if (m->findMetaData(prefix, q))
    {
        q = q.substr(prefix.size());
    }
    return q;
}

string
Slice::CsGenerator::getNamespace(const ContainedPtr& cont)
{
    string scope = fixId(cont->scope());
    if (scope.rfind(".") == scope.size() - 1)
    {
        scope = scope.substr(0, scope.size() - 1);
    }
    string prefix = getNamespacePrefix(cont);
    if (!prefix.empty())
    {
        if (!scope.empty())
        {
            return prefix + "." + scope;
        }
        else
        {
            return prefix;
        }
    }

    return scope;
}

string
Slice::CsGenerator::getUnqualified(const string& type, const string& scope, bool builtin)
{
    if (type.find(".") != string::npos && type.find(scope) == 0 && type.find(".", scope.size() + 1) == string::npos)
    {
        return type.substr(scope.size() + 1);
    }
    else if (builtin)
    {
        return type.find(".") == string::npos ? type : "global::" + type;
    }
    else
    {
        return "global::" + type;
    }
}

string
Slice::CsGenerator::getUnqualified(
    const ContainedPtr& p,
    const string& package,
    const string& prefix,
    const string& suffix)
{
    string name = fixId(prefix + p->name() + suffix);
    string contPkg = getNamespace(p);
    if (contPkg == package || contPkg.empty())
    {
        return name;
    }
    else
    {
        return "global::" + contPkg + "." + name;
    }
}

//
// If the passed name is a scoped name, return the identical scoped name,
// but with all components that are C# keywords replaced by
// their "@"-prefixed version; otherwise, if the passed name is
// not scoped, but a C# keyword, return the "@"-prefixed name;
// otherwise, check if the name is one of the method names of baseTypes;
// if so, prefix it with ice_; otherwise, return the name unchanged.
//
string
Slice::CsGenerator::fixId(const string& name, unsigned int baseTypes, bool mangleCasts)
{
    if (name.empty())
    {
        return name;
    }
    if (name[0] != ':')
    {
        return lookupKwd(name, baseTypes, mangleCasts);
    }
    vector<string> ids = splitScopedName(name);
    vector<string> newIds;
    for (vector<string>::const_iterator i = ids.begin(); i != ids.end(); ++i)
    {
        newIds.push_back(lookupKwd(*i, baseTypes));
    }
    stringstream result;
    for (vector<string>::const_iterator j = newIds.begin(); j != newIds.end(); ++j)
    {
        if (j != newIds.begin())
        {
            result << '.';
        }
        result << *j;
    }
    return result.str();
}

string
Slice::CsGenerator::getOptionalFormat(const TypePtr& type)
{
    BuiltinPtr bp = dynamic_pointer_cast<Builtin>(type);
    string prefix = "Ice.OptionalFormat";
    if (bp)
    {
        switch (bp->kind())
        {
            case Builtin::KindByte:
            case Builtin::KindBool:
            {
                return prefix + ".F1";
            }
            case Builtin::KindShort:
            {
                return prefix + ".F2";
            }
            case Builtin::KindInt:
            case Builtin::KindFloat:
            {
                return prefix + ".F4";
            }
            case Builtin::KindLong:
            case Builtin::KindDouble:
            {
                return prefix + ".F8";
            }
            case Builtin::KindString:
            {
                return prefix + ".VSize";
            }
            case Builtin::KindObject:
            {
                return prefix + ".Class";
            }
            case Builtin::KindObjectProxy:
            {
                return prefix + ".FSize";
            }
            case Builtin::KindValue:
            {
                return prefix + ".Class";
            }
        }
    }

    if (dynamic_pointer_cast<Enum>(type))
    {
        return prefix + ".Size";
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        if (seq->type()->isVariableLength())
        {
            return prefix + ".FSize";
        }
        else
        {
            return prefix + ".VSize";
        }
    }

    DictionaryPtr d = dynamic_pointer_cast<Dictionary>(type);
    if (d)
    {
        if (d->keyType()->isVariableLength() || d->valueType()->isVariableLength())
        {
            return prefix + ".FSize";
        }
        else
        {
            return prefix + ".VSize";
        }
    }

    StructPtr st = dynamic_pointer_cast<Struct>(type);
    if (st)
    {
        if (st->isVariableLength())
        {
            return prefix + ".FSize";
        }
        else
        {
            return prefix + ".VSize";
        }
    }

    if (dynamic_pointer_cast<InterfaceDecl>(type))
    {
        return prefix + ".FSize";
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    assert(cl);
    return prefix + ".Class";
}

string
Slice::CsGenerator::getStaticId(const TypePtr& type)
{
    BuiltinPtr b = dynamic_pointer_cast<Builtin>(type);
    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);

    assert((b && b->usesClasses()) || cl);

    if (b)
    {
        return "Ice.Value.ice_staticId()";
    }
    else
    {
        return getUnqualified(cl) + ".ice_staticId()";
    }
}

string
Slice::CsGenerator::typeToString(const TypePtr& type, const string& package, bool optional)
{
    if (!type)
    {
        return "void";
    }

    if (optional && !isProxyType(type))
    {
        // Proxy types are mapped the same way for optional and non-optional types.
        return typeToString(type, package) + "?";
    }
    // else, just use the regular mapping. null represents "not set",

    static const char* builtinTable[] = {
        "byte",
        "bool",
        "short",
        "int",
        "long",
        "float",
        "double",
        "string",
        "Ice.Object", // not used anymore
        "Ice.ObjectPrx?",
        "Ice.Value?"};

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        if (builtin->kind() == Builtin::KindObject)
        {
            return getUnqualified(builtinTable[Builtin::KindValue], package, true);
        }
        else
        {
            return getUnqualified(builtinTable[builtin->kind()], package, true);
        }
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        return getUnqualified(cl, package) + "?";
    }

    InterfaceDeclPtr proxy = dynamic_pointer_cast<InterfaceDecl>(type);
    if (proxy)
    {
        return getUnqualified(proxy, package, "", "Prx?");
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        string prefix = "cs:generic:";
        string meta;
        if (seq->findMetaData(prefix, meta))
        {
            string customType = meta.substr(prefix.size());
            if (customType == "List" || customType == "LinkedList" || customType == "Queue" || customType == "Stack")
            {
                return "global::System.Collections.Generic." + customType + "<" + typeToString(seq->type(), package) +
                       ">";
            }
            else
            {
                return "global::" + customType + "<" + typeToString(seq->type(), package) + ">";
            }
        }

        return typeToString(seq->type(), package) + "[]";
    }

    DictionaryPtr d = dynamic_pointer_cast<Dictionary>(type);
    if (d)
    {
        string prefix = "cs:generic:";
        string meta;
        string typeName;
        if (d->findMetaData(prefix, meta))
        {
            typeName = meta.substr(prefix.size());
        }
        else
        {
            typeName = "Dictionary";
        }
        return "global::System.Collections.Generic." + typeName + "<" + typeToString(d->keyType(), package) + ", " +
               typeToString(d->valueType(), package) + ">";
    }

    ContainedPtr contained = dynamic_pointer_cast<Contained>(type);
    if (contained)
    {
        return getUnqualified(contained, package);
    }

    return "???";
}

string
Slice::CsGenerator::resultStructName(const string& className, const string& opName, bool marshaledResult)
{
    ostringstream s;
    s << className << "_" << IceUtilInternal::toUpper(opName.substr(0, 1)) << opName.substr(1)
      << (marshaledResult ? "MarshaledResult" : "Result");
    return s.str();
}

string
Slice::CsGenerator::resultType(const OperationPtr& op, const string& package, bool dispatch)
{
    InterfaceDefPtr interface = op->interface();
    if (dispatch && op->hasMarshaledResult())
    {
        return getUnqualified(interface, package, "", resultStructName("", op->name(), true));
    }

    string t;
    ParamDeclList outParams = op->outParameters();
    if (op->returnType() || !outParams.empty())
    {
        if (outParams.empty())
        {
            t = typeToString(op->returnType(), package, op->returnIsOptional());
        }
        else if (op->returnType() || outParams.size() > 1)
        {
            t = getUnqualified(interface, package, "", resultStructName("", op->name()));
        }
        else
        {
            t = typeToString(outParams.front()->type(), package, outParams.front()->optional());
        }
    }

    return t;
}

string
Slice::CsGenerator::taskResultType(const OperationPtr& op, const string& scope, bool dispatch)
{
    string t = resultType(op, scope, dispatch);
    if (t.empty())
    {
        return "global::System.Threading.Tasks.Task";
    }
    else
    {
        return "global::System.Threading.Tasks.Task<" + t + '>';
    }
}

bool
Slice::CsGenerator::isClassType(const TypePtr& type)
{
    if (dynamic_pointer_cast<ClassDecl>(type))
    {
        return true;
    }
    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    return builtin && (builtin->kind() == Builtin::KindObject || builtin->kind() == Builtin::KindValue);
}

bool
Slice::CsGenerator::isValueType(const TypePtr& type)
{
    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindString:
            case Builtin::KindObject:
            case Builtin::KindObjectProxy:
            case Builtin::KindValue:
            {
                return false;
                break;
            }
            default:
            {
                return true;
                break;
            }
        }
    }
    StructPtr s = dynamic_pointer_cast<Struct>(type);
    if (s)
    {
        if (s->hasMetaData("cs:class"))
        {
            return false;
        }
        DataMemberList dm = s->dataMembers();
        for (DataMemberList::const_iterator i = dm.begin(); i != dm.end(); ++i)
        {
            if (!isValueType((*i)->type()) || (*i)->defaultValueType())
            {
                return false;
            }
        }
        return true;
    }
    if (dynamic_pointer_cast<Enum>(type))
    {
        return true;
    }
    return false;
}

bool
Slice::CsGenerator::isNonNullableReferenceType(const TypePtr& p, bool includeString)
{
    if (includeString)
    {
        BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(p);
        if (builtin)
        {
            return builtin->kind() == Builtin::KindString;
        }
    }

    StructPtr st = dynamic_pointer_cast<Struct>(p);
    if (st)
    {
        return isMappedToClass(st);
    }

    return dynamic_pointer_cast<Sequence>(p) || dynamic_pointer_cast<Dictionary>(p);
}

void
Slice::CsGenerator::writeMarshalUnmarshalCode(
    Output& out,
    const TypePtr& type,
    const string& package,
    const string& param,
    bool marshal,
    const string& customStream)
{
    string stream = customStream;
    if (stream.empty())
    {
        stream = marshal ? "ostr" : "istr";
    }

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindByte:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeByte(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readByte()" << ';';
                }
                break;
            }
            case Builtin::KindBool:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeBool(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readBool()" << ';';
                }
                break;
            }
            case Builtin::KindShort:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeShort(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readShort()" << ';';
                }
                break;
            }
            case Builtin::KindInt:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeInt(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readInt()" << ';';
                }
                break;
            }
            case Builtin::KindLong:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeLong(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readLong()" << ';';
                }
                break;
            }
            case Builtin::KindFloat:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeFloat(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readFloat()" << ';';
                }
                break;
            }
            case Builtin::KindDouble:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeDouble(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readDouble()" << ';';
                }
                break;
            }
            case Builtin::KindString:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeString(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readString()" << ';';
                }
                break;
            }
            case Builtin::KindObject:
            case Builtin::KindValue:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeValue(" << param << ");";
                }
                else
                {
                    out << nl << stream << ".readValue(" << param << ");";
                }
                break;
            }
            case Builtin::KindObjectProxy:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeProxy(" << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readProxy()" << ';';
                }
                break;
            }
        }
        return;
    }

    InterfaceDeclPtr prx = dynamic_pointer_cast<InterfaceDecl>(type);
    if (prx)
    {
        string typeS = typeToString(type, package);
        string helperName = typeS.substr(0, typeS.size() - 1) + "Helper"; // remove the trailing '?'
        if (marshal)
        {
            out << nl << helperName << ".write(" << stream << ", " << param << ");";
        }
        else
        {
            out << nl << param << " = " << helperName << ".read(" << stream << ");";
        }
        return;
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        if (marshal)
        {
            out << nl << stream << ".writeValue(" << param << ");";
        }
        else
        {
            out << nl << stream << ".readValue(" << param << ");";
        }
        return;
    }

    StructPtr st = dynamic_pointer_cast<Struct>(type);
    if (st)
    {
        if (marshal)
        {
            if (isMappedToClass(st))
            {
                out << nl << typeToString(st, package) << ".ice_write(" << stream << ", " << param << ");";
            }
            else
            {
                out << nl << param << ".ice_writeMembers(" << stream << ");";
            }
        }
        else
        {
            out << nl << param << " = new " << typeToString(type, package) << "(" << stream << ");";
        }
        return;
    }

    EnumPtr en = dynamic_pointer_cast<Enum>(type);
    if (en)
    {
        if (marshal)
        {
            out << nl << stream << ".writeEnum((int)" << param << ", " << en->maxValue() << ");";
        }
        else
        {
            out << nl << param << " = (" << typeToString(type, package) << ')' << stream << ".readEnum("
                << en->maxValue() << ");";
        }
        return;
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        writeSequenceMarshalUnmarshalCode(out, seq, package, param, marshal, true, stream);
        return;
    }

    assert(dynamic_pointer_cast<Constructed>(type));
    string helperName;
    DictionaryPtr d = dynamic_pointer_cast<Dictionary>(type);
    if (d)
    {
        helperName = getUnqualified(d, package, "", "Helper");
    }
    else
    {
        helperName = typeToString(type, package) + "Helper";
    }

    if (marshal)
    {
        out << nl << helperName << ".write(" << stream << ", " << param << ");";
    }
    else
    {
        out << nl << param << " = " << helperName << ".read(" << stream << ')' << ';';
    }
}

void
Slice::CsGenerator::writeOptionalMarshalUnmarshalCode(
    Output& out,
    const TypePtr& type,
    const string& scope,
    const string& param,
    int tag,
    bool marshal,
    const string& customStream)
{
    string stream = customStream;
    if (stream.empty())
    {
        stream = marshal ? "ostr" : "istr";
    }

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindByte:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeByte(" << tag << ", " << param << ");";
                }
                else
                {
                    //
                    // BUGFIX: with .NET Core reading the byte optional directly in the
                    // result struct can fails unexpectly with optimized builds.
                    //
                    if (param.find(".") != string::npos)
                    {
                        out << sb;
                        out << nl << "var tmp = " << stream << ".readByte(" << tag << ");";
                        out << nl << param << " = tmp;";
                        out << eb;
                    }
                    else
                    {
                        out << nl << param << " = " << stream << ".readByte(" << tag << ");";
                    }
                }
                break;
            }
            case Builtin::KindBool:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeBool(" << tag << ", " << param << ");";
                }
                else
                {
                    //
                    // BUGFIX: with .NET Core reading the bool optional directly in the
                    // result struct fails unexpectly with optimized builds.
                    //
                    if (param.find(".") != string::npos)
                    {
                        out << sb;
                        out << nl << "var tmp = " << stream << ".readBool(" << tag << ");";
                        out << nl << param << " = tmp;";
                        out << eb;
                    }
                    else
                    {
                        out << nl << param << " = " << stream << ".readBool(" << tag << ");";
                    }
                }
                break;
            }
            case Builtin::KindShort:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeShort(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readShort(" << tag << ");";
                }
                break;
            }
            case Builtin::KindInt:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeInt(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readInt(" << tag << ");";
                }
                break;
            }
            case Builtin::KindLong:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeLong(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readLong(" << tag << ");";
                }
                break;
            }
            case Builtin::KindFloat:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeFloat(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readFloat(" << tag << ");";
                }
                break;
            }
            case Builtin::KindDouble:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeDouble(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readDouble(" << tag << ");";
                }
                break;
            }
            case Builtin::KindString:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeString(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readString(" << tag << ");";
                }
                break;
            }
            case Builtin::KindObject:
            case Builtin::KindValue:
            {
                if (marshal)
                {
                    out << nl << stream << ".writeValue(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << stream << ".readValue(" << tag << ", " << param << ");";
                }
                break;
            }
            case Builtin::KindObjectProxy:
            {
                string typeS = typeToString(type, scope);
                if (marshal)
                {
                    out << nl << stream << ".writeProxy(" << tag << ", " << param << ");";
                }
                else
                {
                    out << nl << param << " = " << stream << ".readProxy(" << tag << ");";
                }
                break;
            }
        }
        return;
    }

    InterfaceDeclPtr prx = dynamic_pointer_cast<InterfaceDecl>(type);
    if (prx)
    {
        if (marshal)
        {
            out << nl << "if (" << param << " is not null && " << stream << ".writeOptional(" << tag
                << ", Ice.OptionalFormat.FSize))";
            out << sb;
            out << nl << "int pos = " << stream << ".startSize();";
            writeMarshalUnmarshalCode(out, type, scope, param, marshal, customStream);
            out << nl << stream << ".endSize(pos);";
            out << eb;
        }
        else
        {
            out << nl << "if (" << stream << ".readOptional(" << tag << ", Ice.OptionalFormat.FSize))";
            out << sb;
            out << nl << stream << ".skip(4);";
            string tmp = "tmpVal";
            string typeS = typeToString(type, scope);
            out << nl << typeS << ' ' << tmp << ';';
            writeMarshalUnmarshalCode(out, type, scope, tmp, marshal, customStream);
            out << nl << param << " = " << tmp << ";";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << param << " = null;";
            out << eb;
        }
        return;
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        if (marshal)
        {
            out << nl << stream << ".writeValue(" << tag << ", " << param << ");";
        }
        else
        {
            out << nl << stream << ".readValue(" << tag << ", " << param << ");";
        }
        return;
    }

    StructPtr st = dynamic_pointer_cast<Struct>(type);
    if (st)
    {
        if (marshal)
        {
            out << nl << "if (" << param << " is not null && " << stream << ".writeOptional(" << tag << ", "
                << getOptionalFormat(st) << "))";
            out << sb;
            if (st->isVariableLength())
            {
                out << nl << "int pos = " << stream << ".startSize();";
            }
            else
            {
                out << nl << stream << ".writeSize(" << st->minWireSize() << ");";
            }

            writeMarshalUnmarshalCode(
                out,
                type,
                scope,
                isMappedToClass(st) ? param : param + ".Value",
                marshal,
                customStream);
            if (st->isVariableLength())
            {
                out << nl << stream << ".endSize(pos);";
            }
            out << eb;
        }
        else
        {
            out << nl << "if (" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(st) << "))";
            out << sb;
            if (st->isVariableLength())
            {
                out << nl << stream << ".skip(4);";
            }
            else
            {
                out << nl << stream << ".skipSize();";
            }
            string typeS = typeToString(type, scope);
            string tmp = "tmpVal";
            out << nl << typeS << ' ' << tmp << ";";
            writeMarshalUnmarshalCode(out, type, scope, tmp, marshal, customStream);
            out << nl << param << " = " << tmp << ";";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << param << " = null;";
            out << eb;
        }
        return;
    }

    EnumPtr en = dynamic_pointer_cast<Enum>(type);
    if (en)
    {
        size_t sz = en->enumerators().size();
        if (marshal)
        {
            out << nl << "if (" << param << " is not null)";
            out << sb;
            out << nl << stream << ".writeEnum(" << tag << ", (int)" << param << ".Value, " << sz << ");";
            out << eb;
        }
        else
        {
            out << nl << "if (" << stream << ".readOptional(" << tag << ", Ice.OptionalFormat.Size))";
            out << sb;
            string typeS = typeToString(type, scope);
            string tmp = "tmpVal";
            out << nl << typeS << ' ' << tmp << ';';
            writeMarshalUnmarshalCode(out, type, scope, tmp, marshal, customStream);
            out << nl << param << " = " << tmp << ";";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << param << " = null;";
            out << eb;
        }
        return;
    }

    SequencePtr seq = dynamic_pointer_cast<Sequence>(type);
    if (seq)
    {
        writeOptionalSequenceMarshalUnmarshalCode(out, seq, scope, param, tag, marshal, stream);
        return;
    }

    DictionaryPtr d = dynamic_pointer_cast<Dictionary>(type);
    assert(d);
    TypePtr keyType = d->keyType();
    TypePtr valueType = d->valueType();
    if (marshal)
    {
        out << nl << "if (" << param << " is not null && " << stream << ".writeOptional(" << tag << ", "
            << getOptionalFormat(d) << "))";
        out << sb;
        if (keyType->isVariableLength() || valueType->isVariableLength())
        {
            out << nl << "int pos = " << stream << ".startSize();";
        }
        else
        {
            out << nl << stream << ".writeSize(" << param << ".Count * "
                << (keyType->minWireSize() + valueType->minWireSize()) << " + (" << param << ".Count > 254 ? 5 : 1));";
        }
        writeMarshalUnmarshalCode(out, type, scope, param, marshal, customStream);
        if (keyType->isVariableLength() || valueType->isVariableLength())
        {
            out << nl << stream << ".endSize(pos);";
        }
        out << eb;
    }
    else
    {
        out << nl << "if (" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(d) << "))";
        out << sb;
        if (keyType->isVariableLength() || valueType->isVariableLength())
        {
            out << nl << stream << ".skip(4);";
        }
        else
        {
            out << nl << stream << ".skipSize();";
        }
        string typeS = typeToString(type, scope);
        string tmp = "tmpVal";
        out << nl << typeS << ' ' << tmp << " = new " << typeS << "();";
        writeMarshalUnmarshalCode(out, type, scope, tmp, marshal, customStream);
        out << nl << param << " = " << tmp << ";";
        out << eb;
        out << nl << "else";
        out << sb;
        out << nl << param << " = null;";
        out << eb;
    }
}

void
Slice::CsGenerator::writeSequenceMarshalUnmarshalCode(
    Output& out,
    const SequencePtr& seq,
    const string& scope,
    const string& param,
    bool marshal,
    bool useHelper,
    const string& customStream)
{
    string stream = customStream;
    if (stream.empty())
    {
        stream = marshal ? "ostr" : "istr";
    }

    ContainedPtr cont = dynamic_pointer_cast<Contained>(seq->container());
    assert(cont);
    if (useHelper)
    {
        string helperName = getUnqualified(getNamespace(seq) + "." + seq->name() + "Helper", scope);
        if (marshal)
        {
            out << nl << helperName << ".write(" << stream << ", " << param << ");";
        }
        else
        {
            out << nl << param << " = " << helperName << ".read(" << stream << ");";
        }
        return;
    }

    TypePtr type = seq->type();
    string typeS = typeToString(type, scope);

    const string genericPrefix = "cs:generic:";
    string genericType;
    string addMethod = "Add";
    const bool isGeneric = seq->findMetaData(genericPrefix, genericType);
    bool isStack = false;
    bool isList = false;
    bool isLinkedList = false;
    bool isCustom = false;
    if (isGeneric)
    {
        genericType = genericType.substr(genericPrefix.size());
        if (genericType == "LinkedList")
        {
            addMethod = "AddLast";
            isLinkedList = true;
        }
        else if (genericType == "Queue")
        {
            addMethod = "Enqueue";
        }
        else if (genericType == "Stack")
        {
            addMethod = "Push";
            isStack = true;
        }
        else if (genericType == "List")
        {
            isList = true;
        }
        else
        {
            isCustom = true;
        }
    }

    const bool isArray = !isGeneric;
    const string limitID = isArray ? "Length" : "Count";

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);

    if (builtin)
    {
        Builtin::Kind kind = builtin->kind();
        switch (kind)
        {
            case Builtin::KindValue:
            case Builtin::KindObject:
            case Builtin::KindObjectProxy:
            {
                if (marshal)
                {
                    out << nl << "if (" << param << " is null)";
                    out << sb;
                    out << nl << stream << ".writeSize(0);";
                    out << eb;
                    out << nl << "else";
                    out << sb;
                    out << nl << stream << ".writeSize(" << param << '.' << limitID << ");";
                    if (isGeneric && !isList)
                    {
                        if (isStack)
                        {
                            //
                            // If the collection is a stack, write in top-to-bottom order. Stacks
                            // cannot contain Ice.Value.
                            //
                            out << nl << "Ice.ObjectPrx?[] " << param << "_tmp = " << param << ".ToArray();";
                            out << nl << "for (int ix = 0; ix < " << param << "_tmp.Length; ++ix)";
                            out << sb;
                            out << nl << stream << ".writeProxy(" << param << "_tmp[ix]);";
                            out << eb;
                        }
                        else
                        {
                            out << nl << "global::System.Collections.Generic.IEnumerator<" << typeS << "> e = " << param
                                << ".GetEnumerator();";
                            out << nl << "while(e.MoveNext())";
                            out << sb;
                            string func = (kind == Builtin::KindObject || kind == Builtin::KindValue) ? "writeValue"
                                                                                                      : "writeProxy";
                            out << nl << stream << '.' << func << "(e.Current);";
                            out << eb;
                        }
                    }
                    else
                    {
                        out << nl << "for (int ix = 0; ix < " << param << '.' << limitID << "; ++ix)";
                        out << sb;
                        string func =
                            (kind == Builtin::KindObject || kind == Builtin::KindValue) ? "writeValue" : "writeProxy";
                        out << nl << stream << '.' << func << '(' << param << "[ix]);";
                        out << eb;
                    }
                    out << eb;
                }
                else
                {
                    out << nl << "int " << param << "_lenx = " << stream << ".readAndCheckSeqSize("
                        << static_cast<unsigned>(type->minWireSize()) << ");";
                    if (!isStack)
                    {
                        out << nl << param << " = new ";
                    }
                    if ((kind == Builtin::KindObject || kind == Builtin::KindValue))
                    {
                        string patcherName;
                        if (isArray)
                        {
                            patcherName = "Ice.Internal.Patcher.arrayReadValue";
                            out << "Ice.Value?[" << param << "_lenx];";
                        }
                        else if (isCustom)
                        {
                            patcherName = "Ice.Internal.Patcher.customSeqReadValue";
                            out << "global::" << genericType << "<Ice.Value?>();";
                        }
                        else
                        {
                            patcherName = "Ice.Internal.Patcher.listReadValue";
                            out << "global::System.Collections.Generic." << genericType << "<Ice.Value?>(" << param
                                << "_lenx);";
                        }
                        out << nl << "for (int ix = 0; ix < " << param << "_lenx; ++ix)";
                        out << sb;
                        out << nl << stream << ".readValue(" << patcherName << "<Ice.Value>(" << param << ", ix));";
                    }
                    else
                    {
                        if (isStack)
                        {
                            out << nl << "Ice.ObjectPrx?[] " << param << "_tmp = new Ice.ObjectPrx?[" << param
                                << "_lenx];";
                        }
                        else if (isArray)
                        {
                            out << "Ice.ObjectPrx?[" << param << "_lenx];";
                        }
                        else if (isCustom)
                        {
                            out << "global::" << genericType << "<Ice.ObjectPrx?>();";
                        }
                        else
                        {
                            out << "global::System.Collections.Generic." << genericType << "<Ice.ObjectPrx?>(";
                            if (!isLinkedList)
                            {
                                out << param << "_lenx";
                            }
                            out << ");";
                        }

                        out << nl << "for (int ix = 0; ix < " << param << "_lenx; ++ix)";
                        out << sb;
                        if (isArray || isStack)
                        {
                            string v = isArray ? param : param + "_tmp";
                            out << nl << v << "[ix] = " << stream << ".readProxy();";
                        }
                        else
                        {
                            out << nl << "Ice.ObjectPrx? val = " << stream << ".readProxy();";
                            out << nl << param << "." << addMethod << "(val);";
                        }
                    }
                    out << eb;

                    if (isStack)
                    {
                        out << nl << "global::System.Array.Reverse(" << param << "_tmp);";
                        out << nl << param << " = new global::System.Collections.Generic." << genericType << "<"
                            << typeS << ">(" << param << "_tmp);";
                    }
                }
                break;
            }
            default:
            {
                string func = typeS;
                func[0] = static_cast<char>(toupper(static_cast<unsigned char>(typeS[0])));
                if (marshal)
                {
                    // TODO: we have to pass "param!" because the comparison with null suggests it can be null.
                    // Note that the write method called deals accepts nulls too even though its signature is
                    // non-nullable.
                    if (isArray)
                    {
                        out << nl << stream << ".write" << func << "Seq(" << param << ");";
                    }
                    else if (isCustom)
                    {
                        out << nl << stream << ".write" << func << "Seq(" << param << " == null ? 0 : " << param
                            << ".Count, " << param << "!);";
                    }
                    else
                    {
                        assert(isGeneric);
                        out << nl << stream << ".write" << func << "Seq(" << param << " == null ? 0 : " << param
                            << ".Count, " << param << "!);";
                    }
                }
                else
                {
                    if (isArray)
                    {
                        out << nl << param << " = " << stream << ".read" << func << "Seq();";
                    }
                    else if (isCustom)
                    {
                        out << sb;
                        out << nl << param << " = new "
                            << "global::" << genericType << "<" << typeToString(type, scope) << ">();";
                        out << nl << "int szx = " << stream << ".readSize();";
                        out << nl << "for(int ix = 0; ix < szx; ++ix)";
                        out << sb;
                        out << nl << param << ".Add(" << stream << ".read" << func << "());";
                        out << eb;
                        out << eb;
                    }
                    else
                    {
                        assert(isGeneric);
                        out << nl << stream << ".read" << func << "Seq(out " << param << ");";
                    }
                }
                break;
            }
        }
        return;
    }

    ClassDeclPtr cl = dynamic_pointer_cast<ClassDecl>(type);
    if (cl)
    {
        if (marshal)
        {
            out << nl << "if (" << param << " is null)";
            out << sb;
            out << nl << stream << ".writeSize(0);";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << stream << ".writeSize(" << param << '.' << limitID << ");";
            if (isGeneric && !isList)
            {
                //
                // Stacks cannot contain class instances, so there is no need to marshal a
                // stack bottom-up here.
                //
                out << nl << "global::System.Collections.Generic.IEnumerator<" << typeS << "> e = " << param
                    << ".GetEnumerator();";
                out << nl << "while (e.MoveNext())";
                out << sb;
                out << nl << stream << ".writeValue(e.Current);";
                out << eb;
            }
            else
            {
                out << nl << "for(int ix = 0; ix < " << param << '.' << limitID << "; ++ix)";
                out << sb;
                out << nl << stream << ".writeValue(" << param << "[ix]);";
                out << eb;
            }
            out << eb;
        }
        else
        {
            out << sb;
            out << nl << "int szx = " << stream << ".readAndCheckSeqSize(" << static_cast<unsigned>(type->minWireSize())
                << ");";
            out << nl << param << " = new ";
            string patcherName;
            if (isArray)
            {
                patcherName = "Ice.Internal.Patcher.arrayReadValue";
                out << toArrayAlloc(typeS + "[]", "szx") << ";";
            }
            else if (isCustom)
            {
                patcherName = "Ice.Internal.Patcher.customSeqReadValue";
                out << "global::" << genericType << "<" << typeS << ">();";
            }
            else
            {
                patcherName = "Ice.Internal.Patcher.listReadValue";
                out << "global::System.Collections.Generic." << genericType << "<" << typeS << ">(szx);";
            }
            out << nl << "for (int ix = 0; ix < szx; ++ix)";
            out << sb;
            string scoped = dynamic_pointer_cast<Contained>(type)->scoped();
            // Remove trailing '?'
            string nonNullableTypeS = typeS.substr(0, typeS.size() - 1);
            out << nl << stream << ".readValue(" << patcherName << '<' << nonNullableTypeS << ">(" << param
                << ", ix));";
            out << eb;
            out << eb;
        }
        return;
    }

    StructPtr st = dynamic_pointer_cast<Struct>(type);
    if (st)
    {
        if (marshal)
        {
            out << nl << "if (" << param << " is null)";
            out << sb;
            out << nl << stream << ".writeSize(0);";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << stream << ".writeSize(" << param << '.' << limitID << ");";
            if (isGeneric && !isList)
            {
                //
                // Stacks are marshaled top-down.
                //
                if (isStack)
                {
                    out << nl << typeS << "[] " << param << "_tmp = " << param << ".ToArray();";
                    out << nl << "for(int ix = 0; ix < " << param << "_tmp.Length; ++ix)";
                }
                else
                {
                    out << nl << "global::System.Collections.Generic.IEnumerator<" << typeS << "> e = " << param
                        << ".GetEnumerator();";
                    out << nl << "while(e.MoveNext())";
                }
            }
            else
            {
                out << nl << "for(int ix = 0; ix < " << param << '.' << limitID << "; ++ix)";
            }
            out << sb;
            string call;
            if (isGeneric && !isList && !isStack)
            {
                if (isValueType(type))
                {
                    call = "e.Current";
                }
                else
                {
                    call = "(e.Current == null ? new ";
                    call += typeS + "() : e.Current)";
                }
            }
            else
            {
                call = param;
                if (isStack)
                {
                    call += "_tmp";
                }
                call += "[ix]";
            }
            call += ".";
            call += "ice_writeMembers";
            call += "(" + stream + ");";
            out << nl << call;
            out << eb;
            out << eb;
        }
        else
        {
            out << sb;
            out << nl << "int szx = " << stream << ".readAndCheckSeqSize(" << static_cast<unsigned>(type->minWireSize())
                << ");";
            if (isArray)
            {
                out << nl << param << " = new " << toArrayAlloc(typeS + "[]", "szx") << ";";
            }
            else if (isCustom)
            {
                out << nl << param << " = new global::" << genericType << "<" << typeS << ">();";
            }
            else if (isStack)
            {
                out << nl << typeS << "[] " << param << "_tmp = new " << toArrayAlloc(typeS + "[]", "szx") << ";";
            }
            else
            {
                out << nl << param << " = new global::System.Collections.Generic." << genericType << "<" << typeS
                    << ">(";
                if (!isLinkedList)
                {
                    out << "szx";
                }
                out << ");";
            }
            out << nl << "for(int ix = 0; ix < szx; ++ix)";
            out << sb;
            if (isArray || isStack)
            {
                string v = isArray ? param : param + "_tmp";
                out << nl << v << "[ix] = new " << typeS << "(istr);";
            }
            else
            {
                out << nl << typeS << " val = new " << typeS << "(istr);";
                out << nl << param << "." << addMethod << "(val);";
            }
            out << eb;
            if (isStack)
            {
                out << nl << "global::System.Array.Reverse(" << param << "_tmp);";
                out << nl << param << " = new global::System.Collections.Generic." << genericType << "<" << typeS
                    << ">(" << param << "_tmp);";
            }
            out << eb;
        }
        return;
    }

    EnumPtr en = dynamic_pointer_cast<Enum>(type);
    if (en)
    {
        if (marshal)
        {
            out << nl << "if (" << param << " is null)";
            out << sb;
            out << nl << stream << ".writeSize(0);";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << stream << ".writeSize(" << param << '.' << limitID << ");";
            if (isGeneric && !isList)
            {
                //
                // Stacks are marshaled top-down.
                //
                if (isStack)
                {
                    out << nl << typeS << "[] " << param << "_tmp = " << param << ".ToArray();";
                    out << nl << "for(int ix = 0; ix < " << param << "_tmp.Length; ++ix)";
                    out << sb;
                    out << nl << stream << ".writeEnum((int)" << param << "_tmp[ix], " << en->maxValue() << ");";
                    out << eb;
                }
                else
                {
                    out << nl << "global::System.Collections.Generic.IEnumerator<" << typeS << "> e = " << param
                        << ".GetEnumerator();";
                    out << nl << "while(e.MoveNext())";
                    out << sb;
                    out << nl << stream << ".writeEnum((int)e.Current, " << en->maxValue() << ");";
                    out << eb;
                }
            }
            else
            {
                out << nl << "for(int ix = 0; ix < " << param << '.' << limitID << "; ++ix)";
                out << sb;
                out << nl << stream << ".writeEnum((int)" << param << "[ix], " << en->maxValue() << ");";
                out << eb;
            }
            out << eb;
        }
        else
        {
            out << sb;
            out << nl << "int szx = " << stream << ".readAndCheckSeqSize(" << static_cast<unsigned>(type->minWireSize())
                << ");";
            if (isArray)
            {
                out << nl << param << " = new " << toArrayAlloc(typeS + "[]", "szx") << ";";
            }
            else if (isCustom)
            {
                out << nl << param << " = new global::" << genericType << "<" << typeS << ">();";
            }
            else if (isStack)
            {
                out << nl << typeS << "[] " << param << "_tmp = new " << toArrayAlloc(typeS + "[]", "szx") << ";";
            }
            else
            {
                out << nl << param << " = new global::System.Collections.Generic." << genericType << "<" << typeS
                    << ">(";
                if (!isLinkedList)
                {
                    out << "szx";
                }
                out << ");";
            }
            out << nl << "for(int ix = 0; ix < szx; ++ix)";
            out << sb;
            if (isArray || isStack)
            {
                string v = isArray ? param : param + "_tmp";
                out << nl << v << "[ix] = (" << typeS << ')' << stream << ".readEnum(" << en->maxValue() << ");";
            }
            else
            {
                out << nl << param << "." << addMethod << "((" << typeS << ')' << stream << ".readEnum("
                    << en->maxValue() << "));";
            }
            out << eb;
            if (isStack)
            {
                out << nl << "global::System.Array.Reverse(" << param << "_tmp);";
                out << nl << param << " = new global::System.Collections.Generic." << genericType << "<" << typeS
                    << ">(" << param << "_tmp);";
            }
            out << eb;
        }
        return;
    }

    string helperName;
    if (dynamic_pointer_cast<InterfaceDecl>(type))
    {
        helperName = getUnqualified(dynamic_pointer_cast<InterfaceDecl>(type), scope, "", "PrxHelper");
    }
    else
    {
        helperName = getUnqualified(dynamic_pointer_cast<Contained>(type), scope, "", "Helper");
    }

    string func;
    if (marshal)
    {
        func = "write";
        out << nl << "if (" << param << " is null)";
        out << sb;
        out << nl << stream << ".writeSize(0);";
        out << eb;
        out << nl << "else";
        out << sb;
        out << nl << stream << ".writeSize(" << param << '.' << limitID << ");";
        if (isGeneric && !isList)
        {
            //
            // Stacks are marshaled top-down.
            //
            if (isStack)
            {
                out << nl << typeS << "[] " << param << "_tmp = " << param << ".ToArray();";
                out << nl << "for(int ix = 0; ix < " << param << "_tmp.Length; ++ix)";
                out << sb;
                out << nl << helperName << '.' << func << '(' << stream << ", " << param << "_tmp[ix]);";
                out << eb;
            }
            else
            {
                out << nl << "global::System.Collections.Generic.IEnumerator<" << typeS << "> e = " << param
                    << ".GetEnumerator();";
                out << nl << "while(e.MoveNext())";
                out << sb;
                out << nl << helperName << '.' << func << '(' << stream << ", e.Current);";
                out << eb;
            }
        }
        else
        {
            out << nl << "for(int ix = 0; ix < " << param << '.' << limitID << "; ++ix)";
            out << sb;
            out << nl << helperName << '.' << func << '(' << stream << ", " << param << "[ix]);";
            out << eb;
        }
        out << eb;
    }
    else
    {
        func = "read";
        out << sb;
        out << nl << "int szx = " << stream << ".readAndCheckSeqSize(" << static_cast<unsigned>(type->minWireSize())
            << ");";
        if (isArray)
        {
            out << nl << param << " = new " << toArrayAlloc(typeS + "[]", "szx") << ";";
        }
        else if (isCustom)
        {
            out << nl << param << " = new global::" << genericType << "<" << typeS << ">();";
        }
        else if (isStack)
        {
            out << nl << typeS << "[] " << param << "_tmp = new " << toArrayAlloc(typeS + "[]", "szx") << ";";
        }
        else
        {
            out << nl << param << " = new global::System.Collections.Generic." << genericType << "<" << typeS << ">();";
        }
        out << nl << "for(int ix = 0; ix < szx; ++ix)";
        out << sb;
        if (isArray || isStack)
        {
            string v = isArray ? param : param + "_tmp";
            out << nl << v << "[ix] = " << helperName << '.' << func << '(' << stream << ");";
        }
        else
        {
            out << nl << param << "." << addMethod << "(" << helperName << '.' << func << '(' << stream << "));";
        }
        out << eb;
        if (isStack)
        {
            out << nl << "global::System.Array.Reverse(" << param << "_tmp);";
            out << nl << param << " = new global::System.Collections.Generic." << genericType << "<" << typeS << ">("
                << param << "_tmp);";
        }
        out << eb;
    }

    return;
}

void
Slice::CsGenerator::writeOptionalSequenceMarshalUnmarshalCode(
    Output& out,
    const SequencePtr& seq,
    const string& scope,
    const string& param,
    int tag,
    bool marshal,
    const string& customStream)
{
    string stream = customStream;
    if (stream.empty())
    {
        stream = marshal ? "ostr" : "istr";
    }

    const TypePtr type = seq->type();
    const string typeS = typeToString(type, scope);
    const string seqS = typeToString(seq, scope);

    string meta;
    const bool isArray = !seq->findMetaData("cs:generic:", meta);
    const string length = isArray ? param + ".Length" : param + ".Count";

    BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(type);
    if (builtin)
    {
        switch (builtin->kind())
        {
            case Builtin::KindByte:
            case Builtin::KindBool:
            case Builtin::KindShort:
            case Builtin::KindInt:
            case Builtin::KindFloat:
            case Builtin::KindLong:
            case Builtin::KindDouble:
            case Builtin::KindString:
            {
                string func = typeS;
                func[0] = static_cast<char>(toupper(static_cast<unsigned char>(typeS[0])));

                if (marshal)
                {
                    if (isArray)
                    {
                        out << nl << stream << ".write" << func << "Seq(" << tag << ", " << param << ");";
                    }
                    else
                    {
                        out << nl << "if (" << param << " is not null";
                        out << sb;
                        out << nl << stream << ".write" << func << "Seq(" << tag << ", " << param << ".Count, " << param
                            << ");";
                        out << eb;
                    }
                }
                else
                {
                    out << nl << "if (" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(seq) << "))";
                    out << sb;
                    if (builtin->isVariableLength())
                    {
                        out << nl << stream << ".skip(4);";
                    }
                    else if (builtin->kind() != Builtin::KindByte && builtin->kind() != Builtin::KindBool)
                    {
                        out << nl << stream << ".skipSize();";
                    }
                    string tmp = "tmpVal";
                    out << nl << seqS << ' ' << tmp << ';';
                    writeSequenceMarshalUnmarshalCode(out, seq, scope, tmp, marshal, true, stream);
                    out << nl << param << " = " << tmp << ";";
                    out << eb;
                    out << nl << "else";
                    out << sb;
                    out << nl << param << " = null;";
                    out << eb;
                }
                break;
            }

            case Builtin::KindValue:
            case Builtin::KindObject:
            case Builtin::KindObjectProxy:
            {
                if (marshal)
                {
                    out << nl << "if (" << param << " is not null && " << stream << ".writeOptional(" << tag << ", "
                        << getOptionalFormat(seq) << "))";
                    out << sb;
                    out << nl << "int pos = " << stream << ".startSize();";
                    writeSequenceMarshalUnmarshalCode(out, seq, scope, param, marshal, true, stream);
                    out << nl << stream << ".endSize(pos);";
                    out << eb;
                }
                else
                {
                    out << nl << "if (" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(seq) << "))";
                    out << sb;
                    out << nl << stream << ".skip(4);";
                    string tmp = "tmpVal";
                    out << nl << seqS << ' ' << tmp << ';';
                    writeSequenceMarshalUnmarshalCode(out, seq, scope, tmp, marshal, true, stream);
                    out << nl << param << " = " << tmp << ";";
                    out << eb;
                    out << nl << "else";
                    out << sb;
                    out << nl << param << " = null;";
                    out << eb;
                }
                break;
            }
        }

        return;
    }

    StructPtr st = dynamic_pointer_cast<Struct>(type);
    if (st)
    {
        if (marshal)
        {
            out << nl << "if (" << param << " is not null && " << stream << ".writeOptional(" << tag << ", "
                << getOptionalFormat(seq) << "))";
            out << sb;
            if (st->isVariableLength())
            {
                out << nl << "int pos = " << stream << ".startSize();";
            }
            else if (st->minWireSize() > 1)
            {
                out << nl << stream << ".writeSize(" << length << " * " << st->minWireSize() << " + (" << length
                    << " > 254 ? 5 : 1));";
            }
            writeSequenceMarshalUnmarshalCode(out, seq, scope, param, marshal, true, stream);
            if (st->isVariableLength())
            {
                out << nl << stream << ".endSize(pos);";
            }
            out << eb;
        }
        else
        {
            out << nl << "if (" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(seq) << "))";
            out << sb;
            if (st->isVariableLength())
            {
                out << nl << stream << ".skip(4);";
            }
            else if (st->minWireSize() > 1)
            {
                out << nl << stream << ".skipSize();";
            }
            string tmp = "tmpVal";
            out << nl << seqS << ' ' << tmp << ';';
            writeSequenceMarshalUnmarshalCode(out, seq, scope, tmp, marshal, true, stream);
            out << nl << param << " = " << tmp << ";";
            out << eb;
            out << nl << "else";
            out << sb;
            out << nl << param << " = null;";
            out << eb;
        }
        return;
    }

    //
    // At this point, all remaining element types have variable size.
    //
    if (marshal)
    {
        out << nl << "if (" << param << " is not null && " << stream << ".writeOptional(" << tag << ", "
            << getOptionalFormat(seq) << "))";
        out << sb;
        out << nl << "int pos = " << stream << ".startSize();";
        writeSequenceMarshalUnmarshalCode(out, seq, scope, param, marshal, true, stream);
        out << nl << stream << ".endSize(pos);";
        out << eb;
    }
    else
    {
        out << nl << "if (" << stream << ".readOptional(" << tag << ", " << getOptionalFormat(seq) << "))";
        out << sb;
        out << nl << stream << ".skip(4);";
        string tmp = "tmpVal";
        out << nl << seqS << ' ' << tmp << ';';
        writeSequenceMarshalUnmarshalCode(out, seq, scope, tmp, marshal, true, stream);
        out << nl << param << " = " << tmp << ";";
        out << eb;
        out << nl << "else";
        out << sb;
        out << nl << param << " = null;";
        out << eb;
    }
}

string
Slice::CsGenerator::toArrayAlloc(const string& decl, const string& sz)
{
    string::size_type pos = decl.size();
    while (pos > 1 && decl.substr(pos - 2, 2) == "[]")
    {
        pos -= 2;
    }

    ostringstream o;
    o << decl.substr(0, pos) << '[' << sz << ']' << decl.substr(pos + 2);
    return o.str();
}

void
Slice::CsGenerator::validateMetaData(const UnitPtr& u)
{
    MetaDataVisitor visitor;
    u->visit(&visitor, true);
}

bool
Slice::CsGenerator::MetaDataVisitor::visitUnitStart(const UnitPtr& p)
{
    //
    // Validate file metadata in the top-level file and all included files.
    //
    StringList files = p->allFiles();
    for (StringList::iterator q = files.begin(); q != files.end(); ++q)
    {
        string file = *q;
        DefinitionContextPtr dc = p->findDefinitionContext(file);
        assert(dc);
        StringList globalMetaData = dc->getMetaData();
        StringList newGlobalMetaData;
        static const string csPrefix = "cs:";

        for (StringList::iterator r = globalMetaData.begin(); r != globalMetaData.end(); ++r)
        {
            string& s = *r;
            string oldS = s;

            if (s.find(csPrefix) == 0)
            {
                static const string csAttributePrefix = csPrefix + "attribute:";
                if (!(s.find(csAttributePrefix) == 0 && s.size() > csAttributePrefix.size()))
                {
                    dc->warning(InvalidMetaData, file, -1, "ignoring invalid file metadata `" + oldS + "'");
                    continue;
                }
            }
            newGlobalMetaData.push_back(oldS);
        }

        dc->setMetaData(newGlobalMetaData);
    }
    return true;
}

bool
Slice::CsGenerator::MetaDataVisitor::visitModuleStart(const ModulePtr& p)
{
    validate(p);
    return true;
}

void
Slice::CsGenerator::MetaDataVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    validate(p);
}

bool
Slice::CsGenerator::MetaDataVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    validate(p);
    return true;
}

bool
Slice::CsGenerator::MetaDataVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    validate(p);
    return true;
}

bool
Slice::CsGenerator::MetaDataVisitor::visitStructStart(const StructPtr& p)
{
    validate(p);
    return true;
}

void
Slice::CsGenerator::MetaDataVisitor::visitOperation(const OperationPtr& p)
{
    validate(p);

    ParamDeclList params = p->parameters();
    for (ParamDeclList::const_iterator i = params.begin(); i != params.end(); ++i)
    {
        visitParamDecl(*i);
    }
}

void
Slice::CsGenerator::MetaDataVisitor::visitParamDecl(const ParamDeclPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitDataMember(const DataMemberPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitSequence(const SequencePtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitDictionary(const DictionaryPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitEnum(const EnumPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::visitConst(const ConstPtr& p)
{
    validate(p);
}

void
Slice::CsGenerator::MetaDataVisitor::validate(const ContainedPtr& cont)
{
    const string msg = "ignoring invalid metadata";

    StringList localMetaData = cont->getMetaData();
    StringList newLocalMetaData;

    const UnitPtr ut = cont->unit();
    const DefinitionContextPtr dc = ut->findDefinitionContext(cont->file());
    assert(dc);

    for (StringList::iterator p = localMetaData.begin(); p != localMetaData.end(); ++p)
    {
        string& s = *p;
        string oldS = s;

        const string csPrefix = "cs:";
        if (s.find(csPrefix) == 0)
        {
            SequencePtr seq = dynamic_pointer_cast<Sequence>(cont);
            if (seq)
            {
                static const string csGenericPrefix = csPrefix + "generic:";
                if (s.find(csGenericPrefix) == 0)
                {
                    string type = s.substr(csGenericPrefix.size());
                    if (type == "LinkedList" || type == "Queue" || type == "Stack")
                    {
                        if (!isClassType(seq->type()))
                        {
                            newLocalMetaData.push_back(s);
                            continue;
                        }
                    }
                    else if (!type.empty())
                    {
                        newLocalMetaData.push_back(s);
                        continue; // Custom type or List<T>
                    }
                }
            }
            else if (dynamic_pointer_cast<Struct>(cont))
            {
                if (s.substr(csPrefix.size()) == "class")
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
                if (s.substr(csPrefix.size()) == "property")
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
                static const string csImplementsPrefix = csPrefix + "implements:";
                if (s.find(csImplementsPrefix) == 0)
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
            }
            else if (dynamic_pointer_cast<ClassDef>(cont) || dynamic_pointer_cast<Exception>(cont))
            {
                if (s.substr(csPrefix.size()) == "property")
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
            }
            else if (dynamic_pointer_cast<InterfaceDef>(cont))
            {
                static const string csImplementsPrefix = csPrefix + "implements:";
                if (s.find(csImplementsPrefix) == 0)
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
            }
            else if (dynamic_pointer_cast<Dictionary>(cont))
            {
                static const string csGenericPrefix = csPrefix + "generic:";
                if (s.find(csGenericPrefix) == 0)
                {
                    string type = s.substr(csGenericPrefix.size());
                    if (type == "SortedDictionary" || type == "SortedList")
                    {
                        newLocalMetaData.push_back(s);
                        continue;
                    }
                }
            }
            else if (dynamic_pointer_cast<DataMember>(cont))
            {
                DataMemberPtr dataMember = dynamic_pointer_cast<DataMember>(cont);
                StructPtr st = dynamic_pointer_cast<Struct>(dataMember->container());
                ExceptionPtr ex = dynamic_pointer_cast<Exception>(dataMember->container());
                ClassDefPtr cl = dynamic_pointer_cast<ClassDef>(dataMember->container());
                static const string csTypePrefix = csPrefix + "type:";
            }
            else if (dynamic_pointer_cast<Module>(cont))
            {
                static const string csNamespacePrefix = csPrefix + "namespace:";
                if (s.find(csNamespacePrefix) == 0 && s.size() > csNamespacePrefix.size())
                {
                    newLocalMetaData.push_back(s);
                    continue;
                }
            }

            static const string csAttributePrefix = csPrefix + "attribute:";
            if (s.find(csAttributePrefix) == 0 && s.size() > csAttributePrefix.size())
            {
                newLocalMetaData.push_back(s);
                continue;
            }

            dc->warning(InvalidMetaData, cont->file(), cont->line(), msg + " `" + oldS + "'");
            continue;
        }
        newLocalMetaData.push_back(s);
    }

    cont->setMetaData(newLocalMetaData);
}
