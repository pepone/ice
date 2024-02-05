//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef GEN_H
#define GEN_H

#include <Slice/Parser.h>
#include <IceUtil/OutputUtil.h>

namespace Slice
{

class Gen : private ::IceUtil::noncopyable
{
public:

    Gen(const std::string&,
        const std::string&,
        const std::string&,
        const std::vector<std::string>&,
        const std::string&,
        const std::vector<std::string>&,
        const std::string&,
        const std::string&);
    ~Gen();

    void generate(const UnitPtr&);
    void closeOutput();

    static int setUseWstring(ContainedPtr, std::list<int>&, int);
    static int resetUseWstring(std::list<int>&);

private:

    void writeExtraHeaders(::IceUtilInternal::Output&);

    //
    // Returns the header extension defined in the file metadata for a given file,
    // or an empty string if no file metadata was found.
    //
    std::string getHeaderExt(const std::string& file, const UnitPtr& unit);

    //
    // Returns the source extension defined in the file metadata for a given file,
    // or an empty string if no file metadata was found.
    //
    std::string getSourceExt(const std::string& file, const UnitPtr& unit);

    ::IceUtilInternal::Output H;
    ::IceUtilInternal::Output C;

    std::string _base;
    std::string _headerExtension;
    std::string _sourceExtension;
    std::vector<std::string> _extraHeaders;
    std::string _include;
    std::vector<std::string> _includePaths;
    std::string _dllExport;
    std::string _dir;

    // Visitors

    class DeclVisitor : private ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

        DeclVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

        virtual bool visitUnitStart(const UnitPtr&);
        virtual void visitUnitEnd(const UnitPtr&);
        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual void visitClassDecl(const ClassDeclPtr&);
        virtual bool visitClassDefStart(const ClassDefPtr&);
        virtual void visitInterfaceDecl(const InterfaceDeclPtr&);
        virtual bool visitInterfaceDefStart(const InterfaceDefPtr&);
        virtual bool visitExceptionStart(const ExceptionPtr&);
        virtual void visitOperation(const OperationPtr&);

    private:

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllExport;
    };

    class TypesVisitor : private ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

        TypesVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual bool visitClassDefStart(const ClassDefPtr&) { return false; }
        virtual bool visitExceptionStart(const ExceptionPtr&);
        virtual void visitExceptionEnd(const ExceptionPtr&);
        virtual bool visitStructStart(const StructPtr&);
        virtual void visitStructEnd(const StructPtr&);
        virtual void visitSequence(const SequencePtr&);
        virtual void visitDictionary(const DictionaryPtr&);
        virtual void visitEnum(const EnumPtr&);
        virtual void visitConst(const ConstPtr&);
        virtual void visitDataMember(const DataMemberPtr&);

    private:

        void emitUpcall(const ExceptionPtr&, const std::string&, const std::string&);

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllExport;
        std::string _dllClassExport;
        std::string _dllMemberExport;
        bool _doneStaticSymbol;
        int _useWstring;
        std::list<int> _useWstringHist;
    };

    class ProxyVisitor : private ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

        ProxyVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual bool visitInterfaceDefStart(const InterfaceDefPtr&);
        virtual void visitInterfaceDefEnd(const InterfaceDefPtr&);
        virtual void visitOperation(const OperationPtr&);

    private:

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllClassExport;
        std::string _dllMemberExport;
        int _useWstring;
        std::list<int> _useWstringHist;
    };

    class ObjectVisitor : public ParserVisitor
    {
    public:

        ObjectVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

    protected:

        bool emitBaseInitializers(const ClassDefPtr&);
        void emitOneShotConstructor(const ClassDefPtr&);
        void emitDataMember(const DataMemberPtr&);

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllExport;
        std::string _dllClassExport;
        std::string _dllMemberExport;
        bool _doneStaticSymbol;
        int _useWstring;
        std::list<int> _useWstringHist;
    };

    class InterfaceVisitor : private ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

        InterfaceVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual bool visitInterfaceDefStart(const InterfaceDefPtr&);
        virtual void visitInterfaceDefEnd(const InterfaceDefPtr&);
        virtual void visitOperation(const OperationPtr&);
        void emitUpcall(const ClassDefPtr&, const std::string&, const std::string&);

    private:

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;

        std::string _dllExport;
        std::string _dllClassExport;
        std::string _dllMemberExport;
        int _useWstring;
        std::list<int> _useWstringHist;
    };

    class ValueVisitor : private ::IceUtil::noncopyable, public ObjectVisitor
    {
    public:

        ValueVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual bool visitClassDefStart(const ClassDefPtr&);
        virtual void visitClassDefEnd(const ClassDefPtr&);
        void emitUpcall(const ClassDefPtr&, const std::string&, const std::string&);
    };

    class StreamVisitor : private ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

        StreamVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual bool visitStructStart(const StructPtr&);
        virtual bool visitClassDefStart(const ClassDefPtr&);
        virtual bool visitExceptionStart(const ExceptionPtr&);
        virtual void visitExceptionEnd(const ExceptionPtr&);
        virtual void visitEnum(const EnumPtr&);

    private:

        ::IceUtilInternal::Output& H;
        ::IceUtilInternal::Output& C;
        std::string _dllExport;
    };

    class CompatibilityVisitor : private ::IceUtil::noncopyable, public ParserVisitor
    {
    public:

        CompatibilityVisitor(::IceUtilInternal::Output&, ::IceUtilInternal::Output&, const std::string&);

        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual void visitClassDecl(const ClassDeclPtr&);
        virtual void visitInterfaceDecl(const InterfaceDeclPtr&);

    private:

        ::IceUtilInternal::Output& H;
        std::string _dllExport;
    };

private:

    class MetaDataVisitor : public ParserVisitor
    {
    public:

        virtual bool visitUnitStart(const UnitPtr&);
        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual void visitClassDecl(const ClassDeclPtr&);
        virtual bool visitClassDefStart(const ClassDefPtr&);
        virtual void visitClassDefEnd(const ClassDefPtr&);
        virtual bool visitExceptionStart(const ExceptionPtr&);
        virtual void visitExceptionEnd(const ExceptionPtr&);
        virtual bool visitStructStart(const StructPtr&);
        virtual void visitStructEnd(const StructPtr&);
        virtual void visitOperation(const OperationPtr&);
        virtual void visitDataMember(const DataMemberPtr&);
        virtual void visitSequence(const SequencePtr&);
        virtual void visitDictionary(const DictionaryPtr&);
        virtual void visitEnum(const EnumPtr&);
        virtual void visitConst(const ConstPtr&);

    private:

        StringList validate(const SyntaxTreeBasePtr&, const StringList&, const std::string&, const std::string&,
                            bool = false);
    };

    class NormalizeMetaDataVisitor : public ParserVisitor
    {
    public:

        NormalizeMetaDataVisitor(bool);

        virtual bool visitUnitStart(const UnitPtr&);
        virtual bool visitModuleStart(const ModulePtr&);
        virtual void visitModuleEnd(const ModulePtr&);
        virtual void visitClassDecl(const ClassDeclPtr&);
        virtual bool visitClassDefStart(const ClassDefPtr&);
        virtual void visitClassDefEnd(const ClassDefPtr&);
        virtual bool visitExceptionStart(const ExceptionPtr&);
        virtual void visitExceptionEnd(const ExceptionPtr&);
        virtual bool visitStructStart(const StructPtr&);
        virtual void visitStructEnd(const StructPtr&);
        virtual void visitOperation(const OperationPtr&);
        virtual void visitDataMember(const DataMemberPtr&);
        virtual void visitSequence(const SequencePtr&);
        virtual void visitDictionary(const DictionaryPtr&);
        virtual void visitEnum(const EnumPtr&);
        virtual void visitConst(const ConstPtr&);

    private:

        StringList normalize(const StringList&);

        bool _cpp11;
    };

    static void validateMetaData(const UnitPtr&);
    static void normalizeMetaData(const UnitPtr&, bool);
};

}

#endif
