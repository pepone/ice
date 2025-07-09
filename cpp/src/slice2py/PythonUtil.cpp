// Copyright (c) ZeroC, Inc.

#include "PythonUtil.h"
#include "../Slice/MetadataValidation.h"
#include "../Slice/Util.h"
#include "Ice/StringUtil.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

using namespace std;
using namespace Slice;
using namespace IceInternal;

namespace
{
    struct PythonCodeFragment
    {
        /// The Slice definition.
        ContainedPtr contained;

        /// The generated code.
        string code;
    };

    enum MethodKind
    {
        SyncInvocation,
        AsyncInvocation,
        Dispatch
    };

    const char* const tripleQuotes = R"(""")";

    string typeToTypeHintString(const TypePtr& type, bool optional)
    {
        assert(type);

        if (optional)
        {
            if (isProxyType(type))
            {
                // We map optional proxies like regular proxies, as XxxPrx or None.
                return typeToTypeHintString(type, false);
            }
            else
            {
                return typeToTypeHintString(type, false) + " | None";
            }
        }

        static constexpr string_view builtinTable[] = {
            "int",
            "bool",
            "int",
            "int",
            "int",
            "float",
            "float",
            "str",
            "Ice.Object | None", // Not used anymore
            "Ice.ObjectPrx | None",
            "Ice.Value | None"};

        if (auto builtin = dynamic_pointer_cast<Builtin>(type))
        {
            if (builtin->kind() == Builtin::KindObject)
            {
                return string{builtinTable[Builtin::KindValue]};
            }
            else
            {
                return string{builtinTable[builtin->kind()]};
            }
        }
        else if (auto proxy = dynamic_pointer_cast<InterfaceDecl>(type))
        {
            return proxy->mappedScoped(".") + "Prx | None";
        }
        else if (auto seq = dynamic_pointer_cast<Sequence>(type))
        {
            return "list[" + typeToTypeHintString(seq->type(), false) + "]";
        }
        else if (auto dict = dynamic_pointer_cast<Dictionary>(type))
        {
            ostringstream os;
            os << "dict[" << typeToTypeHintString(dict->keyType(), false) << ", "
               << typeToTypeHintString(dict->valueType(), false) << "]";
            return os.str();
        }
        else
        {
            auto contained = dynamic_pointer_cast<Contained>(type);
            assert(contained);
            return contained->mappedScoped(".");
        }
    }

    string returnTypeHint(const OperationPtr& operation, MethodKind methodKind)
    {
        assert(operation);
        string returnTypeHint;
        ParameterList outParameters = operation->outParameters();
        if (operation->returnsMultipleValues())
        {
            ostringstream os;
            os << "tuple[";
            if (operation->returnType())
            {
                os << typeToTypeHintString(operation->returnType(), operation->returnIsOptional());
                os << ", ";
            }

            for (const auto& param : outParameters)
            {
                os << typeToTypeHintString(param->type(), param->optional());
                if (param != outParameters.back())
                {
                    os << ", ";
                }
            }
            os << "]";
            returnTypeHint = os.str();
        }
        else if (operation->returnType())
        {
            returnTypeHint = typeToTypeHintString(operation->returnType(), operation->returnIsOptional());
        }
        else if (!outParameters.empty())
        {
            const auto& param = outParameters.front();
            returnTypeHint = typeToTypeHintString(param->type(), param->optional());
        }
        else
        {
            returnTypeHint = "None";
        }

        switch (methodKind)
        {
            case AsyncInvocation:
                return "Awaitable[" + returnTypeHint + "]";
            case Dispatch:
                return returnTypeHint + " | Awaitable[" + returnTypeHint + "]";
            case SyncInvocation:
                return returnTypeHint;
        }
    }

    string operationReturnTypeHint(const OperationPtr& operation, MethodKind methodKind)
    {
        return " -> " + returnTypeHint(operation, methodKind);
    }

    /// Returns a DocString formatted link to the provided Slice identifier.
    string pyLinkFormatter(const string& rawLink, const ContainedPtr&, const SyntaxTreeBasePtr& target)
    {
        ostringstream result;
        if (target)
        {
            if (auto builtinTarget = dynamic_pointer_cast<Builtin>(target))
            {
                if (builtinTarget->kind() == Builtin::KindObject)
                {
                    result << ":class:`Ice.Object`";
                }
                else if (builtinTarget->kind() == Builtin::KindValue)
                {
                    result << ":class:`Ice.Value`";
                }
                else if (builtinTarget->kind() == Builtin::KindObjectProxy)
                {
                    result << ":class:`Ice.ObjectPrx`";
                }
                else
                {
                    result << "``" << typeToTypeHintString(builtinTarget, false) << "``";
                }
            }
            else if (auto operationTarget = dynamic_pointer_cast<Operation>(target))
            {
                string targetScoped = operationTarget->interface()->mappedScoped(".");

                // link to the method on the proxy interface
                result << ":meth:`" << targetScoped << "Prx." << operationTarget->mappedName() << "`";
            }
            else
            {
                string targetScoped = dynamic_pointer_cast<Contained>(target)->mappedScoped(".");
                result << ":class:`" << targetScoped;
                if (auto interfaceTarget = dynamic_pointer_cast<InterfaceDecl>(target))
                {
                    // link to the proxy interface
                    result << "Prx";
                }
                result << "`";
            }
        }
        else
        {
            result << "``";

            auto hashPos = rawLink.find('#');
            if (hashPos != string::npos)
            {
                if (hashPos != 0)
                {
                    result << rawLink.substr(0, hashPos) << ".";
                }
                result << rawLink.substr(hashPos + 1);
            }
            else
            {
                result << rawLink;
            }

            result << "``";
        }
        return result.str();
    }

    string formatFields(const DataMemberList& members)
    {
        if (members.empty())
        {
            return "";
        }

        ostringstream os;
        bool first = true;
        os << "{Ice.Util.format_fields(";
        for (const auto& dataMember : members)
        {
            if (!first)
            {
                os << ", ";
            }
            first = false;
            os << dataMember->mappedName() << "=self." << dataMember->mappedName();
        }
        os << ")}";
        return os.str();
    }
}

namespace Slice::Python
{
    class BufferedOutputBase
    {
    protected:
        std::ostringstream _outBuffer;
    };

    class BufferedOutput final : public BufferedOutputBase, public Output
    {
    public:
        BufferedOutput() : Output(_outBuffer) {}

        string str() const { return _outBuffer.str(); }
    };

    // CodeVisitor generates the Python mapping for a translation unit.
    class CodeVisitor final : public ParserVisitor
    {
    public:
        CodeVisitor();

        bool visitModuleStart(const ModulePtr&) final;
        void visitModuleEnd(const ModulePtr&) final;
        void visitClassDecl(const ClassDeclPtr&) final;
        bool visitClassDefStart(const ClassDefPtr&) final;
        void visitInterfaceDecl(const InterfaceDeclPtr&) final;
        bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
        bool visitExceptionStart(const ExceptionPtr&) final;
        bool visitStructStart(const StructPtr&) final;
        void visitSequence(const SequencePtr&) final;
        void visitDictionary(const DictionaryPtr&) final;
        void visitEnum(const EnumPtr&) final;
        void visitConst(const ConstPtr&) final;

        const vector<PythonCodeFragment>& codeFragments() const { return _codeFragments; }

    private:
        // Emit Python code for operations
        void writeOperations(const InterfaceDefPtr&, Output&);

        // Emit the tuple for a Slice type.
        string getMetaType(const TypePtr&);

        // Get the default value for initializing a given type.
        string getTypeInitializer(const DataMemberPtr&);

        // Write Python metadata as a tuple.
        void writeMetadata(const MetadataList&, Output&);

        // Convert an operation mode into a string.
        string getOperationMode(Slice::Operation::Mode);

        // Write a member assignment statement for a constructor.
        void writeAssign(const DataMemberPtr& member, Output& out);

        // Write a constant value.
        void writeConstantValue(const TypePtr&, const SyntaxTreeBasePtr&, const string&, Output&);

        /// Write constructor parameters with default values.
        void writeConstructorParams(const DataMemberList& members, Output&);

        /// Writes the provided @p remarks in its own subheading in the current comment (if @p remarks is non-empty).
        void writeRemarksDocComment(const StringList& remarks, bool needsNewline, Output& out);

        void writeDocstring(const optional<DocComment>&, const string&, Output&);
        void writeDocstring(const optional<DocComment>&, const DataMemberList&, Output&);
        void writeDocstring(const optional<DocComment>&, const EnumeratorList&, Output&);

        void writeDocstring(const OperationPtr&, MethodKind, Output&);

        vector<PythonCodeFragment> _codeFragments;
    };
}

// CodeVisitor implementation.
Slice::Python::CodeVisitor::CodeVisitor() {}

bool
Slice::Python::CodeVisitor::visitModuleStart(const ModulePtr& p)
{
    BufferedOutput out;
    out << nl << "__name__ = \"" << getAbsolute(p) << "\"";

    optional<DocComment> comment = DocComment::parseFrom(p, pyLinkFormatter);
    if (comment)
    {
        out << sp;
        writeDocstring(comment, "__doc__ = ", out);
    }
    _codeFragments.emplace_back(p, out.str());
    return true;
}

void
Slice::Python::CodeVisitor::visitModuleEnd(const ModulePtr&)
{
}

void
Slice::Python::CodeVisitor::visitClassDecl(const ClassDeclPtr& p)
{
    // Emit forward declarations.
    BufferedOutput out;
    out << nl << p->mappedName() << "_t = IcePy.declareValue(\"" << p->scoped() << "\")";
    _codeFragments.emplace_back(p, out.str());
}

void
Slice::Python::CodeVisitor::visitInterfaceDecl(const InterfaceDeclPtr& p)
{
    // Emit forward declarations.
    BufferedOutput out;
    out << nl << p->mappedName() << "Prx_t" << " = IcePy.declareProxy(\"" << p->scoped() << "\")";
    _codeFragments.emplace_back(p, out.str());
}

void
Slice::Python::CodeVisitor::writeOperations(const InterfaceDefPtr& p, Output& out)
{
    // Emits an abstract method for each operation.
    for (const auto& operation : p->operations())
    {
        const string sliceName = operation->name();
        const string mappedName = operation->mappedName();

        if (operation->hasMarshaledResult())
        {
            string capName = sliceName;
            capName[0] = static_cast<char>(toupper(static_cast<unsigned char>(capName[0])));
            out << sp;
            out << nl << "@staticmethod";
            out << nl << "def " << capName << "MarshaledResult(result, current: Ice.Current):";
            out.inc();
            out << nl << tripleQuotes;
            out << nl << "Immediately marshals the result of an invocation of " << sliceName;
            out << nl << "and returns an object that the servant implementation must return";
            out << nl << "as its result.";
            out << nl;
            out << nl << "Args:";
            out << nl << "  result: The result (or result tuple) of the invocation.";
            out << nl << "  current: The Current object passed to the invocation.";
            out << nl;
            out << nl << "Returns";
            out << nl << "  An object containing the marshaled result.";
            out << nl << tripleQuotes;
            out << nl << "return IcePy.MarshaledResult(result, " << getAbsolute(p) << "._op_" << sliceName
                << ", current.adapter.getCommunicator()._getImpl(), current.encoding)";
            out.dec();
        }

        out << sp;
        out << nl << "@abstractmethod";
        out << nl << "def " << mappedName << spar << "self";

        for (const auto& param : operation->parameters())
        {
            if (!param->isOutParam())
            {
                out << (param->mappedName() + ": " + typeToTypeHintString(param->type(), param->optional()));
            }
        }

        const string currentParamName = getEscapedParamName(operation->parameters(), "current");
        out << (currentParamName + ": Ice.Current");
        out << epar << operationReturnTypeHint(operation, Dispatch) << ":";
        out.inc();

        writeDocstring(operation, Dispatch, out);

        out << nl << "pass";
        out.dec();
    }
}

bool
Slice::Python::CodeVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    const string scoped = p->scoped();
    const string type = getMetaTypeReference(p);
    const string valueName = p->mappedName();
    const ClassDefPtr base = p->base();
    const DataMemberList members = p->dataMembers();

    BufferedOutput out;

    out << nl << "class " << valueName << '(' << (base ? getAbsolute(base) : "Ice.Value") << "):";
    out.inc();

    writeDocstring(DocComment::parseFrom(p, pyLinkFormatter), members, out);

    // __init__
    out << nl << "def __init__(";
    writeConstructorParams(p->allDataMembers(), out);
    out << "):";
    out.inc();
    if (!base && members.empty())
    {
        out << nl << "pass";
    }
    else
    {
        if (base)
        {
            out << nl << getTypeReference(base) << ".__init__(self";
            for (const auto& member : base->allDataMembers())
            {
                out << ", " << member->mappedName();
            }
            out << ')';
        }

        for (const auto& member : members)
        {
            writeAssign(member, out);
        }
    }
    out.dec();

    // ice_id
    out << sp;
    out << nl << "def ice_id(self):";
    out.inc();
    out << nl << "return \"" << scoped << "\"";
    out.dec();

    // ice_staticId
    out << sp;
    out << nl << "@staticmethod";
    out << nl << "def ice_staticId():";
    out.inc();
    out << nl << "return \"" << scoped << "\"";
    out.dec();

    // Generate the __repr__ method for this Value class.
    // The default __str__ method inherited from Ice.Value calls __repr__().
    out << sp;
    out << nl << "def __repr__(self):";
    out.inc();
    const auto& allDataMembers = p->allDataMembers();
    if (allDataMembers.empty())
    {
        out << nl << "return \"" << getAbsolute(p) << "()\"";
    }
    else
    {
        out << nl << "return f\"" << getAbsolute(p) << "(" << formatFields(allDataMembers) << ")\"";
    }
    out.dec();

    out.dec();

    out << sp;
    out << nl << valueName << "._t = IcePy.defineValue(";
    out.inc();
    out << nl << "\"" << scoped << "\",";
    out << nl << valueName << ",";
    out << nl << p->compactId() << ",";
    out << nl;
    writeMetadata(p->getMetadata(), out);
    out << ",";
    out << nl << "False,";
    out << nl << (base ? getTypeReference(base) : "None") << ",";
    out << nl << "(";

    // Members
    //
    // Data members are represented as a tuple:
    //
    //   ("MemberName", MemberMetadata, MemberType, Optional, Tag)
    //
    // where MemberType is either a primitive type constant (T_INT, etc.) or the id of a user-defined type.
    if (members.size() > 1)
    {
        out.inc();
        out << nl;
    }

    for (auto r = members.begin(); r != members.end(); ++r)
    {
        if (r != members.begin())
        {
            out << ',' << nl;
        }
        out << "(\"" << (*r)->mappedName() << "\", ";
        writeMetadata((*r)->getMetadata(), out);
        out << ", " << getMetaType((*r)->type());
        out << ", " << ((*r)->optional() ? "True" : "False") << ", " << ((*r)->optional() ? (*r)->tag() : 0) << ')';
    }

    if (members.size() == 1)
    {
        out << ',';
    }
    else if (members.size() > 1)
    {
        out.dec();
        out << nl;
    }
    out << "))";
    out.dec();

    _codeFragments.emplace_back(p, out.str());
    return false;
}

bool
Slice::Python::CodeVisitor::visitInterfaceDefStart(const InterfaceDefPtr& p)
{
    string scoped = p->scoped();
    string className = p->mappedName();
    string classAbs = getTypeReference(p);
    string prxName = className + "Prx";
    string prxAbs = classAbs + "Prx";
    InterfaceList bases = p->bases();

    BufferedOutput out;

    // Define the proxy class
    out << sp;
    out << nl << "class " << prxName << '(';

    vector<string> baseClasses;
    for (const auto& base : bases)
    {
        InterfaceDefPtr d = base;
        baseClasses.push_back(getTypeReference(base) + "Prx");
    }

    if (baseClasses.empty())
    {
        out << "Ice.ObjectPrx";
    }
    else
    {
        auto q = baseClasses.begin();
        while (q != baseClasses.end())
        {
            out << *q;

            if (++q != baseClasses.end())
            {
                out << ", ";
            }
        }
    }
    out << "):";
    out.inc();

    out << sp;
    out << nl << "def __init__(self, communicator, proxyString):";
    out.inc();
    out << nl << tripleQuotes;
    out << nl << "Creates a new " << prxName << " proxy";
    out << nl;
    out << nl << "Parameters";
    out << nl << "----------";
    out << nl << "communicator : Ice.Communicator";
    out << nl << "    The communicator of the new proxy.";
    out << nl << "proxyString : str";
    out << nl << "    The string representation of the proxy.";
    out << nl;
    out << nl << "Raises";
    out << nl << "------";
    out << nl << "ParseException";
    out << nl << "    Thrown when proxyString is not a valid proxy string.";
    out << nl << tripleQuotes;
    out << nl << "super().__init__(communicator, proxyString)";
    out.dec();

    OperationList operations = p->operations();
    for (const auto& operation : operations)
    {
        const string opName = operation->name();
        string mappedOpName = operation->mappedName();
        if (mappedOpName == "checkedCast" || mappedOpName == "uncheckedCast")
        {
            mappedOpName.insert(0, "_");
        }
        TypePtr ret = operation->returnType();
        ParameterList paramList = operation->parameters();
        string inParams;
        string inParamsDecl;

        // Find the last required parameter, all optional parameters after the last required parameter will use
        // None as the default.
        ParameterPtr lastRequiredParameter;
        for (const auto& q : paramList)
        {
            if (!q->isOutParam() && !q->optional())
            {
                lastRequiredParameter = q;
            }
        }

        bool afterLastRequiredParameter = lastRequiredParameter == nullptr;
        for (const auto& q : paramList)
        {
            if (!q->isOutParam())
            {
                if (!inParams.empty())
                {
                    inParams.append(", ");
                    inParamsDecl.append(", ");
                }
                string param = q->mappedName();
                inParams.append(param);
                param += ": " + typeToTypeHintString(q->type(), q->optional());
                if (afterLastRequiredParameter)
                {
                    param += " = None";
                }
                inParamsDecl.append(param);

                if (q == lastRequiredParameter)
                {
                    afterLastRequiredParameter = true;
                }
            }
        }

        out << sp;
        out << nl << "def " << mappedOpName << "(self";
        if (!inParamsDecl.empty())
        {
            out << ", " << inParamsDecl;
        }
        const string contextParamName = getEscapedParamName(operation->parameters(), "context");
        out << ", " << contextParamName << ": dict[str, str] | None = None)"
            << operationReturnTypeHint(operation, SyncInvocation) << ":";
        out.inc();
        writeDocstring(operation, SyncInvocation, out);
        out << nl << "return " << className << "._op_" << opName << ".invoke(self, ((" << inParams;
        if (!inParams.empty() && inParams.find(',') == string::npos)
        {
            out << ", ";
        }
        out << "), " << contextParamName << "))";
        out.dec();

        // Async operations.
        out << sp;
        out << nl << "def " << mappedOpName << "Async(self";
        if (!inParams.empty())
        {
            out << ", " << inParams;
        }
        out << ", " << contextParamName << ": dict[str, str] = None)"
            << operationReturnTypeHint(operation, AsyncInvocation) << ":";
        out.inc();
        writeDocstring(operation, AsyncInvocation, out);
        out << nl << "return " << className << "._op_" << opName << ".invokeAsync(self, ((" << inParams;
        if (!inParams.empty() && inParams.find(',') == string::npos)
        {
            out << ", ";
        }
        out << "), " << contextParamName << "))";
        out.dec();
    }

    const string prxTypeHint = typeToTypeHintString(p->declaration(), false);

    out << sp;
    out << nl << "@staticmethod";
    out << nl << "def checkedCast(";
    out.inc();
    out << nl << "proxy: Ice.ObjectPrx | None,";
    out << nl << "facet: str | None = None,";
    out << nl << "context: dict[str, str] | None = None";
    out.dec();
    out << nl << ") -> " << prxTypeHint << ":";
    out.inc();
    out << nl << "return Ice.checkedCast(" << prxName << ", proxy, facet, context)";
    out.dec();

    out << sp;
    out << nl << "@staticmethod";
    out << nl << "def checkedCastAsync(";
    out.inc();
    out << nl << "proxy: Ice.ObjectPrx | None,";
    out << nl << "facet: str | None = None,";
    out << nl << "context: dict[str, str] | None = None";
    out.dec();
    out << nl << ") -> Awaitable[" << prxTypeHint << "]:";
    out.inc();
    out << nl << "return Ice.checkedCastAsync(" << prxName << ", proxy, facet, context)";
    out.dec();

    out << sp << nl << "@staticmethod";
    out << nl << "def uncheckedCast(proxy: Ice.ObjectPrx | None, facet: str | None = None) -> " << prxTypeHint << ":";
    out.inc();
    out << nl << "return Ice.uncheckedCast(" << prxName << ", proxy, facet)";
    out.dec();

    // ice_staticId
    out << sp;
    out << nl << "@staticmethod";
    out << nl << "def ice_staticId() -> str:";
    out.inc();
    out << nl << "return \"" << scoped << "\"";
    out.dec();

    out.dec(); // end prx class

    out << sp;
    out << nl << prxName << "._t = IcePy.defineProxy(\"" << scoped << "\", " << prxName << ")";

    // Define the servant class
    out << sp;
    out << nl << "class " << className;
    out << spar;
    if (bases.empty())
    {
        out << "Ice.Object";
    }
    else
    {
        for (const auto& base : bases)
        {
            out << getTypeReference(base);
        }
    }
    out << "ABC" << epar << ':';
    out.inc();

    // ice_ids
    StringList ids = p->ids();
    out << sp;
    out << nl << "def ice_ids(self, current: Ice.Current) -> list[str] | Awaitable[list[str]]:";
    out.inc();
    out << nl << "return (";
    for (auto q = ids.begin(); q != ids.end(); ++q)
    {
        if (q != ids.begin())
        {
            out << ", ";
        }
        out << "\"" << *q << "\"";
    }
    out << ')';
    out.dec();

    // ice_id
    out << sp;
    out << nl << "def ice_id(self, current: Ice.Current) -> str | Awaitable[str]:";
    out.inc();
    out << nl << "return \"" << scoped << "\"";
    out.dec();

    // ice_staticId
    out << sp;
    out << nl << "@staticmethod";
    out << nl << "def ice_staticId() -> str:";
    out.inc();
    out << nl << "return \"" << scoped << "\"";
    out.dec();

    writeOperations(p, out);

    out.dec();

    //
    // Define each operation. The arguments to the IcePy.Operation constructor are:
    //
    // "sliceOpName", "mappedOpName", Mode, AMD, Format, Metadata, (InParams), (OutParams), ReturnParam, (Exceptions)
    //
    // where InParams and OutParams are tuples of type descriptions, and Exceptions
    // is a tuple of exception type ids.

    for (const auto& operation : operations)
    {
        string format;
        optional<FormatType> opFormat = operation->format();
        if (opFormat)
        {
            switch (*opFormat)
            {
                case CompactFormat:
                    format = "Ice.FormatType.CompactFormat";
                    break;
                case SlicedFormat:
                    format = "Ice.FormatType.SlicedFormat";
                    break;
                default:
                    assert(false);
            }
        }
        else
        {
            format = "None";
        }

        const string sliceName = operation->name();

        out << sp;
        out << nl << className << "._op_" << sliceName << " = IcePy.Operation(";
        out.inc();
        out << nl << "\"" << sliceName << "\",";
        out << nl << "\"" << operation->mappedName() << "\",";
        out << nl << getOperationMode(operation->mode()) << ",";
        out << nl << format << ",";
        writeMetadata(operation->getMetadata(), out);
        out << ",";
        out << nl << "(";
        for (const auto& param : operation->inParameters())
        {
            if (param != operation->inParameters().front())
            {
                out << ", ";
            }
            out << '(';
            writeMetadata(param->getMetadata(), out);
            out << ", " << getMetaType(param->type());
            out << ", ";
            if (param->optional())
            {
                out << "True, " << param->tag();
            }
            else
            {
                out << "False, 0";
            }
            out << ')';
        }

        // A trailing command to ensure that the outut is interpreted as a Python tuple.
        if (operation->inParameters().size() == 1)
        {
            out << ',';
        }
        out << "),";
        out << nl << "(";
        for (const auto& param : operation->outParameters())
        {
            if (param != operation->outParameters().front())
            {
                out << ", ";
            }
            out << '(';
            writeMetadata(param->getMetadata(), out);
            out << ", " << getMetaType(param->type());
            out << ", ";
            if (param->optional())
            {
                out << "True, " << param->tag();
            }
            else
            {
                out << "False, 0";
            }
            out << ')';
        }

        // A trailing command to ensure that the outut is interpreted as a Python tuple.
        if (operation->outParameters().size() == 1)
        {
            out << ',';
        }
        out << "),";

        out << nl;
        TypePtr returnType = operation->returnType();
        if (returnType)
        {
            // The return type has the same format as an in/out parameter:
            //
            // Metadata, Type, Optional?, OptionalTag
            out << "((), " << getMetaType(returnType) << ", ";
            if (operation->returnIsOptional())
            {
                out << "True, " << operation->returnTag();
            }
            else
            {
                out << "False, 0";
            }
            out << ')';
        }
        else
        {
            out << "None";
        }
        out << ",";
        out << nl << "(";
        for (const auto& ex : operation->throws())
        {
            if (ex != operation->throws().front())
            {
                out << ", ";
            }
            out << getAbsolute(ex) + "._t";
        }

        // A trailing command to ensure that the outut is interpreted as a Python tuple.
        if (operation->throws().size() == 1)
        {
            out << ',';
        }
        out << "))";
        out.dec();

        if (operation->isDeprecated())
        {
            // Get the deprecation reason if present, or default to an empty string.
            string reason = operation->getDeprecationReason().value_or("");
            out << nl << className << "._op_" << sliceName << ".deprecate(\"" << reason << "\")";
        }
    }

    _codeFragments.emplace_back(p, out.str());
    return false;
}

bool
Slice::Python::CodeVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    const string scoped = p->scoped();
    const string name = p->mappedName();

    const ExceptionPtr base = p->base();
    BufferedOutput out;

    const DataMemberList members = p->dataMembers();

    out << sp;
    out << nl << "class " << name << '(';

    string baseName;
    if (base)
    {
        baseName = getTypeReference(base);
        out << baseName;
    }
    else
    {
        out << "Ice.UserException";
    }
    out << "):";
    out.inc();

    writeDocstring(DocComment::parseFrom(p, pyLinkFormatter), members, out);

    // __init__
    out << nl << "def __init__(";
    writeConstructorParams(p->allDataMembers(), out);
    out << "):";
    out.inc();
    if (!base && members.empty())
    {
        out << nl << "pass";
    }
    else
    {
        if (base)
        {
            out << nl << baseName << ".__init__(self";
            for (const auto& member : base->allDataMembers())
            {
                out << ", " << member->mappedName();
            }
            out << ')';
        }
        for (const auto& member : members)
        {
            writeAssign(member, out);
        }
    }
    out.dec();

    // Generate the __repr__ method for this Exception class.
    // The default __str__ method inherited from Ice.UserException calls __repr__().
    out << sp;
    out << nl << "def __repr__(self) -> str:";
    out.inc();
    const auto& allDataMembers = p->allDataMembers();
    if (allDataMembers.empty())
    {
        out << nl << "return \"" << getAbsolute(p) << "()\"";
    }
    else
    {
        out << nl << "return f\"" << getAbsolute(p) << "(" << formatFields(p->allDataMembers()) << ")\"";
    }
    out.dec();

    // _ice_id
    out << sp;
    out << nl << "_ice_id = \"" << scoped << "\"";

    out.dec();

    // Emit the type information.
    string type = getMetaTypeReference(p);
    out << sp;
    out << nl << "IcePy.defineException(\"" << scoped << "\", " << name << ", ";
    writeMetadata(p->getMetadata(), out);
    out << ", ";
    if (!base)
    {
        out << "None";
    }
    else
    {
        out << getMetaTypeReference(base);
    }
    out << ", (";
    if (members.size() > 1)
    {
        out.inc();
        out << nl;
    }
    //
    // Data members are represented as a tuple:
    //
    //   ("MemberName", MemberMetadata, MemberType, Optional, Tag)
    //
    // where MemberType is either a primitive type constant (T_INT, etc.) or the id of a user-defined type.
    //
    for (auto dmli = members.begin(); dmli != members.end(); ++dmli)
    {
        if (dmli != members.begin())
        {
            out << ',' << nl;
        }
        out << "(\"" << (*dmli)->mappedName() << "\", ";
        writeMetadata((*dmli)->getMetadata(), out);
        out << ", " << getMetaType((*dmli)->type());
        out << ", " << ((*dmli)->optional() ? "True" : "False") << ", " << ((*dmli)->optional() ? (*dmli)->tag() : 0)
            << ')';
    }
    if (members.size() == 1)
    {
        out << ',';
    }
    else if (members.size() > 1)
    {
        out.dec();
        out << nl;
    }
    out << "))";

    _codeFragments.emplace_back(p, out.str());
    return false;
}

bool
Slice::Python::CodeVisitor::visitStructStart(const StructPtr& p)
{
    const string scoped = p->scoped();
    const string abs = getTypeReference(p);
    const string name = p->mappedName();
    const DataMemberList members = p->dataMembers();
    BufferedOutput out;

    out << sp;
    out << nl << "@dataclass";
    if (Dictionary::isLegalKeyType(p))
    {
        out << "(order=True, unsafe_hash=True)";
    }

    out << nl << "class " << name << ":";
    out.inc();

    writeDocstring(DocComment::parseFrom(p, pyLinkFormatter), members, out);

    for (const auto& field : members)
    {
        out << nl << field->mappedName() << ": " << typeToTypeHintString(field->type(), field->optional());

        if (field->defaultValue())
        {
            out << " = ";
            writeConstantValue(field->type(), field->defaultValueType(), *field->defaultValue(), out);
        }
        else if (field->optional())
        {
            out << " = None";
        }
        else if (auto st = dynamic_pointer_cast<Struct>(field->type()))
        {
            // See writeAssign.
            out << " = " << "field(default_factory=" << getTypeReference(st) << ')';
        }
        else
        {
            out << " = " + getTypeInitializer(field);
        }
    }
    out.dec();

    //
    // Emit the type information.
    //
    out << sp;
    out << nl << name << "._t = IcePy.defineStruct(\"" << scoped << "\", " << name << ", ";
    writeMetadata(p->getMetadata(), out);
    out << ", (";
    //
    // Data members are represented as a tuple:
    //
    //   ("MemberName", MemberMetadata, MemberType)
    //
    // where MemberType is either a primitive type constant (T_INT, etc.) or the id of a user-defined type.
    //
    if (members.size() > 1)
    {
        out.inc();
        out << nl;
    }

    for (auto r = members.begin(); r != members.end(); ++r)
    {
        if (r != members.begin())
        {
            out << ',' << nl;
        }
        out << "(\"" << (*r)->mappedName() << "\", ";
        writeMetadata((*r)->getMetadata(), out);
        out << ", " << getMetaType((*r)->type());
        out << ')';
    }

    if (members.size() == 1)
    {
        out << ',';
    }
    else if (members.size() > 1)
    {
        out.dec();
        out << nl;
    }
    out << "))";

    _codeFragments.emplace_back(p, out.str());
    return false;
}

void
Slice::Python::CodeVisitor::visitSequence(const SequencePtr& p)
{
    BufferedOutput out;
    out << nl << p->mappedName() << "_t = IcePy.defineSequence(\"" << p->scoped() << "\", ";
    writeMetadata(p->getMetadata(), out);
    out << ", " << getMetaType(p->type());
    out << ")";
    _codeFragments.emplace_back(p, out.str());
}

void
Slice::Python::CodeVisitor::visitDictionary(const DictionaryPtr& p)
{
    BufferedOutput out;
    out << nl << p->mappedName() << "_t = IcePy.defineDictionary(\"" << p->scoped() << "\", ";
    writeMetadata(p->getMetadata(), out);
    out << ", " << getMetaType(p->keyType()) << ", " << getMetaType(p->valueType()) << ")";
    _codeFragments.emplace_back(p, out.str());
}

void
Slice::Python::CodeVisitor::visitEnum(const EnumPtr& p)
{
    string scoped = p->scoped();
    string name = p->mappedName();
    EnumeratorList enumerators = p->enumerators();

    BufferedOutput out;
    out << nl << "class " << name << "(Ice.EnumBase):";
    out.inc();

    writeDocstring(DocComment::parseFrom(p, pyLinkFormatter), enumerators, out);

    out << sp;
    out << nl << "def __init__(self, name, value):";
    out.inc();
    out << nl << "Ice.EnumBase.__init__(self, name, value)";
    out.dec();

    out << sp;
    out << nl << "@staticmethod";
    out << nl << "def valueOf(name):";
    out.inc();
    out << nl << "return " << name << "._enumerators.get(name)";
    out.dec();

    out << sp;
    out << nl << "def __repr__(self):";
    out.inc();

    out << nl << "if self._value in " << name << "._enumerators:";
    out.inc();
    out << nl << "return f\"" << getAbsolute(p) << ".{self._name}\"";
    out.dec();

    out << nl << "else:";
    out.inc();
    out << nl << "return f\"" << getAbsolute(p) << "({self._name!r}, {self._value!r})\"";
    out.dec();

    out.dec();

    out.dec();

    out << sp;
    for (const auto& enumerator : enumerators)
    {
        out << nl << name << '.' << enumerator->mappedName() << " = " << name << "(\"" << enumerator->name() << "\", "
            << enumerator->value() << ')';
    }
    out << nl << name << "._enumerators = { ";
    out.inc();
    for (const auto& enumerator : enumerators)
    {
        out << nl << enumerator->value() << ':' << name << '.' << enumerator->mappedName() << ',';
    }
    out.dec();
    out << nl << "}";

    // Emit the type information.
    out << sp;
    out << nl << name << "._t = IcePy.defineEnum(\"" << scoped << "\", " << name << ", ";
    writeMetadata(p->getMetadata(), out);
    out << ", " << name << "._enumerators)";
    _codeFragments.emplace_back(p, out.str());
}

void
Slice::Python::CodeVisitor::visitConst(const ConstPtr& p)
{
    BufferedOutput out;
    out << sp;
    out << nl << p->mappedName() << " = ";
    writeConstantValue(p->type(), p->valueType(), p->value(), out);
    _codeFragments.emplace_back(p, out.str());
}

string
Slice::Python::CodeVisitor::getMetaType(const TypePtr& p)
{
    static const char* builtinTable[] = {
        "IcePy._t_byte",
        "IcePy._t_bool",
        "IcePy._t_short",
        "IcePy._t_int",
        "IcePy._t_long",
        "IcePy._t_float",
        "IcePy._t_double",
        "IcePy._t_string",
        "Ice.Value._t",
        "Ice.ObjectPrx._t",
        "Ice.Value._t"};

    if (auto builtin = dynamic_pointer_cast<Builtin>(p))
    {
        return builtinTable[builtin->kind()];
    }
    else if (auto prx = dynamic_pointer_cast<InterfaceDecl>(p))
    {
        return getAbsolute(prx) + "Prx_t";
    }
    else
    {
        ContainedPtr cont = dynamic_pointer_cast<Contained>(p);
        assert(cont);
        return getAbsolute(cont) + "_t";
    }
}

string
Slice::Python::CodeVisitor::getTypeInitializer(const DataMemberPtr& field)
{
    static constexpr string_view builtinTable[] = {
        "0",     // Builtin::KindByte
        "False", // Builtin::KindBool
        "0",     // Builtin::KindShort
        "0",     // Builtin::KindInt
        "0",     // Builtin::KindLong
        "0.0",   // Builtin::KindFloat
        "0.0",   // Builtin::KindDouble
        R"("")", // Builtin::KindString
        "None",  // Builtin::KindObject. Not used anymore
        "None",  // Builtin::KindObjectProxy.
        "None"}; // Builtin::KindValue.

    if (auto builtin = dynamic_pointer_cast<Builtin>(field->type()))
    {
        return string{builtinTable[builtin->kind()]};
    }
    else if (auto enumeration = dynamic_pointer_cast<Enum>(field->type()))
    {
        return getAbsolute(enumeration) + "." + enumeration->enumerators().front()->mappedName();
    }
    else if (dynamic_pointer_cast<Sequence>(field->type()))
    {
        // TODO: add support for Python metadata.
        return "field(default_factory=list)";
    }
    else if (dynamic_pointer_cast<Dictionary>(field->type()))
    {
        return "field(default_factory=dict)";
    }
    else
    {
        return "None";
    }
}

void
Slice::Python::CodeVisitor::writeMetadata(const MetadataList& metadata, Output& out)
{
    MetadataList pythonMetadata = metadata;
    auto newEnd = std::remove_if(
        pythonMetadata.begin(),
        pythonMetadata.end(),
        [](const MetadataPtr& meta) { return meta->directive().find("python:") != 0; });
    pythonMetadata.erase(newEnd, pythonMetadata.end());

    out << '(';
    for (const auto& meta : pythonMetadata)
    {
        out << "\"" << *meta << "\"";
        if (meta != pythonMetadata.back())
        {
            out << ", ";
        }
    }

    if (pythonMetadata.size() == 1)
    {
        out << ',';
    }

    out << ')';
}

void
Slice::Python::CodeVisitor::writeAssign(const DataMemberPtr& member, Output& out)
{
    const string memberName = member->mappedName();

    // Structures are treated differently (see bug 3676).
    StructPtr st = dynamic_pointer_cast<Struct>(member->type());
    if (st && !member->optional())
    {
        out << nl << "self." << memberName << " = " << memberName << " if " << memberName << " is not None else "
            << getTypeReference(st) << "()";
    }
    else
    {
        out << nl << "self." << memberName << " = " << memberName;
    }
}

void
Slice::Python::CodeVisitor::writeConstantValue(
    const TypePtr& type,
    const SyntaxTreeBasePtr& valueType,
    const string& value,
    Output& out)
{
    ConstPtr constant = dynamic_pointer_cast<Const>(valueType);
    if (constant)
    {
        out << getTypeReference(constant);
    }
    else if (auto builtin = dynamic_pointer_cast<Slice::Builtin>(type))
    {
        switch (builtin->kind())
        {
            case Slice::Builtin::KindBool:
            {
                out << (value == "true" ? "True" : "False");
                break;
            }
            case Slice::Builtin::KindByte:
            case Slice::Builtin::KindShort:
            case Slice::Builtin::KindInt:
            case Slice::Builtin::KindFloat:
            case Slice::Builtin::KindDouble:
            case Slice::Builtin::KindLong:
            {
                out << value;
                break;
            }
            case Slice::Builtin::KindString:
            {
                const string controlChars = "\a\b\f\n\r\t\v";
                const unsigned char cutOff = 0;

                out << "\"" << toStringLiteral(value, controlChars, "", UCN, cutOff) << "\"";
                break;
            }
            case Slice::Builtin::KindValue:
            case Slice::Builtin::KindObject:
            case Slice::Builtin::KindObjectProxy:
                assert(false);
        }
    }
    else if (dynamic_pointer_cast<Slice::Enum>(type))
    {
        EnumeratorPtr enumerator = dynamic_pointer_cast<Enumerator>(valueType);
        assert(enumerator);
        out << getTypeReference(enumerator);
    }
    else
    {
        assert(false); // Unknown const type.
    }
}

void
Slice::Python::CodeVisitor::writeConstructorParams(const DataMemberList& members, Output& out)
{
    out << "self";
    for (const auto& member : members)
    {
        out << ", " << member->mappedName() << "=";
        if (member->defaultValue())
        {
            writeConstantValue(member->type(), member->defaultValueType(), *member->defaultValue(), out);
        }
        else if (member->optional())
        {
            out << "None";
        }
        else
        {
            out << getTypeInitializer(member);
        }
    }
}

string
Slice::Python::CodeVisitor::getOperationMode(Slice::Operation::Mode mode)
{
    return mode == Slice::Operation::Mode::Normal ? "Ice.MethodKind.Normal" : "Ice.MethodKind.Idempotent";
}

void
Slice::Python::CodeVisitor::writeRemarksDocComment(const StringList& remarks, bool needsNewline, Output& out)
{
    if (!remarks.empty())
    {
        if (needsNewline)
        {
            out << nl;
        }
        out << nl << "Notes";
        out << nl << "-----";
        for (const auto& line : remarks)
        {
            out << nl << "    " << line;
        }
    }
}

void
Slice::Python::CodeVisitor::writeDocstring(const optional<DocComment>& comment, const string& prefix, Output& out)
{
    if (comment)
    {
        const StringList& overview = comment->overview();
        const StringList& remarks = comment->remarks();
        if (overview.empty() && remarks.empty())
        {
            return;
        }

        out << nl << prefix << tripleQuotes;
        for (const auto& line : overview)
        {
            out << nl << line;
        }

        writeRemarksDocComment(remarks, !overview.empty(), out);

        out << nl << tripleQuotes;
    }
}

void
Slice::Python::CodeVisitor::writeDocstring(
    const optional<DocComment>& comment,
    const DataMemberList& members,
    Output& out)
{
    if (!comment)
    {
        return;
    }

    // Collect docstrings (if any) for the members.
    map<string, list<string>> docs;
    for (const auto& member : members)
    {
        if (auto memberDoc = DocComment::parseFrom(member, pyLinkFormatter))
        {
            const StringList& memberOverview = memberDoc->overview();
            if (!memberOverview.empty())
            {
                docs[member->name()] = memberOverview;
            }
        }
    }

    const StringList& overview = comment->overview();
    const StringList& remarks = comment->remarks();
    if (overview.empty() && remarks.empty() && docs.empty())
    {
        return;
    }

    out << nl << tripleQuotes;

    for (const auto& line : overview)
    {
        out << nl << line;
    }

    // Only emit members if there's a docstring for at least one member.
    if (!docs.empty())
    {
        if (!overview.empty())
        {
            out << nl;
        }
        out << nl << "Attributes";
        out << nl << "----------";
        for (const auto& member : members)
        {
            out << nl << member->mappedName() << " : " << typeToTypeHintString(member->type(), member->optional());
            auto p = docs.find(member->name());
            if (p != docs.end())
            {
                for (const auto& line : p->second)
                {
                    out << nl << "    " << line;
                }
            }
        }
    }

    writeRemarksDocComment(remarks, !overview.empty() || !docs.empty(), out);

    out << nl << tripleQuotes;
}

void
Slice::Python::CodeVisitor::writeDocstring(
    const optional<DocComment>& comment,
    const EnumeratorList& enumerators,
    Output& out)
{
    if (!comment)
    {
        return;
    }

    // Collect docstrings (if any) for the enumerators.
    map<string, list<string>> docs;
    for (const auto& enumerator : enumerators)
    {
        if (auto enumeratorDoc = DocComment::parseFrom(enumerator, pyLinkFormatter))
        {
            const StringList& enumeratorOverview = enumeratorDoc->overview();
            if (!enumeratorOverview.empty())
            {
                docs[enumerator->name()] = enumeratorOverview;
            }
        }
    }

    const StringList& overview = comment->overview();
    const StringList& remarks = comment->remarks();
    if (overview.empty() && remarks.empty() && docs.empty())
    {
        return;
    }

    out << nl << tripleQuotes;

    for (const auto& line : overview)
    {
        out << nl << line;
    }

    // Only emit enumerators if there's a docstring for at least one enumerator.
    if (!docs.empty())
    {
        if (!overview.empty())
        {
            out << nl;
        }
        out << nl << "Enumerators:";
        for (const auto& enumerator : enumerators)
        {
            out << nl << nl << "- " << enumerator->mappedName();
            auto p = docs.find(enumerator->name());
            if (p != docs.end())
            {
                out << ":"; // Only emit a trailing ':' if there's documentation to emit for it.
                for (const auto& line : p->second)
                {
                    out << nl << "    " << line;
                }
            }
        }
    }

    writeRemarksDocComment(remarks, !overview.empty() || !docs.empty(), out);

    out << nl << tripleQuotes;
}

void
Slice::Python::CodeVisitor::writeDocstring(const OperationPtr& op, MethodKind methodKind, Output& out)
{
    optional<DocComment> comment = DocComment::parseFrom(op, pyLinkFormatter);
    if (!comment)
    {
        return;
    }

    TypePtr returnType = op->returnType();
    ParameterList params = op->parameters();
    ParameterList inParams = op->inParameters();
    ParameterList outParams = op->outParameters();

    const StringList& overview = comment->overview();
    const StringList& remarks = comment->remarks();
    const StringList& returnsDoc = comment->returns();
    const auto& parametersDoc = comment->parameters();
    const auto& exceptionsDoc = comment->exceptions();

    if (overview.empty() && remarks.empty())
    {
        if ((methodKind == SyncInvocation || methodKind == Dispatch) && parametersDoc.empty() &&
            exceptionsDoc.empty() && returnsDoc.empty())
        {
            return;
        }
        else if (methodKind == AsyncInvocation && inParams.empty())
        {
            return;
        }
        else if (methodKind == Dispatch && inParams.empty() && exceptionsDoc.empty())
        {
            return;
        }
    }

    // Emit the general description.
    out << nl << tripleQuotes;
    for (const string& line : overview)
    {
        out << nl << line;
    }

    // Emit arguments.
    bool needArgs = false;
    switch (methodKind)
    {
        case SyncInvocation:
        case AsyncInvocation:
        case Dispatch:
            needArgs = true;
            break;
    }

    if (needArgs)
    {
        if (!overview.empty())
        {
            out << nl;
        }

        out << nl << "Parameters";
        out << nl << "----------";
        for (const auto& param : inParams)
        {
            out << nl << param->mappedName() << " : " << typeToTypeHintString(param->type(), param->optional());
            const auto r = parametersDoc.find(param->name());
            if (r != parametersDoc.end())
            {
                for (const auto& line : r->second)
                {
                    out << nl << "    " << line;
                }
            }
        }

        if (methodKind == SyncInvocation || methodKind == AsyncInvocation)
        {
            const string contextParamName = getEscapedParamName(op->parameters(), "context");
            out << nl << contextParamName << " : dict[str, str]";
            out << nl << "    The request context for the invocation.";
        }

        if (methodKind == Dispatch)
        {
            const string currentParamName = getEscapedParamName(op->parameters(), "current");
            out << nl << currentParamName << " : Ice.Current";
            out << nl << "    The Current object for the dispatch.";
        }
    }

    // Emit return value(s).
    bool hasReturnValue = false;
    if (!op->returnsAnyValues() && (methodKind == AsyncInvocation || methodKind == Dispatch))
    {
        hasReturnValue = true;
        if (!overview.empty() || needArgs)
        {
            out << nl;
        }
        out << nl << "Returns";
        out << nl << "-------";
        out << nl << returnTypeHint(op, methodKind);
        if (methodKind == AsyncInvocation)
        {
            out << nl << "    An awaitable that is completed when the invocation completes.";
        }
        else if (methodKind == Dispatch)
        {
            out << nl << "    None or an awaitable that completes when the dispatch completes.";
        }
    }
    else if (op->returnsAnyValues())
    {
        hasReturnValue = true;
        if (!overview.empty() || needArgs)
        {
            out << nl;
        }
        out << nl << "Returns";
        out << nl << "-------";
        out << nl << returnTypeHint(op, methodKind);

        if (op->returnsMultipleValues())
        {
            out << nl;
            out << nl << "    A tuple containing:";
            if (returnType)
            {
                out << nl << "        - " << typeToTypeHintString(returnType, op->returnIsOptional());
                bool firstLine = true;
                for (const string& line : returnsDoc)
                {
                    if (firstLine)
                    {
                        firstLine = false;
                        out << " " << line;
                    }
                    else
                    {
                        out << nl << "          " << line;
                    }
                }
            }

            for (const auto& param : outParams)
            {
                out << nl << "        - " << typeToTypeHintString(param->type(), param->optional());
                const auto r = parametersDoc.find(param->name());
                if (r != parametersDoc.end())
                {
                    bool firstLine = true;
                    for (const string& line : r->second)
                    {
                        if (firstLine)
                        {
                            firstLine = false;
                            out << " " << line;
                        }
                        else
                        {
                            out << nl << "          " << line;
                        }
                    }
                }
            }
        }
        else if (returnType)
        {
            for (const string& line : returnsDoc)
            {
                out << nl << "    " << line;
            }
        }
        else if (!outParams.empty())
        {
            assert(outParams.size() == 1);
            const auto& param = outParams.front();
            out << nl << typeToTypeHintString(param->type(), param->optional());
            const auto r = parametersDoc.find(param->name());
            if (r != parametersDoc.end())
            {
                for (const string& line : r->second)
                {
                    out << nl << "    " << line;
                }
            }
        }
    }

    // Emit exceptions.
    if ((methodKind == SyncInvocation || methodKind == Dispatch) && !exceptionsDoc.empty())
    {
        if (!overview.empty() || needArgs || hasReturnValue)
        {
            out << nl;
        }
        out << nl << "Raises";
        out << nl << "------";
        for (const auto& [exception, exceptionDescription] : exceptionsDoc)
        {
            out << nl << exception;
            for (const auto& line : exceptionDescription)
            {
                out << nl << "    " << line;
            }
        }
    }

    writeRemarksDocComment(remarks, true, out);

    out << nl << tripleQuotes;
}

string
Slice::Python::getImportFileName(const string& file, const vector<string>& includePaths)
{
    // The file and includePaths arguments must be fully-qualified path names.
    string name = changeInclude(file, includePaths);
    replace(name.begin(), name.end(), '/', '_');
    return name + "_ice";
}

void
Slice::Python::generate(const UnitPtr& unit, bool all, const vector<string>& includePaths, Output& out)
{
    validateMetadata(unit);

    out << nl << "# Avoid evaluating annotations at function definition time.";
    out << nl << "from __future__ import annotations";

    if (unit->contains<InterfaceDef>())
    {
        // For async method annotations
        out << nl << "from collections.abc import Awaitable";
        // For skeleton classes
        out << nl << "from abc import ABC, abstractmethod";
    }
    if (unit->contains<Struct>())
    {
        out << nl << "from dataclasses import dataclass, field";
    }

    out << nl << "import Ice";
    out << nl << "import IcePy";

    if (!all)
    {
        vector<string> paths = includePaths;
        for (auto& path : paths)
        {
            path = fullPath(path);
        }

        StringList includes = unit->includeFiles();
        for (const auto& include : includes)
        {
            out << nl << "import " << getImportFileName(include, paths);
        }
    }

    CodeVisitor codeVisitor;
    unit->visit(&codeVisitor);

    for (const auto& codeFragment : codeVisitor.codeFragments())
    {
        cout << endl;
        cout << "# " << codeFragment.contained->scoped() << endl;
        cout << codeFragment.code << endl;
    }

    out << nl; // Trailing newline.
}

string
Slice::Python::getPackageMetadata(const ContainedPtr& cont)
{
    ModulePtr topLevelModule = cont->getTopLevelModule();

    // The python:package metadata can be defined as file metadata or applied to a top-level module.
    // We check for the metadata at the top-level module first and then fall back to the global scope.
    static const string directive = "python:package";
    if (auto packageMetadata = topLevelModule->getMetadataArgs(directive))
    {
        return *packageMetadata;
    }

    string_view file = cont->file();
    DefinitionContextPtr dc = cont->unit()->findDefinitionContext(file);
    assert(dc);
    return dc->getMetadataArgs(directive).value_or("");
}

string
Slice::Python::getAbsolute(const ContainedPtr& p)
{
    const string package = getPackageMetadata(p);
    const string packagePrefix = package + (package.empty() ? "" : ".");
    return packagePrefix + p->mappedScoped(".");
}

string
Slice::Python::getTypeReference(const ContainedPtr& p)
{
    const string package = getPackageMetadata(p);
    const string packagePrefix = package + (package.empty() ? "" : "_");
    return packagePrefix + p->mappedScoped("_");
}

string
Slice::Python::getMetaTypeReference(const ContainedPtr& p)
{
    string absoluteName = getTypeReference(p);

    // Append a "_t_" in front of the last name segment.
    auto pos = absoluteName.rfind('.');
    pos = (pos == string::npos ? 0 : pos + 1);
    absoluteName.insert(pos, "_t_");

    return absoluteName;
}

void
Slice::Python::printHeader(IceInternal::Output& out)
{
    out << "# Copyright (c) ZeroC, Inc.";
    out << sp;
    out << nl << "# slice2py version " << ICE_STRING_VERSION;
}

void
Slice::Python::validateMetadata(const UnitPtr& unit)
{
    auto pythonArrayTypeValidationFunc = [](const MetadataPtr& m, const SyntaxTreeBasePtr& p) -> optional<string>
    {
        if (auto sequence = dynamic_pointer_cast<Sequence>(p))
        {
            BuiltinPtr builtin = dynamic_pointer_cast<Builtin>(sequence->type());
            if (!builtin || !(builtin->isNumericType() || builtin->kind() == Builtin::KindBool))
            {
                return "the '" + m->directive() +
                       "' metadata can only be applied to sequences of bools, bytes, shorts, ints, longs, floats, "
                       "or doubles";
            }
        }
        return nullopt;
    };

    map<string, MetadataInfo> knownMetadata;

    // "python:<array-type>"
    MetadataInfo arrayTypeInfo = {
        .validOn = {typeid(Sequence)},
        .acceptedArgumentKind = MetadataArgumentKind::NoArguments,
        .acceptedContext = MetadataApplicationContext::DefinitionsAndTypeReferences,
        .extraValidation = pythonArrayTypeValidationFunc,
    };
    knownMetadata.emplace("python:array.array", arrayTypeInfo);
    knownMetadata.emplace("python:numpy.ndarray", std::move(arrayTypeInfo));

    // "python:identifier"
    MetadataInfo identifierInfo = {
        .validOn =
            {typeid(Module),
             typeid(InterfaceDecl),
             typeid(Operation),
             typeid(ClassDecl),
             typeid(Slice::Exception),
             typeid(Struct),
             typeid(Sequence),
             typeid(Dictionary),
             typeid(Enum),
             typeid(Enumerator),
             typeid(Const),
             typeid(Parameter),
             typeid(DataMember)},
        .acceptedArgumentKind = MetadataArgumentKind::SingleArgument,
    };
    knownMetadata.emplace("python:identifier", std::move(identifierInfo));

    // "python:memoryview"
    MetadataInfo memoryViewInfo = {
        .validOn = {typeid(Sequence)},
        .acceptedArgumentKind = MetadataArgumentKind::RequiredTextArgument,
        .acceptedContext = MetadataApplicationContext::DefinitionsAndTypeReferences,
        .extraValidation = pythonArrayTypeValidationFunc,
    };
    knownMetadata.emplace("python:memoryview", std::move(memoryViewInfo));

    // "python:package"
    MetadataInfo packageInfo = {
        .validOn = {typeid(Module), typeid(Unit)},
        .acceptedArgumentKind = MetadataArgumentKind::SingleArgument,
        .extraValidation = [](const MetadataPtr& metadata, const SyntaxTreeBasePtr& p) -> optional<string>
        {
            const string msg = "'python:package' is deprecated; use 'python:identifier' to remap modules instead";
            p->unit()->warning(metadata->file(), metadata->line(), Deprecated, msg);

            if (auto cont = dynamic_pointer_cast<Contained>(p); cont && cont->hasMetadata("python:identifier"))
            {
                return "A Slice element can only have one of 'python:package' and 'python:identifier' applied to it";
            }

            // If 'python:package' is applied to a module, it must be a top-level module.
            // Top-level modules are contained by the 'Unit'. Non-top-level modules are contained in 'Module's.
            if (auto mod = dynamic_pointer_cast<Module>(p); mod && !mod->isTopLevel())
            {
                return "the 'python:package' metadata can only be applied at the file level or to top-level modules";
            }
            return nullopt;
        },
    };
    knownMetadata.emplace("python:package", std::move(packageInfo));

    // "python:seq"
    // We support 3 arguments to this metadata: "default", "list", and "tuple".
    // We also allow users to omit the "seq" in the middle, ie. "python:seq:list" and "python:list" are equivalent.
    MetadataInfo seqInfo = {
        .validOn = {typeid(Sequence)},
        .acceptedArgumentKind = MetadataArgumentKind::SingleArgument,
        .validArgumentValues = {{"default", "list", "tuple"}},
        .acceptedContext = MetadataApplicationContext::DefinitionsAndTypeReferences,
    };
    knownMetadata.emplace("python:seq", std::move(seqInfo));
    MetadataInfo unqualifiedSeqInfo = {
        .validOn = {typeid(Sequence)},
        .acceptedArgumentKind = MetadataArgumentKind::NoArguments,
        .acceptedContext = MetadataApplicationContext::DefinitionsAndTypeReferences,
    };
    knownMetadata.emplace("python:default", unqualifiedSeqInfo);
    knownMetadata.emplace("python:list", unqualifiedSeqInfo);
    knownMetadata.emplace("python:tuple", std::move(unqualifiedSeqInfo));

    // Pass this information off to the parser's metadata validation logic.
    Slice::validateMetadata(unit, "python", std::move(knownMetadata));
}
