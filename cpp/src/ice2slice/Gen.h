//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef GEN_H
#define GEN_H

#include <Slice/Parser.h>
#include <IceUtil/OutputUtil.h>

#include <map>

namespace Slice
{

class Gen
{
public:

    Gen(const std::string&);

    void generate(const UnitPtr&);

private:

    std::string _fileBase;

    class OutputVisitor : public ParserVisitor
    {
    public:

        virtual bool visitClassDefStart(const ClassDefPtr&);
        virtual bool visitExceptionStart(const ExceptionPtr&);
        virtual bool visitStructStart(const StructPtr&);
        virtual void visitSequence(const SequencePtr&);
        virtual void visitDictionary(const DictionaryPtr&);
        virtual void visitEnum(const EnumPtr&);

        std::set<std::string> modules() const;

    private:

        std::set<std::string> _modules;
    };

    class TypesVisitor : public ParserVisitor
    {
    public:

        TypesVisitor(const std::string&, const std::set<std::string>&);

        virtual bool visitClassDefStart(const ClassDefPtr&);
        virtual bool visitExceptionStart(const ExceptionPtr&);
        virtual bool visitStructStart(const StructPtr&);
        virtual void visitSequence(const SequencePtr&);
        virtual void visitDictionary(const DictionaryPtr&);
        virtual void visitEnum(const EnumPtr&);

    private:

        IceUtilInternal::Output& getOutput(const ContainedPtr&);

        std::string _fileBase;
        std::set<std::string> _modules;
        std::map<std::string, IceUtilInternal::Output*> _outputs;
    };
};

}

#endif
