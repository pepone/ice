//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//

#include <IceUtil/OutputUtil.h>
#include <IceUtil/StringUtil.h>
#include <IceUtil/Functional.h>
#include <Slice/Parser.h>
#include <Slice/FileTracker.h>
#include <Slice/Util.h>

#include <iterator>

#include "Gen.h"

using namespace std;
using namespace Slice;
using namespace IceUtilInternal;

namespace
{

string
getClassResolverPrefix(const UnitPtr& p)
{
    DefinitionContextPtr dc = p->findDefinitionContext(p->topLevelFile());
    assert(dc);

    static const string classResolverPrefix = "swift:class-resolver-prefix:";
    string result = dc->findMetaData(classResolverPrefix);
    if(!result.empty())
    {
        result = result.substr(classResolverPrefix.size());
    }
    return result;
}

}

Gen::Gen(const string& base, const vector<string>& includePaths, const string& dir) :
    _out(false, true), // No break before opening block in Swift + short empty blocks
    _includePaths(includePaths)
{
    _fileBase = base;
    string::size_type pos = base.find_last_of("/\\");
    if(pos != string::npos)
    {
        _fileBase = base.substr(pos + 1);
    }

    string file = _fileBase + ".swift";

    if(!dir.empty())
    {
        file = dir + '/' + file;
    }

    _out.open(file.c_str());
    if(!_out)
    {
        ostringstream os;
        os << "cannot open `" << file << "': " << IceUtilInternal::errorToString(errno);
        throw FileException(__FILE__, __LINE__, os.str());
    }
    FileTracker::instance()->addFile(file);

    printHeader();
    printGeneratedHeader(_out, _fileBase + ".ice");

    _out << nl << "import Foundation";
}

Gen::~Gen()
{
    if(_out.isOpen())
    {
        _out << nl;
    }
}

void
Gen::generate(const UnitPtr& p)
{
    SwiftGenerator::validateMetaData(p);

    ImportVisitor importVisitor(_out);
    p->visit(&importVisitor, false);
    importVisitor.writeImports();

    TypesVisitor typesVisitor(_out);
    p->visit(&typesVisitor, false);

    ProxyVisitor proxyVisitor(_out);
    p->visit(&proxyVisitor, false);

    ValueVisitor valueVisitor(_out);
    p->visit(&valueVisitor, false);

    ObjectVisitor objectVisitor(_out);
    p->visit(&objectVisitor, false);

    ObjectExtVisitor objectExtVisitor(_out);
    p->visit(&objectExtVisitor, false);

    LocalObjectVisitor localObjectVisitor(_out);
    p->visit(&localObjectVisitor, false);
}

void
Gen::closeOutput()
{
    _out.close();
}

void
Gen::printHeader()
{
    static const char* header =
        "//\n"
        "// Copyright (c) ZeroC, Inc. All rights reserved.\n"
        "//\n";

    _out << header;
    _out << "//\n";
    _out << "// Ice version " << ICE_STRING_VERSION << "\n";
    _out << "//\n";
}

Gen::ImportVisitor::ImportVisitor(IceUtilInternal::Output& o) : out(o)
{
}

bool
Gen::ImportVisitor::visitModuleStart(const ModulePtr& p)
{
    //
    // Always import Ice module first if not building Ice
    //
    if(UnitPtr::dynamicCast(p->container()) && _imports.empty())
    {
        string swiftModule = getSwiftModule(p);
        if(swiftModule != "Ice")
        {
            addImport("Ice");
        }
    }

    //
    // Add PromiseKit import for interfaces and local interfaces which contain "async-oneway" metadata
    //
    if(p->hasNonLocalInterfaceDefs() || p->hasLocalClassDefsWithAsync())
    {
        addImport("PromiseKit");
    }

    return true;
}

bool
Gen::ImportVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    //
    // Add imports required for base classes
    //
    ClassList bases = p->bases();
    for(ClassList::const_iterator i = bases.begin(); i != bases.end(); ++i)
    {
        addImport(ContainedPtr::dynamicCast(*i), p);
    }

    //
    // Add imports required for data members
    //
    const DataMemberList allDataMembers = p->allDataMembers();
    for(DataMemberList::const_iterator i = allDataMembers.begin(); i != allDataMembers.end(); ++i)
    {
        addImport((*i)->type(), p);
    }

    //
    // Add imports required for operation parameters and return type
    //
    const OperationList operationList = p->allOperations();
    for(OperationList::const_iterator i = operationList.begin(); i != operationList.end(); ++i)
    {
        const TypePtr ret = (*i)->returnType();
        if(ret && ret->definitionContext())
        {
            addImport(ret, p);
        }

        const ParamDeclList paramList = (*i)->parameters();
        for(ParamDeclList::const_iterator j = paramList.begin(); j != paramList.end(); ++j)
        {
            addImport((*j)->type(), p);
        }
    }

    return false;
}

bool
Gen::ImportVisitor::visitStructStart(const StructPtr& p)
{
    //
    // Add imports required for data members
    //
    const DataMemberList dataMembers = p->dataMembers();
    for(DataMemberList::const_iterator i = dataMembers.begin(); i != dataMembers.end(); ++i)
    {
        addImport((*i)->type(), p);
    }

    return true;
}

bool
Gen::ImportVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    //
    // Add imports required for base exceptions
    //
    ExceptionPtr base = p->base();
    if(base)
    {
        addImport(ContainedPtr::dynamicCast(base), p);
    }

    //
    // Add imports required for data members
    //
    const DataMemberList allDataMembers = p->allDataMembers();
    for(DataMemberList::const_iterator i = allDataMembers.begin(); i != allDataMembers.end(); ++i)
    {
        addImport((*i)->type(), p);
    }
    return true;
}

void
Gen::ImportVisitor::visitSequence(const SequencePtr& seq)
{
    //
    // Add import required for the sequence element type
    //
    addImport(seq->type(), seq);
}

void
Gen::ImportVisitor::visitDictionary(const DictionaryPtr& dict)
{
    //
    // Add imports required for the dictionary key and value types
    //
    addImport(dict->keyType(), dict);
    addImport(dict->valueType(), dict);
}

void
Gen::ImportVisitor::writeImports()
{
    for(vector<string>::const_iterator i = _imports.begin(); i != _imports.end(); ++i)
    {
        out << nl << "import " << *i;
    }
}

void
Gen::ImportVisitor::addImport(const TypePtr& definition, const ContainedPtr& toplevel)
{
    if(!BuiltinPtr::dynamicCast(definition))
    {
        ModulePtr m1 = getTopLevelModule(definition);
        ModulePtr m2 = getTopLevelModule(toplevel);

        string swiftM1 = getSwiftModule(m1);
        string swiftM2 = getSwiftModule(m2);
        if(swiftM1 != swiftM2 && find(_imports.begin(), _imports.end(), swiftM1) == _imports.end())
        {
            _imports.push_back(swiftM1);
        }
    }
}

void
Gen::ImportVisitor::addImport(const ContainedPtr& definition, const ContainedPtr& toplevel)
{
    ModulePtr m1 = getTopLevelModule(definition);
    ModulePtr m2 = getTopLevelModule(toplevel);

    string swiftM1 = getSwiftModule(m1);
    string swiftM2 = getSwiftModule(m2);
    if(swiftM1 != swiftM2 && find(_imports.begin(), _imports.end(), swiftM1) == _imports.end())
    {
        _imports.push_back(swiftM1);
    }
}

void
Gen::ImportVisitor::addImport(const string& module)
{
    if(find(_imports.begin(), _imports.end(), module) == _imports.end())
    {
        _imports.push_back(module);
    }
}

Gen::TypesVisitor::TypesVisitor(IceUtilInternal::Output& o) : out(o)
{
}

bool
Gen::TypesVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal())
    {
        return false;
    }

    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = fixIdent(getUnqualified(getAbsolute(p), swiftModule));
    const string traits = fixIdent(getUnqualified(getAbsolute(p), swiftModule) + "Traits");

    ClassList allBases = p->allBases();
    StringList allIds;
#ifdef ICE_CPP11_COMPILER
    transform(allBases.begin(), allBases.end(), back_inserter(allIds),
              [](const ContainedPtr& it)
              {
                  return it->scoped();
              });
#else
    transform(allBases.begin(), allBases.end(), back_inserter(allIds), ::IceUtil::constMemFun(&Contained::scoped));
#endif
    allIds.push_back(p->scoped());
    allIds.push_back("::Ice::Object");
    allIds.sort();
    allIds.unique();

    ostringstream ids;

    ids << "[";
    for(StringList::const_iterator r = allIds.begin(); r != allIds.end(); ++r)
    {
        if(r != allIds.begin())
        {
            ids << ", ";
        }
        ids << "\"" << (*r) << "\"";

    }
    ids << "]";

    out << sp;
    out << nl << "/// Traits for Slice ";
    if(p->isInterface())
    {
        out << "interface ";
    }
    else
    {
        out << "class ";
    }
    out << '`' << name << "`.";
    out << nl << "public struct " << traits << ": " << getUnqualified("Ice.SliceTraits", swiftModule);
    out << sb;
    out << nl << "public static let staticIds = " << ids.str();
    out << nl << "public static let staticId = \"" << p->scoped() << '"';
    out << eb;

    return false;
}

bool
Gen::TypesVisitor::visitExceptionStart(const ExceptionPtr& p)
{
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = getUnqualified(getAbsolute(p), swiftModule);

    ExceptionPtr base = p->base();

    if(!p->isLocal())
    {
        const string prefix = getClassResolverPrefix(p->unit());

        //
        // For each UserException class we generate an extension in ClassResolver
        //
        ostringstream factory;
        factory << prefix;
        StringList parts = splitScopedName(p->scoped());
        for(StringList::const_iterator it = parts.begin(); it != parts.end();)
        {
            factory << (*it);
            if(++it != parts.end())
            {
                factory << "_";
            }
        }

        out << sp;
        out << nl << "/// :nodoc:";
        out << nl << "public class " << name << "_TypeResolver: "
            << getUnqualified("Ice.UserExceptionTypeResolver", swiftModule);
        out << sb;
        out << nl << "public override func type() -> " << getUnqualified("Ice.UserException.Type", swiftModule);
        out << sb;
        out << nl << "return " << fixIdent(name) << ".self";
        out << eb;
        out << eb;

        out << sp;
        out << nl << "public extension " << getUnqualified("Ice.ClassResolver", swiftModule);
        out << sb;
        out << nl << "@objc static func " <<  factory.str() << "() -> "
            << getUnqualified("Ice.UserExceptionTypeResolver", swiftModule);
        out << sb;
        out << nl << "return " << name << "_TypeResolver()";
        out << eb;
        out << eb;
    }

    out << sp;
    writeDocSummary(out, p);
    writeSwiftAttributes(out, p->getMetaData());
    out << nl << "open class " << fixIdent(name) << ": ";
    if(base)
    {
        out << fixIdent(getUnqualified(getAbsolute(base), swiftModule));
    }
    else if(p->isLocal())
    {
        out << getUnqualified("Ice.LocalException", swiftModule);
    }
    else
    {
        out << getUnqualified("Ice.UserException", swiftModule);
    }
    out << sb;

    const DataMemberList members = p->dataMembers();
    const DataMemberList allMembers = p->allDataMembers();
    const DataMemberList baseMembers = base ? base->allDataMembers() : DataMemberList();
    const DataMemberList optionalMembers = p->orderedOptionalDataMembers();

    StringPairList extraParams;
    if(p->isLocal())
    {
        extraParams.push_back(make_pair("file", "Swift.String = #file"));
        extraParams.push_back(make_pair("line", "Swift.Int = #line"));
    }

    writeMembers(out, members, p);

    const bool basePreserved = p->inheritsMetaData("preserve-slice");
    const bool preserved = p->hasMetaData("preserve-slice");

    if(!p->isLocal() && preserved && !basePreserved)
    {
        out << nl << "var _slicedData: Ice.SlicedData?";
    }

    bool rootClass = !base && !p->isLocal();
    if(rootClass || !members.empty())
    {
        writeDefaultInitializer(out, true, rootClass);
    }
    writeMemberwiseInitializer(out, members, baseMembers, allMembers, p, p->isLocal(), rootClass, extraParams);

    out << sp;
    out << nl << "/// Returns the Slice type ID of this exception.";
    out << nl << "///";
    out << nl << "/// - returns: `Swift.String` - the Slice type ID of this exception.";
    out << nl << "open override class func ice_staticId() -> Swift.String";
    out << sb;
    out << nl << "return \"" << p->scoped() << "\"";
    out << eb;

    if(p->isLocal())
    {
        out << sp;
        out << nl << "/// Returns a string representation of this exception";
        out << nl << "///";
        out << nl << "/// - returns: `Swift.String` - The string representaton of this exception.";
        out << nl << "open override func ice_print() -> Swift.String";
        out << sb;
        out << nl << "return _" << name << "Description";
        out << eb;
    }
    else
    {
        out << sp;
        out << nl << "open override func _iceWriteImpl(to ostr: "
            << getUnqualified("Ice.OutputStream", swiftModule) << ")";
        out << sb;
        out << nl << "ostr.startSlice(typeId: " << fixIdent(name) << ".ice_staticId(), compactId: -1, last: "
            << (!base ? "true" : "false") << ")";
        for(DataMemberList::const_iterator i = members.begin(); i != members.end(); ++i)
        {
            DataMemberPtr member = *i;
            if(!member->optional())
            {
                writeMarshalUnmarshalCode(out, member->type(), p, "self." + fixIdent(member->name()), true);
            }
        }

        for(DataMemberList::const_iterator i = optionalMembers.begin(); i != optionalMembers.end(); ++i)
        {
            DataMemberPtr member = *i;
            writeMarshalUnmarshalCode(out, member->type(), p, "self." + fixIdent(member->name()), true, member->tag());
        }
        out << nl << "ostr.endSlice()";
        if(base)
        {
            out << nl << "super._iceWriteImpl(to: ostr);";
        }
        out << eb;

        out << sp;
        out << nl << "open override func _iceReadImpl(from istr: "
            << getUnqualified("Ice.InputStream", swiftModule) << ") throws";
        out << sb;
        out << nl << "_ = try istr.startSlice()";
        for(DataMemberList::const_iterator i = members.begin(); i != members.end(); ++i)
        {
            DataMemberPtr member = *i;
            if(!member->optional())
            {
                writeMarshalUnmarshalCode(out, member->type(), p, "self." + fixIdent(member->name()), false);
            }
        }

        for(DataMemberList::const_iterator i = optionalMembers.begin(); i != optionalMembers.end(); ++i)
        {
            DataMemberPtr member = *i;
            writeMarshalUnmarshalCode(out, member->type(), p, "self." + fixIdent(member->name()), false, member->tag());
        }

        out << nl << "try istr.endSlice()";
        if(base)
        {
            out << nl << "try super._iceReadImpl(from: istr);";
        }
        out << eb;

        if(p->usesClasses(false) && (!base || (base && !base->usesClasses(false))))
        {
            out << sp;
            out << nl << "open override func _usesClasses() -> Swift.Bool" << sb;
            out << nl << "return true";
            out << eb;
        }

        if(preserved && !basePreserved)
        {
            out << sp;
            out << nl << "/// Returns the sliced data if the exception has a preserved-slice base class and has been";
            out << nl << "/// sliced during un-marshaling, nil is returned otherwise.";
            out << nl << "///";
            out << nl << "/// - returns: `Ice.SlicedData` - The sliced data.";
            out << nl << "open override func ice_getSlicedData() -> " << getUnqualified("Ice.SlicedData", swiftModule)
                << "?" << sb;
            out << nl << "return _slicedData";
            out << eb;

            out << sp;
            out << nl << "open override func _iceRead(from istr: " << getUnqualified("Ice.InputStream", swiftModule)
                << ") throws" << sb;
            out << nl << "istr.startException()";
            out << nl << "try _iceReadImpl(from: istr)";
            out << nl << "_slicedData = try istr.endException(preserve: true)";
            out << eb;

            out << sp;
            out << nl << "open override func _iceWrite(to ostr: " << getUnqualified("Ice.OutputStream", swiftModule)
                << ")" << sb;
            out << nl << "ostr.startException(data: _slicedData)";
            out << nl << "_iceWriteImpl(to: ostr)";
            out << nl << "ostr.endException()";
            out << eb;
        }
    }

    out << eb;
    return false;
}

bool
Gen::TypesVisitor::visitStructStart(const StructPtr& p)
{
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = fixIdent(getUnqualified(getAbsolute(p), swiftModule));
    bool containsSequence;
    bool legalKeyType = Dictionary::legalKeyType(p, containsSequence);
    const DataMemberList members = p->dataMembers();
    const string optionalFormat = getOptionalFormat(p);

    bool isClass = containsClassMembers(p);
    out << sp;
    writeDocSummary(out, p);
    writeSwiftAttributes(out, p->getMetaData());
    out << nl << "public " << (isClass ? "class " : "struct ") << name;
    if(legalKeyType)
    {
        out << ": Swift.Hashable";
    }
    out << sb;

    writeMembers(out, members, p);
    writeDefaultInitializer(out, false, true);
    writeMemberwiseInitializer(out, members, p);

    out << eb;

    if(!p->isLocal())
    {
        out << sp;
        out << nl << "/// An `Ice.InputStream` extension to read `" << name << "` structured values from the stream.";
        out << nl << "public extension " << getUnqualified("Ice.InputStream", swiftModule);
        out << sb;

        out << sp;
        out << nl << "/// Read a `" << name << "` structured value from the stream.";
        out << nl << "///";
        out << nl << "/// - returns: `" << name << "` - The structured value read from the stream.";
        out << nl << "func read() throws -> " << name;
        out << sb;
        out << nl << (isClass ? "let" : "var") << " v = " << name << "()";
        for(DataMemberList::const_iterator q = members.begin(); q != members.end(); ++q)
        {
            writeMarshalUnmarshalCode(out, (*q)->type(), p, "v." + fixIdent((*q)->name()), false);
        }
        out << nl << "return v";
        out << eb;

        out << sp;
        out << nl << "/// Read an optional `" << name << "?` structured value from the stream.";
        out << nl << "///";
        out << nl << "/// - parameter tag: `Swift.Int32` - The numeric tag associated with the value.";
        out << nl << "///";
        out << nl << "/// - returns: `" << name << "?` - The structured value read from the stream.";
        out << nl << "func read(tag: Swift.Int32) throws -> " << name << "?";
        out << sb;
        out << nl << "guard try readOptional(tag: tag, expectedFormat: " << optionalFormat << ") else";
        out << sb;
        out << nl << "return nil";
        out << eb;
        if(p->isVariableLength())
        {
            out << nl << "try skip(4)";
        }
        else
        {
            out << nl << "try skipSize()";
        }
        out << nl << "return try read() as " << name;
        out << eb;

        out << eb;

        out << sp;
        out << nl << "/// An `Ice.OutputStream` extension to write `" << name << "` structured values from the stream.";
        out << nl << "public extension " << getUnqualified("Ice.OutputStream", swiftModule);
        out << sb;

        out << nl << "/// Write a `" << name << "` structured value to the stream.";
        out << nl << "///";
        out << nl << "/// - parameter _: `" << name << "` - The value to write to the stream.";
        out << nl << "func write(_ v: " << name << ")" << sb;
        for(DataMemberList::const_iterator q = members.begin(); q != members.end(); ++q)
        {
            writeMarshalUnmarshalCode(out, (*q)->type(), p, "v." + fixIdent((*q)->name()), true);
        }
        out << eb;

        out << sp;
        out << nl << "/// Write an optional `" << name << "?` structured value to the stream.";
        out << nl << "///";
        out << nl << "/// - parameter tag: `Swift.Int32` - The numeric tag associated with the value.";
        out << nl << "///";
        out << nl << "/// - parameter value: `" << name << "?` - The value to write to the stream.";
        out << nl << "func write(tag: Swift.Int32, value: " << name << "?)" << sb;
        out << nl << "if let v = value" << sb;
        out << nl << "if writeOptional(tag: tag, format: " << optionalFormat << ")" << sb;

        if(p->isVariableLength())
        {
            out << nl << "let pos = startSize()";
            out << nl << "write(v)";
            out << nl << "endSize(position: pos)";
        }
        else
        {
            out << nl << "write(size: " << p->minWireSize() << ")";
            out << nl << "write(v)";
        }
        out << eb;
        out << eb;
        out << eb;

        out << eb;
    }

    return false;
}

void
Gen::TypesVisitor::visitSequence(const SequencePtr& p)
{
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = getUnqualified(getAbsolute(p), swiftModule);
    int typeCtx = p->isLocal() ? TypeContextLocal : 0;

    const TypePtr type = p->type();
    BuiltinPtr builtin = BuiltinPtr::dynamicCast(p->type());

    out << sp;
    writeDocSummary(out, p);
    out << nl << "public typealias " << fixIdent(name) << " = ";

    if(builtin && builtin->kind() == Builtin::KindByte)
    {
        out << "Foundation.Data";
    }
    else
    {
        out << "[" << typeToString(p->type(), p, p->getMetaData(), false, typeCtx) << "]";
    }

    if(p->isLocal())
    {
        return;
    }

    if(builtin && builtin->kind() <= Builtin::KindString)
    {
        return; // No helpers for sequence of primitive types
    }

    const string ostr = getUnqualified("Ice.OutputStream", swiftModule);
    const string istr = getUnqualified("Ice.InputStream", swiftModule);

    const string optionalFormat = getUnqualified(getOptionalFormat(p), swiftModule);

    out << sp;
    out << nl << "/// Helper class to read and write `" << fixIdent(name) << "` sequence values from";
    out << nl << "/// `Ice.InputStream` and `Ice.OutputStream`.";
    out << nl << "public struct " << name << "Helper";
    out << sb;

    out << nl << "/// Read a `" << fixIdent(name) << "` sequence from the stream.";
    out << nl << "///";
    out << nl << "/// - parameter istr: `Ice.InputStream` - The stream to read from.";
    out << nl << "///";
    out << nl << "/// - returns: `" << fixIdent(name) << "` - The sequence read from the stream.";
    out << nl << "public static func read(from istr: " << istr << ") throws -> "
        << fixIdent(name);
    out << sb;
    out << nl << "let sz = try istr.readAndCheckSeqSize(minSize: " << p->type()->minWireSize() << ")";

    if(isClassType(type))
    {
        out << nl << "var v = " << fixIdent(name) << "(repeating: nil, count: sz)";
        out << nl << "for i in 0 ..< sz";
        out << sb;
        out << nl << "try Swift.withUnsafeMutablePointer(to: &v[i])";
        out << sb;
        out << " p in";
        writeMarshalUnmarshalCode(out, type, p, "p.pointee", false);
        out << eb;
        out << eb;
    }
    else
    {
        out << nl << "var v = " << fixIdent(name) << "()";
        out << nl << "v.reserveCapacity(sz)";
        out << nl << "for _ in 0 ..< sz";
        out << sb;
        string param = "let j: " + typeToString(p->type(), p);
        writeMarshalUnmarshalCode(out, type, p, param, false);
        out << nl << "v.append(j)";
        out << eb;
    }
    out << nl << "return v";
    out << eb;

    out << nl << "/// Read an optional `" << fixIdent(name) << "?` sequence from the stream.";
    out << nl << "///";
    out << nl << "/// - parameter istr: `Ice.InputStream` - The stream to read from.";
    out << nl << "///";
    out << nl << "/// - parameter tag: `Swift.Int32` - The numeric tag associated with the value.";
    out << nl << "///";
    out << nl << "/// - returns: `" << fixIdent(name) << "` - The sequence read from the stream.";
    out << nl << "public static func read(from istr: " << istr << ", tag: Swift.Int32) throws -> "
        << fixIdent(name) << "?";
    out << sb;
    out << nl << "guard try istr.readOptional(tag: tag, expectedFormat: " << optionalFormat << ") else";
    out << sb;
    out << nl << "return nil";
    out << eb;
    if(p->type()->isVariableLength())
    {
        out << nl << "try istr.skip(4)";
    }
    else if(p->type()->minWireSize() > 1)
    {
        out << nl << "try istr.skipSize()";
    }
    out << nl << "return try read(from: istr)";
    out << eb;

    out << sp;
    out << nl << "/// Wite a `" << fixIdent(name) << "` sequence to the stream.";
    out << nl << "///";
    out << nl << "/// - parameter ostr: `Ice.OuputStream` - The stream to write to.";
    out << nl << "///";
    out << nl << "/// - parameter value: `" << fixIdent(name) << "` - The sequence value to write to the stream.";
    out << nl << "public static func write(to ostr: " << ostr << ", value v: " << fixIdent(name) << ")";
    out << sb;
    out << nl << "ostr.write(size: v.count)";
    out << nl << "for item in v";
    out << sb;
    writeMarshalUnmarshalCode(out, type,  p, "item", true);
    out << eb;
    out << eb;

    out << sp;
    out << nl << "/// Wite an optional `" << fixIdent(name) << "?` sequence to the stream.";
    out << nl << "///";
    out << nl << "/// - parameter ostr: `Ice.OuputStream` - The stream to write to.";
    out << nl << "///";
    out << nl << "/// - parameter tag: `Int32` - The numeric tag associated with the value.";
    out << nl << "///";
    out << nl << "/// - parameter value: `" << fixIdent(name) << "` The sequence value to write to the stream.";
    out << nl << "public static func write(to ostr: " << ostr << ",  tag: Swift.Int32, value v: "
        << fixIdent(name) << "?)";
    out << sb;
    out << nl << "guard let val = v else";
    out << sb;
    out << nl << "return";
    out << eb;
    if(p->type()->isVariableLength())
    {
        out << nl << "if ostr.writeOptional(tag: tag, format: " << optionalFormat << ")";
        out << sb;
        out << nl << "let pos = ostr.startSize()";
        out << nl << "write(to: ostr, value: val)";
        out << nl << "ostr.endSize(position: pos)";
        out << eb;
    }
    else
    {
        if(p->type()->minWireSize() == 1)
        {
            out << nl << "if ostr.writeOptional(tag: tag, format: .VSize)";
        }
        else
        {
            out << nl << "if ostr.writeOptionalVSize(tag: tag, len: val.count, elemSize: "
                << p->type()->minWireSize() << ")";
        }
        out << sb;
        out << nl << "write(to: ostr, value: val)";
        out << eb;
    }
    out << eb;

    out << eb;
}

void
Gen::TypesVisitor::visitDictionary(const DictionaryPtr& p)
{
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = getUnqualified(getAbsolute(p), swiftModule);
    int typeCtx = p->isLocal() ? TypeContextLocal : 0;

    const string keyType = typeToString(p->keyType(), p, p->keyMetaData(), false, typeCtx);
    const string valueType = typeToString(p->valueType(), p, p->valueMetaData(), false, typeCtx);
    out << sp;
    writeDocSummary(out, p);
    out << nl << "public typealias " << fixIdent(name) << " = [" << keyType << ": " << valueType << "]";

    if(p->isLocal())
    {
        return;
    }

    const string ostr = getUnqualified("Ice.OutputStream", swiftModule);
    const string istr = getUnqualified("Ice.InputStream", swiftModule);

    const string optionalFormat = getUnqualified(getOptionalFormat(p), swiftModule);
    const bool isVariableLength = p->keyType()->isVariableLength() || p->valueType()->isVariableLength();
    const size_t minWireSize = p->keyType()->minWireSize() + p->valueType()->minWireSize();

    out << sp;
    out << nl << "/// Helper class to read and write `" << fixIdent(name) << "` dictionary values from";
    out << nl << "/// `Ice.InputStream` and `Ice.OutputStream`.";
    out << nl << "public struct " << name << "Helper";
    out << sb;

    out << nl << "/// Read a `" << fixIdent(name) << "` dictionary from the stream.";
    out << nl << "///";
    out << nl << "/// - parameter istr: `Ice.InputStream` - The stream to read from.";
    out << nl << "///";
    out << nl << "/// - returns: `" << fixIdent(name) << "` - The dictionary read from the stream.";
    out << nl << "public static func read(from istr: " << istr << ") throws -> " << fixIdent(name);
    out << sb;
    out << nl << "let sz = try Swift.Int(istr.readSize())";
    out << nl << "var v = " << fixIdent(name) << "()";
    if(isClassType(p->valueType()))
    {
        out << nl << "let e = " << getUnqualified("Ice.DictEntryArray", swiftModule) << "<" << keyType << ", "
            << valueType << ">(size: sz)";
        out << nl << "for i in 0 ..< sz";
        out << sb;
        string keyParam = "let key: " + keyType;
        writeMarshalUnmarshalCode(out, p->keyType(), p, keyParam, false);
        out << nl << "v[key] = nil as " << valueType;
        out << nl << "Swift.withUnsafeMutablePointer(to: &v[key, default:nil])";
        out << sb;
        out << nl << "e.values[i] = Ice.DictEntry<" << keyType << ", " << valueType << ">("
            << "key: key, "
            << "value: $0)";
        out << eb;
        writeMarshalUnmarshalCode(out, p->valueType(), p, "e.values[i].value.pointee", false);
        out << eb;

        out << nl << "for i in 0..<sz" << sb;
        out << nl << "Swift.withUnsafeMutablePointer(to: &v[e.values[i].key, default:nil])";
        out << sb;
        out << nl << "e.values[i].value = $0";
        out << eb;
        out << eb;
    }
    else
    {
        out << nl << "for _ in 0 ..< sz";
        out << sb;
        string keyParam = "let key: " + keyType;
        writeMarshalUnmarshalCode(out, p->keyType(), p, keyParam, false);
        string valueParam = "let value: " + typeToString(p->valueType(), p);
        writeMarshalUnmarshalCode(out, p->valueType(), p, valueParam, false);
        out << nl << "v[key] = value";
        out << eb;
    }

    out << nl << "return v";
    out << eb;

    out << nl << "/// Read an optional `" << fixIdent(name) << "?` dictionary from the stream.";
    out << nl << "///";
    out << nl << "/// - parameter istr: `Ice.InputStream` - The stream to read from.";
    out << nl << "///";
    out << nl << "/// - parameter tag: `Int32` - The numeric tag associated with the value.";
    out << nl << "///";
    out << nl << "/// - returns: `" << fixIdent(name) << "` - The dictionary read from the stream.";
    out << nl << "public static func read(from istr: " << istr << ", tag: Swift.Int32) throws -> "
        << fixIdent(name) << "?";
    out << sb;
    out << nl << "guard try istr.readOptional(tag: tag, expectedFormat: " << optionalFormat << ") else";
    out << sb;
    out << nl << "return nil";
    out << eb;
    if(p->keyType()->isVariableLength() || p->valueType()->isVariableLength())
    {
        out << nl << "try istr.skip(4)";
    }
    else
    {
        out << nl << "try istr.skipSize()";
    }
    out << nl << "return try read(from: istr)";
    out << eb;

    out << sp;
    out << nl << "/// Wite a `" << fixIdent(name) << "` dictionary to the stream.";
    out << nl << "///";
    out << nl << "/// - parameter ostr: `Ice.OuputStream` - The stream to write to.";
    out << nl << "///";
    out << nl << "/// - parameter value: `" << fixIdent(name) << "` - The dictionary value to write to the stream.";
    out << nl << "public static func write(to ostr: " << ostr << ", value v: " << fixIdent(name) << ")";
    out << sb;
    out << nl << "ostr.write(size: v.count)";
    out << nl << "for (key, value) in v";
    out << sb;
    writeMarshalUnmarshalCode(out, p->keyType(), p, "key", true);
    writeMarshalUnmarshalCode(out, p->valueType(), p, "value", true);
    out << eb;
    out << eb;

    out << sp;
    out << nl << "/// Wite an optional `" << fixIdent(name) << "?` dictionary to the stream.";
    out << nl << "///";
    out << nl << "/// - parameter ostr: `Ice.OuputStream` - The stream to write to.";
    out << nl << "///";
    out << nl << "/// - parameter tag: `Int32` - The numeric tag associated with the value.";
    out << nl << "///";
    out << nl << "/// - parameter value: `" << fixIdent(name) << "` - The dictionary value to write to the stream.";
    out << nl << "public static func write(to ostr: " << ostr << ", tag: Swift.Int32, value v: "
        << fixIdent(name) << "?)";
    out << sb;
    out << nl << "guard let val = v else";
    out << sb;
    out << nl << "return";
    out << eb;
    if(isVariableLength)
    {
        out << nl << "if ostr.writeOptional(tag: tag, format: " << optionalFormat << ")";
        out << sb;
        out << nl << "let pos = ostr.startSize()";
        out << nl << "write(to: ostr, value: val)";
        out << nl << "ostr.endSize(position: pos)";
        out << eb;
    }
    else
    {
        out << nl << "if ostr.writeOptionalVSize(tag: tag, len: val.count, elemSize: " << minWireSize << ")";
        out << sb;
        out << nl << "write(to: ostr, value: val)";
        out << eb;
    }
    out << eb;

    out << eb;
}

void
Gen::TypesVisitor::visitEnum(const EnumPtr& p)
{
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = fixIdent(getUnqualified(getAbsolute(p), swiftModule));
    const EnumeratorList enumerators = p->enumerators();
    const string enumType = p->maxValue() <= 0xFF ? "Swift.UInt8" : "Swift.Int32";
    const string optionalFormat = getOptionalFormat(p);

    out << sp;
    writeDocSummary(out, p);
    writeSwiftAttributes(out, p->getMetaData());
    out << nl << "public enum " << name << ": " << enumType;
    out << sb;

    for(EnumeratorList::const_iterator en = enumerators.begin(); en != enumerators.end(); ++en)
    {
        StringList sl = splitComment((*en)->comment());
        out << nl << "/// " << fixIdent((*en)->name());
        if(!sl.empty())
        {
            out << " ";
            writeDocLines(out, sl, false);
        }
        out << nl << "case " << fixIdent((*en)->name()) << " = " << (*en)->value();
    }

    out << nl << "public init()";
    out << sb;
    out << nl << "self = ." << fixIdent((*enumerators.begin())->name());
    out << eb;

    out << eb;

    out << sp;
    out << nl << "/// An `Ice.InputStream` extension to read `" << name << "` enumerated values from the stream.";
    out << nl << "public extension " << getUnqualified("Ice.InputStream", swiftModule);
    out << sb;

    out << sp;
    out << nl << "/// Read an enumerated value.";
    out << nl << "///";
    out << nl << "/// - returns: `" << name << "` - The enumarated value.";
    out << nl << "func read() throws -> " << name;
    out << sb;
    out << nl << "let rawValue: " << enumType << " = try read(enumMaxValue: " << p->maxValue() << ")";
    out << nl << "guard let val = " << name << "(rawValue: rawValue) else";
    out << sb;
    out << nl << "throw " << getUnqualified("Ice.MarshalException", swiftModule) << "(reason: \"invalid enum value\")";
    out << eb;
    out << nl << "return val";
    out << eb;

    out << sp;
    out << nl << "/// Read an optional enumerated value from the stream.";
    out << nl << "///";
    out << nl << "/// - parameter tag: `Int32` - The numeric tag associated with the value.";
    out << nl << "///";
    out << nl << "/// - returns: `" << name << "` - The enumerated value.";
    out << nl << "func read(tag: Swift.Int32) throws -> " << name << "?";
    out << sb;
    out << nl << "guard try readOptional(tag: tag, expectedFormat: " << optionalFormat << ") else";
    out << sb;
    out << nl << "return nil";
    out << eb;
    out << nl << "return try read() as " << name;
    out << eb;

    out << eb;

    out << sp;
    out << nl << "/// An `Ice.OutputStream` extension to write `" << name << "` enumerated values to the stream.";
    out << nl << "public extension " << getUnqualified("Ice.OutputStream", swiftModule);
    out << sb;

    out << sp;
    out << nl << "/// Writes an enumerated value to the stream.";
    out << nl << "///";
    out << nl << "/// parameter _: `" << name << "` - The enumerator to write.";
    out << nl << "func write(_ v: " << name << ")";
    out << sb;
    out << nl << "write(enum: v.rawValue, maxValue: " << p->maxValue() << ")";
    out << eb;

    out << sp;
    out << nl << "/// Writes an optional enumerated value to the stream.";
    out << nl << "///";
    out << nl << "/// parameter tag: `Int32` - The numeric tag associated with the value.";
    out << nl << "///";
    out << nl << "/// parameter _: `" << name << "` - The enumerator to write.";
    out << nl << "func write(tag: Swift.Int32, value: " << name << "?)";
    out << sb;
    out << nl << "guard let v = value else";
    out << sb;
    out << nl << "return";
    out << eb;
    out << nl << "write(tag: tag, val: v.rawValue, maxValue: " << p->maxValue() << ")";
    out << eb;

    out << eb;
}

void
Gen::TypesVisitor::visitConst(const ConstPtr& p)
{
    const string name = fixIdent(p->name());
    const TypePtr type = p->type();
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));

    writeDocSummary(out, p);
    out << nl << "public let " << name << ": " << typeToString(type, p) << " = ";
    writeConstantValue(out, type, p->valueType(), p->value(), p->getMetaData(), swiftModule);
    out << nl;
}

Gen::ProxyVisitor::ProxyVisitor(::IceUtilInternal::Output& o) : out(o)
{
}

bool
Gen::ProxyVisitor::visitModuleStart(const ModulePtr& p)
{
    return p->hasNonLocalClassDefs();
}

void
Gen::ProxyVisitor::visitModuleEnd(const ModulePtr&)
{
}

bool
Gen::ProxyVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal() || (!p->isInterface() && p->allOperations().empty()))
    {
        return false;
    }

    ClassList bases = p->bases();
    bool hasBase = false;
    while(!bases.empty() && !hasBase)
    {
        ClassDefPtr baseClass = bases.front();
        if(!baseClass->isInterface() && baseClass->allOperations().empty())
        {
            // does not count
            bases.pop_front();
        }
        else
        {
            hasBase = true;
        }
    }

    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = getUnqualified(getAbsolute(p), swiftModule);
    const string traits = name + "Traits";
    const string prx = name + "Prx";
    const string prxI = name + "PrxI";

    out << sp;
    writeProxyDocSummary(out, p, swiftModule);
    out << nl << "public protocol " << prx << ":";
    if(!hasBase)
    {
        out << " " << getUnqualified("Ice.ObjectPrx", swiftModule);
    }
    else
    {
        for(ClassList::const_iterator i = bases.begin(); i != bases.end();)
        {
            out << " " << getUnqualified(getAbsolute(*i), swiftModule) << "Prx";
            if(++i != bases.end())
            {
                out << ",";
            }
        }
    }
    out << sb;
    out << eb;

    out << sp;
    out << nl;
    if(swiftModule == "Ice")
    {
        out << "internal ";
    }
    else
    {
        out << "private ";
    }
    out << "final class " << prxI << ": " << getUnqualified("Ice.ObjectPrxI", swiftModule) << ", " << prx;
    out << sb;

    out << nl << "public override class func ice_staticId() -> Swift.String";
    out << sb;
    out << nl << "return " << traits << ".staticId";
    out << eb;

    out << eb;

    //
    // checkedCast
    //
    out << sp;
    out << nl << "/// Casts a proxy to the requested type. This call contacts the server and verifies that the object";
    out << nl << "/// implements this type.";
    out << nl << "///";
    out << nl << "/// It will throw a local exception if a communication error occurs. You can optionally supply a";
    out << nl << "/// facet name and a context map.";
    out << nl << "///";
    out << nl << "/// - parameter prx: `Ice.ObjectPrx` - The proxy to be cast.";
    out << nl << "///";
    out << nl << "/// - parameter type: `" << prx << ".Protocol` - The proxy type to cast to.";
    out << nl << "///";
    out << nl << "/// - parameter facet: `String` - The optional name of the desired facet.";
    out << nl << "///";
    out << nl << "/// - parameter context: `Ice.Context` The optional context dictionary for the remote invocation.";
    out << nl << "///";
    out << nl << "/// - returns: `" << prx << "` - A proxy with the requested type or nil if the objet does not";
    out << nl << "///   support this type.";
    out << nl << "///";
    out << nl << "/// - throws: `Ice.LocalException` if a communication error occurs.";
    out << nl << "public func checkedCast" << spar
        << ("prx: " + getUnqualified("Ice.ObjectPrx", swiftModule))
        << ("type: " + prx + ".Protocol")
        << ("facet: Swift.String? = nil")
        << ("context: " + getUnqualified("Ice.Context", swiftModule) + "? = nil")
        << epar << " throws -> " << prx << "?";
    out << sb;
    out << nl << "return try " << prxI << ".checkedCast(prx: prx, facet: facet, context: context) as " << prxI << "?";
    out << eb;

    //
    // uncheckedCast
    //
    out << sp;
    out << nl << "/// Downcasts the given proxy to this type without contacting the remote server.";
    out << nl << "///";
    out << nl << "/// - parameter prx: `Ice.ObjectPrx` The proxy to be cast.";
    out << nl << "///";
    out << nl << "/// - parameter type: `" << prx << ".Protocol` - The proxy type to cast to.";
    out << nl << "///";
    out << nl << "/// - parameter facet: `String` - The optional name of the desired facet";
    out << nl << "///";
    out << nl << "/// - returns: `" << prx << "` - A proxy with the requested type";
    out << nl << "public func uncheckedCast" << spar
        << ("prx: " + getUnqualified("Ice.ObjectPrx", swiftModule))
        << ("type: " + prx + ".Protocol")
        << ("facet: Swift.String? = nil") << epar << " -> " << prx;
    out << sb;
    out << nl << "return " << prxI << ".uncheckedCast(prx: prx, facet: facet) as " << prxI;
    out << eb;

    //
    // ice_staticId
    //
    out << sp;
    out << nl << "/// Returns the Slice type id of the interface or class associated with this proxy type.";
    out << nl << "///";
    out << nl << "/// parameter type: `" << prx << ".Protocol` -  The proxy type to retrieve the type id.";
    out << nl << "///";
    out << nl << "/// returns: `String` - The type id of the interface or class associated with this proxy type.";
    out << nl << "public func ice_staticId" << spar << ("_ type: " + prx + ".Protocol") << epar << " -> Swift.String";
    out << sb;
    out << nl << "return " << traits << ".staticId";
    out << eb;

    //
    // InputStream extension
    //
    out << sp;
    out << nl << "/// Extension to `Ice.InputStream` class to support reading proxy of type";
    out << nl << "/// `" << prx << "`.";
    out << nl << "public extension " << getUnqualified("Ice.InputStream", swiftModule);
    out << sb;

    out << nl << "/// Extracts a proxy from the stream. The stream must have been initialized with a communicator.";
    out << nl << "///";
    out << nl << "/// - parameter type: `" << prx << ".Protocol` - The type of the proxy to be extracted.";
    out << nl << "///";
    out << nl << "/// - returns: `" << prx << "?` - The extracted proxy";
    out << nl << "func read(_ type: " << prx << ".Protocol) throws -> " << prx << "?";
    out << sb;
    out << nl << "return try read() as " << prxI << "?";
    out << eb;

    out << nl << "/// Extracts a proxy from the stream. The stream must have been initialized with a communicator.";
    out << nl << "///";
    out << nl << "/// - parameter tag: `Int32` - The numeric tag associated with the value.";
    out << nl << "///";
    out << nl << "/// - parameter type: `" << prx << ".Protocol` - The type of the proxy to be extracted.";
    out << nl << "///";
    out << nl << "/// - returns: `" << prx << "` - The extracted proxy.";
    out << nl << "func read(tag: Swift.Int32, type: " << prx << ".Protocol) throws -> " << prx << "?";
    out << sb;
    out << nl << "return try read(tag: tag) as " << prxI << "?";
    out << eb;

    out << eb;

    out << sp;
    writeProxyDocSummary(out, p, swiftModule);
    out << nl << "public extension " << prx;
    out << sb;

    return true;
}

void
Gen::ProxyVisitor::visitClassDefEnd(const ClassDefPtr&)
{
    out << eb;
}

void
Gen::ProxyVisitor::visitOperation(const OperationPtr& op)
{
    writeProxyOperation(out, op);
    writeProxyAsyncOperation(out, op);
}

Gen::ValueVisitor::ValueVisitor(::IceUtilInternal::Output& o) : out(o)
{
}

bool
Gen::ValueVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal() || p->isInterface())
    {
        return false;
    }

    const string prefix = getClassResolverPrefix(p->unit());
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = getUnqualified(getAbsolute(p), swiftModule);
    const string traits = name + "Traits";

    ClassList bases = p->bases();
    ClassDefPtr base;
    if(!bases.empty() && !bases.front()->isInterface())
    {
        base = bases.front();
    }

    out << sp;
    out << nl << "/// :nodoc:";
    out << nl << "public class " << name << "_TypeResolver: " << getUnqualified("Ice.ValueTypeResolver", swiftModule);
    out << sb;
    out << nl << "public override func type() -> " << getUnqualified("Ice.Value.Type", swiftModule);
    out << sb;
    out << nl << "return " << fixIdent(name) << ".self";
    out << eb;
    out << eb;

    if(p->compactId() >= 0)
    {
        //
        // For each Value class using a compact id we generate an extension
        // method in TypeIdResolver.
        //
        out << sp;
        out << nl << "public extension " << getUnqualified("Ice.TypeIdResolver", swiftModule);
        out << sb;
        out << nl << "@objc static func TypeId_" << p->compactId() << "() -> Swift.String";
        out << sb;
        out << nl << "return \"" << p->scoped() << "\"";
        out << eb;
        out << eb;
    }

    //
    // For each Value class we generate an extension method in ClassResolver
    //
    ostringstream factory;
    factory << prefix;
    StringList parts = splitScopedName(p->scoped());
    for(StringList::const_iterator it = parts.begin(); it != parts.end();)
    {
        factory << (*it);
        if(++it != parts.end())
        {
            factory << "_";
        }
    }

    out << sp;
    out << nl << "public extension " << getUnqualified("Ice.ClassResolver", swiftModule);
    out << sb;
    out << nl << "@objc static func " << factory.str() << "() -> "
        << getUnqualified("Ice.ValueTypeResolver", swiftModule);
    out << sb;
    out << nl << "return " << name << "_TypeResolver()";
    out << eb;
    out << eb;

    out << sp;
    writeDocSummary(out, p);
    writeSwiftAttributes(out, p->getMetaData());
    out << nl << "open class " << fixIdent(name) << ": ";
    if(base)
    {
        out << fixIdent(getUnqualified(getAbsolute(base), swiftModule));
    }
    else
    {
        out << getUnqualified("Ice.Value", swiftModule);
    }
    out << sb;

    const DataMemberList members = p->dataMembers();
    const DataMemberList baseMembers = base ? base->allDataMembers() : DataMemberList();
    const DataMemberList allMembers = p->allDataMembers();
    const DataMemberList optionalMembers = p->orderedOptionalDataMembers();

    const bool basePreserved = p->inheritsMetaData("preserve-slice");
    const bool preserved = p->hasMetaData("preserve-slice");

    writeMembers(out, members, p);
    if(!p->isLocal() && preserved && !basePreserved)
    {
        out << nl << "var _slicedData: " << getUnqualified("Ice.SlicedData?", swiftModule);
    }

    if(!base || !members.empty())
    {
        writeDefaultInitializer(out, true, !base);
    }
    writeMemberwiseInitializer(out, members, baseMembers, allMembers, p, p->isLocal(), !base);

    out << sp;
    out << nl << "/// Returns the Slice type ID of the most-derived interface supported by this object.";
    out << nl << "///";
    out << nl << "/// - returns: `String` - The Slice type ID of the most-derived interface supported by this object";
    out << nl << "open override func ice_id() -> Swift.String" << sb;
    out << nl << "return " << traits << ".staticId";
    out << eb;

    out << sp;
    out << nl << "/// Returns the Slice type ID of the interface supported by this object.";
    out << nl << "///";
    out << nl << "/// - returns: `String` - The Slice type ID of the interface supported by this object.";
    out << nl << "open override class func ice_staticId() -> Swift.String" << sb;
    out << nl << "return " << traits << ".staticId";
    out << eb;

    out << sp;
    out << nl << "open override func _iceReadImpl(from istr: "
        << getUnqualified("Ice.InputStream", swiftModule) << ") throws";
    out << sb;
    out << nl << "_ = try istr.startSlice()";
    for(DataMemberList::const_iterator i = members.begin(); i != members.end(); ++i)
    {
        DataMemberPtr member = *i;
        if(!member->optional())
        {
            writeMarshalUnmarshalCode(out, member->type(), p, "self." + fixIdent(member->name()), false);
        }
    }
    for(DataMemberList::const_iterator d = optionalMembers.begin(); d != optionalMembers.end(); ++d)
    {
        writeMarshalUnmarshalCode(out, (*d)->type(), p, "self." + fixIdent((*d)->name()), false, (*d)->tag());
    }
    out << nl << "try istr.endSlice()";
    if(base)
    {
        out << nl << "try super._iceReadImpl(from: istr);";
    }
    out << eb;

    out << sp;
    out << nl << "open override func _iceWriteImpl(to ostr: "
        << getUnqualified("Ice.OutputStream", swiftModule) << ")";
    out << sb;
    out << nl << "ostr.startSlice(typeId: " << name << "Traits.staticId, compactId: " << p->compactId()
        << ", last: " << (!base ? "true" : "false") << ")";
    for(DataMemberList::const_iterator i = members.begin(); i != members.end(); ++i)
    {
        DataMemberPtr member = *i;
        TypePtr type = member->type();
        if(!member->optional())
        {
            writeMarshalUnmarshalCode(out, member->type(), p, "self." + fixIdent(member->name()), true);
        }
    }
    for(DataMemberList::const_iterator d = optionalMembers.begin(); d != optionalMembers.end(); ++d)
    {
        writeMarshalUnmarshalCode(out, (*d)->type(), p, "self." + fixIdent((*d)->name()), true, (*d)->tag());
    }
    out << nl << "ostr.endSlice()";
    if(base)
    {
        out << nl << "super._iceWriteImpl(to: ostr);";
    }
    out << eb;

    if(preserved && !basePreserved)
    {
        out << sp;
        out << nl << "/// Returns the sliced data if the value has a preserved-slice base class and has been sliced";
        out << nl << "/// during un-marshaling of the value, nil is returned otherwise.";
        out << nl << "///";
        out << nl << "/// returns: `Ice.SlicedData?` - The sliced data or nil";
        out << nl << "open override func ice_getSlicedData() -> " << getUnqualified("Ice.SlicedData?", swiftModule)
            << sb;
        out << nl << "return _slicedData";
        out << eb;

        out << sp;
        out << nl << "open override func _iceRead(from istr: " << getUnqualified("Ice.InputStream", swiftModule)
            << ") throws" << sb;
        out << nl << "istr.startValue()";
        out << nl << "try _iceReadImpl(from: istr)";
        out << nl << "_slicedData = try istr.endValue(preserve: true)";
        out << eb;

        out << sp;
        out << nl << "open override func _iceWrite(to ostr: " << getUnqualified("Ice.OutputStream", swiftModule)
            << ")" << sb;
        out << nl << "ostr.startValue(data: _slicedData)";
        out << nl << "_iceWriteImpl(to: ostr)";
        out << nl << "ostr.endValue()";
        out << eb;
    }

    return true;
}

void
Gen::ValueVisitor::visitClassDefEnd(const ClassDefPtr&)
{
    out << eb;
}

void
Gen::ValueVisitor::visitOperation(const OperationPtr&)
{
}

Gen::ObjectVisitor::ObjectVisitor(::IceUtilInternal::Output& o) : out(o)
{
}

bool
Gen::ObjectVisitor::visitModuleStart(const ModulePtr&)
{
    return true;
}

void
Gen::ObjectVisitor::visitModuleEnd(const ModulePtr&)
{
}

bool
Gen::ObjectVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal() || (!p->isInterface() && p->allOperations().empty()))
    {
        return false;
    }

    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string disp = fixIdent(getUnqualified(getAbsolute(p), swiftModule) + "Disp");
    const string traits = fixIdent(getUnqualified(getAbsolute(p), swiftModule) + "Traits");
    const string servant = fixIdent(getUnqualified(getAbsolute(p), swiftModule) +
                                    (p->isInterface() ? "" : "Operations"));

    //
    // Disp struct
    //
    out << sp;
    out << sp;
    out << nl << "/// Dispatcher for `" << servant << "` servants.";
    out << nl << "public struct " << disp << ": " << getUnqualified("Ice.Disp", swiftModule);
    out << sb;
    out << nl << "public let servant: " << servant;

    out << nl << "private static let defaultObject = " << getUnqualified("Ice.ObjectI", swiftModule)
        << "<" << traits << ">()";

    out << sp;
    out << nl << "public init(_ servant: " << servant << ")";
    out << sb;
    out << nl << "self.servant = servant";
    out << eb;

    const OperationList allOps = p->allOperations();

    StringList allOpNames;
#ifdef ICE_CPP11_COMPILER
    transform(allOps.begin(), allOps.end(), back_inserter(allOpNames),
              [](const ContainedPtr& it)
              {
                  return it->name();
              });
#else
    transform(allOps.begin(), allOps.end(), back_inserter(allOpNames), ::IceUtil::constMemFun(&Contained::name));
#endif

    allOpNames.push_back("ice_id");
    allOpNames.push_back("ice_ids");
    allOpNames.push_back("ice_isA");
    allOpNames.push_back("ice_ping");
    allOpNames.sort();
    allOpNames.unique();

    out << sp;
    out << nl;
    out << "public func dispatch";
    out << spar;
    out << ("request: " + getUnqualified("Ice.Request", swiftModule));
    out << ("current: " + getUnqualified("Ice.Current", swiftModule));
    out << epar;
    out << " throws -> PromiseKit.Promise<" << getUnqualified("Ice.OutputStream", swiftModule) << ">?";

    out << sb;
    // Call startOver() so that dispatch interceptors can retry requests
    out << nl << "request.startOver()";
    out << nl << "switch current.operation";
    out << sb;
    out.dec(); // to align case with switch
    for(StringList::const_iterator q = allOpNames.begin(); q != allOpNames.end(); ++q)
    {
        const string opName = *q;
        out << nl << "case \"" << opName << "\":";
        out.inc();
        if(opName == "ice_id" || opName == "ice_ids" || opName == "ice_isA" || opName == "ice_ping")
        {
            out << nl << "return try (servant as? Object ?? " << disp << ".defaultObject)._iceD_"
                << opName << "(incoming: request, current: current)";
        }
        else
        {
            out << nl << "return try servant._iceD_" << opName << "(incoming: request, current: current)";
        }
        out.dec();
    }
    out << nl << "default:";
    out.inc();
    out << nl << "throw " << getUnqualified("Ice.OperationNotExistException", swiftModule)
        << "(id: current.id, facet: current.facet, operation: current.operation)";
    // missing dec to compensate for the extra dec after switch sb
    out << eb;
    out << eb;
    out << eb;

    //
    // Protocol
    //
    ClassList bases = p->bases();
    bool hasBase = false;
    while(!bases.empty() && !hasBase)
    {
        ClassDefPtr baseClass = bases.front();
        if(!baseClass->isInterface() && baseClass->allOperations().empty())
        {
            // does not count
            bases.pop_front();
        }
        else
        {
            hasBase = true;
        }
    }

    StringList baseNames;
    for(ClassList::const_iterator i = bases.begin(); i != bases.end(); ++i)
    {
        baseNames.push_back(fixIdent(getUnqualified(getAbsolute(*i), swiftModule) +
                                     ((*i)->isInterface() ? "" : "Operations")));
    }

    //
    // Check for swift:inherits metadata.
    //
    const StringList metaData = p->getMetaData();
    static const string prefix = "swift:inherits:";
    for(StringList::const_iterator q = metaData.begin(); q != metaData.end(); ++q)
    {
        if(q->find(prefix) == 0)
        {
            baseNames.push_back(q->substr(prefix.size()));
        }
    }

    out << sp;
    writeDocSummary(out, p);
    out << nl << "public protocol " << servant;
    if(!baseNames.empty())
    {
        out << ":";
    }

    for(StringList::const_iterator i = baseNames.begin(); i != baseNames.end();)
    {
        out << " " << (*i);
        if(++i != baseNames.end())
        {
            out << ",";
        }
    }

    out << sb;

    return true;
}

void
Gen::ObjectVisitor::visitClassDefEnd(const ClassDefPtr&)
{
    out << eb;
}

void
Gen::ObjectVisitor::visitOperation(const OperationPtr& op)
{
    const bool isAmd = operationIsAmd(op);
    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(op)));
    const string opName = fixIdent(op->name() + (isAmd ? "Async" : ""));
    const ParamInfoList allInParams = getAllInParams(op);
    const ParamInfoList allOutParams = getAllOutParams(op);
    const ExceptionList allExceptions = op->throws();

    out << sp;
    writeOpDocSummary(out, op, isAmd, true);
    out << nl << "func " << opName;
    out << spar;
    for(ParamInfoList::const_iterator q = allInParams.begin(); q != allInParams.end(); ++q)
    {
        ostringstream s;
        s << q->name << ": " << q->typeStr;
        out << s.str();
    }
    out << ("current: " + getUnqualified("Ice.Current", swiftModule));
    out << epar;

    if(isAmd)
    {
        out << " -> PromiseKit.Promise<" << (allOutParams.size() > 0 ? operationReturnType(op) : "Swift.Void") << ">";
    }
    else
    {
        out << " throws";
        if(allOutParams.size() > 0)
        {
            out << " -> " << operationReturnType(op);
        }
    }
}

Gen::ObjectExtVisitor::ObjectExtVisitor(::IceUtilInternal::Output& o) : out(o)
{
}

bool
Gen::ObjectExtVisitor::visitModuleStart(const ModulePtr&)
{
    return true;
}

void
Gen::ObjectExtVisitor::visitModuleEnd(const ModulePtr&)
{
}

bool
Gen::ObjectExtVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(p->isLocal() || (!p->isInterface() && p->allOperations().empty()))
    {
        return false;
    }

    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = getUnqualified(getAbsolute(p), swiftModule) + (p->isInterface() ? "" : "Operations");

    ClassList allBases = p->allBases();

    out << sp;
    writeServantDocSummary(out, p, swiftModule);
    out << nl << "public extension " << fixIdent(name);

    out << sb;
    return true;
}

void
Gen::ObjectExtVisitor::visitClassDefEnd(const ClassDefPtr&)
{
    out << eb;
}

void
Gen::ObjectExtVisitor::visitOperation(const OperationPtr& op)
{
    if(operationIsAmd(op))
    {
        writeDispatchAsyncOperation(out, op);
    }
    else
    {
        writeDispatchOperation(out, op);
    }
}

Gen::LocalObjectVisitor::LocalObjectVisitor(::IceUtilInternal::Output& o) : out(o)
{
}

bool
Gen::LocalObjectVisitor::visitModuleStart(const ModulePtr&)
{
    return true;
}

void
Gen::LocalObjectVisitor::visitModuleEnd(const ModulePtr&)
{
}

bool
Gen::LocalObjectVisitor::visitClassDefStart(const ClassDefPtr& p)
{
    if(!p->isLocal())
    {
        return false;
    }

    const string swiftModule = getSwiftModule(getTopLevelModule(ContainedPtr::dynamicCast(p)));
    const string name = getUnqualified(getAbsolute(p), swiftModule);

    if(p->isDelegate())
    {
        OperationPtr op = p->allOperations().front();
        const ParamDeclList params = op->parameters();

        out << sp;
        writeDocSummary(out, p);
        out << nl << "///";
        writeOpDocSummary(out, op, false, false, true);
        out << nl << "public typealias " << name << " = ";
        out << spar;
        for(ParamDeclList::const_iterator i = params.begin(); i != params.end(); ++i)
        {
            ParamDeclPtr param = *i;
            if(!param->isOutParam())
            {
                TypePtr type = param->type();
                ostringstream s;
                s << typeToString(type, p, param->getMetaData(), param->optional(), TypeContextLocal);
                out << s.str();
            }
        }
        out << epar;
        if(!op->hasMetaData("swift:noexcept"))
        {
            out << " throws";
        }
        out << " -> ";

        TypePtr ret = op->returnType();
        ParamDeclList outParams = op->outParameters();

        if(ret || !outParams.empty())
        {
            if(outParams.empty())
            {
                out << typeToString(ret, op, op->getMetaData(), op->returnIsOptional(), TypeContextLocal);
            }
            else if(!ret && outParams.size() == 1)
            {
                ParamDeclPtr param = outParams.front();
                out << typeToString(param->type(), op, param->getMetaData(), param->optional(), TypeContextLocal);
            }
            else
            {
                string returnValueS = "returnValue";
                for(ParamDeclList::const_iterator i = outParams.begin(); i != outParams.end(); ++i)
                {
                    ParamDeclPtr param = *i;
                    if(param->name() == "returnValue")
                    {
                        returnValueS = "_returnValue";
                        break;
                    }
                }

                out << spar;
                out << (returnValueS + ": " + typeToString(ret, op, op->getMetaData(), op->returnIsOptional(),
                                                           TypeContextLocal));
                for(ParamDeclList::const_iterator i = outParams.begin(); i != outParams.end(); ++i)
                {
                    ParamDeclPtr param = *i;
                    out << (fixIdent(param->name()) + ": " +
                            typeToString(param->type(), op, op->getMetaData(), param->optional(), TypeContextLocal));
                }
                out << epar;
            }
        }
        else
        {
            out << "Swift.Void";
        }
        return false;
    }

    ClassList bases = p->bases();
    StringList baseNames;
    if(bases.empty())
    {
        baseNames.push_back(" Swift.AnyObject");
    }
    else
    {
        for(ClassList::const_iterator i = bases.begin(); i != bases.end(); ++i)
        {
            baseNames.push_back(getUnqualified(getAbsolute(*i), swiftModule));
        }
    }

    //
    // Check for swift:inherits metadata.
    //
    const StringList metaData = p->getMetaData();
    static const string prefix = "swift:inherits:";
    for(StringList::const_iterator q = metaData.begin(); q != metaData.end(); ++q)
    {
        if(q->find(prefix) == 0)
        {
            baseNames.push_back(q->substr(prefix.size()));
        }
    }

    //
    // Local interfaces and local classes map to Swift protocol
    //
    out << sp;
    writeDocSummary(out, p);
    out << nl << "public protocol " << fixIdent(name) << ":";

    for(StringList::const_iterator i = baseNames.begin(); i != baseNames.end();)
    {
        out << " " << (*i);
        if(++i != baseNames.end())
        {
            out << ",";
        }
    }

    out << sb;
    writeMembers(out, p->dataMembers(), p, TypeContextProtocol | TypeContextLocal);
    return true;
}

void
Gen::LocalObjectVisitor::visitClassDefEnd(const ClassDefPtr&)
{
    out << eb;
}

void
Gen::LocalObjectVisitor::visitOperation(const OperationPtr& p)
{
    const string name = fixIdent(p->name());
    ParamDeclList params = p->parameters();
    ParamDeclList inParams = p->inParameters();

    int typeCtx = TypeContextInParam | TypeContextLocal;

    out << sp;
    writeOpDocSummary(out, p, false, false, true);
    writeSwiftAttributes(out, p->getMetaData());
    out << nl << "func " << name;
    out << spar;
    for(ParamDeclList::const_iterator i = inParams.begin(); i != inParams.end(); ++i)
    {
        ParamDeclPtr param = *i;
        TypePtr type = param->type();
        ostringstream s;
        if(inParams.size() == 1)
        {
            s << "_ ";
        }
        s << param->name() << ": "
          << typeToString(type, p, param->getMetaData(), param->optional(), typeCtx);
        out << s.str();
    }
    out << epar;

    if(!p->hasMetaData("swift:noexcept"))
    {
        out << " throws";
    }

    TypePtr ret = p->returnType();
    ParamDeclList outParams = p->outParameters();

    if(ret || !outParams.empty())
    {
        out << " -> ";
        if(outParams.empty())
        {
            out << typeToString(ret, p, p->getMetaData(), p->returnIsOptional(), typeCtx);
        }
        else if(!ret && outParams.size() == 1)
        {
            ParamDeclPtr param = outParams.front();
            out << typeToString(param->type(), p, param->getMetaData(), param->optional(), typeCtx);
        }
        else
        {
            string returnValueS = "returnValue";
            for(ParamDeclList::const_iterator i = outParams.begin(); i != outParams.end(); ++i)
            {
                ParamDeclPtr param = *i;
                if(param->name() == "returnValue")
                {
                    returnValueS = "_returnValue";
                    break;
                }
            }

            out << spar;
            out << (returnValueS + ": " + typeToString(ret, p, p->getMetaData(), p->returnIsOptional(), typeCtx));
            for(ParamDeclList::const_iterator i = outParams.begin(); i != outParams.end(); ++i)
            {
                ParamDeclPtr param = *i;
                out << (fixIdent(param->name()) + ": " +
                        typeToString(param->type(), p, p->getMetaData(), param->optional(), typeCtx));
            }
            out << epar;
        }
    }

    if(p->hasMetaData("async-oneway"))
    {
        out << sp;
        writeOpDocSummary(out, p, true, false, true);
        out << nl << "func " << name << "Async";
        out << spar;
        for(ParamDeclList::const_iterator i = inParams.begin(); i != inParams.end(); ++i)
        {
            ParamDeclPtr param = *i;
            TypePtr type = param->type();
            ostringstream s;
            if(inParams.size() == 1)
            {
                s << "_ ";
            }
            s << fixIdent(param->name()) << ": "
              << typeToString(type, p, param->getMetaData(), param->optional(), typeCtx);
            out << s.str();
        }
        out << "sentOn: Dispatch.DispatchQueue?";
        out << "sentFlags: Dispatch.DispatchWorkItemFlags?";
        out << "sent: ((Swift.Bool) -> Swift.Void)?";
        out << epar;

        out << " -> ";

        assert(!ret && outParams.empty());
        out << "PromiseKit.Promise<Swift.Void>";
    }
}
