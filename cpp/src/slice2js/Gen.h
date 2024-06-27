//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef GEN_H
#define GEN_H

#include "JsUtil.h"

namespace Slice
{
    class JsVisitor : public JsGenerator, public ParserVisitor
    {
    public:
        JsVisitor(
            ::IceInternal::Output&,
            const std::vector<std::pair<std::string, std::string>>& imports =
                std::vector<std::pair<std::string, std::string>>());
        virtual ~JsVisitor();

        std::vector<std::pair<std::string, std::string>> imports() const;

    protected:
        void writeMarshalDataMembers(const DataMemberList&, const DataMemberList&, const ContainedPtr&);
        void writeUnmarshalDataMembers(const DataMemberList&, const DataMemberList&, const ContainedPtr&);
        void writeInitDataMembers(const DataMemberList&, const ContainedPtr&);

        std::string getValue(const std::string&, const TypePtr&);

        std::string
        writeConstantValue(const std::string&, const TypePtr&, const SyntaxTreeBasePtr&, const std::string&);

        static StringList splitComment(const ContainedPtr&);
        void writeDocCommentFor(const ContainedPtr&);

        ::IceInternal::Output& _out;

        std::vector<std::pair<std::string, std::string>> _imports;
    };

    class Gen final : public JsGenerator
    {
    public:
        Gen(const std::string&, const std::vector<std::string>&, const std::string&, bool);

        Gen(const std::string&, const std::vector<std::string>&, const std::string&, bool, std::ostream&);

        ~Gen();

        void generate(const UnitPtr&);

    private:
        IceInternal::Output _javaScriptOutput;
        IceInternal::Output _typeScriptOutput;

        std::vector<std::string> _includePaths;
        std::string _fileBase;
        bool _useStdout;
        bool _typeScript;

        class ImportVisitor final : public JsVisitor
        {
        public:
            ImportVisitor(::IceInternal::Output&, std::vector<std::string>);

            bool visitClassDefStart(const ClassDefPtr&) final;
            bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
            bool visitStructStart(const StructPtr&) final;
            void visitOperation(const OperationPtr&) final;
            bool visitExceptionStart(const ExceptionPtr&) final;
            void visitSequence(const SequencePtr&) final;
            void visitDictionary(const DictionaryPtr&) final;
            void visitEnum(const EnumPtr&) final;

            // Emit the import statements for the given unit and return a list of the imported modules.
            std::set<std::string> writeImports(const UnitPtr&);

        private:
            bool _seenClass;
            bool _seenInterface;
            bool _seenCompactId;
            bool _seenOperation;
            bool _seenStruct;
            bool _seenUserException;
            bool _seenEnum;
            bool _seenSeq;
            bool _seenDict;
            bool _seenObjectSeq;
            bool _seenObjectProxySeq;
            bool _seenObjectDict;
            bool _seenObjectProxyDict;
            std::vector<std::string> _includePaths;
        };

        class ExportsVisitor final : public JsVisitor
        {
        public:
            ExportsVisitor(::IceInternal::Output&, std::set<std::string>);

            bool visitModuleStart(const ModulePtr&) final;

            std::set<std::string> exportedModules() const;

        private:
            std::string encodeTypeForOperation(const TypePtr&);

            std::set<std::string> _importedModules;
            std::set<std::string> _exportedModules;
        };

        class TypesVisitor final : public JsVisitor
        {
        public:
            TypesVisitor(::IceInternal::Output&);

            bool visitClassDefStart(const ClassDefPtr&) final;
            bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
            bool visitExceptionStart(const ExceptionPtr&) final;
            bool visitStructStart(const StructPtr&) final;
            void visitSequence(const SequencePtr&) final;
            void visitDictionary(const DictionaryPtr&) final;
            void visitEnum(const EnumPtr&) final;
            void visitConst(const ConstPtr&) final;

        private:
            std::string encodeTypeForOperation(const TypePtr&);
        };

        class TypeScriptImportVisitor final : public JsVisitor
        {
        public:
            TypeScriptImportVisitor(::IceInternal::Output&);

            bool visitUnitStart(const UnitPtr&) final;
            bool visitClassDefStart(const ClassDefPtr&) final;
            bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
            bool visitStructStart(const StructPtr&) final;
            bool visitExceptionStart(const ExceptionPtr&) final;
            void visitSequence(const SequencePtr&) final;
            void visitDictionary(const DictionaryPtr&) final;

            // Emit the import statements for the given unit and return a map of the imported types per module.
            std::map<std::string, std::string> writeImports();

        private:
            void addImport(const ContainedPtr&);

            // All modules imported by the current unit.
            std::set<std::string> _importedModules;
            // A map of imported types to their module name.
            std::map<std::string, std::string> _importedTypes;
            // The module name of the current unit.
            std::string _module;
            // The filename of the current unit.
            std::string _filename;
        };

        class TypeScriptVisitor final : public JsVisitor
        {
        public:
            TypeScriptVisitor(::IceInternal::Output&, std::map<std::string, std::string>);

            bool visitUnitStart(const UnitPtr&) final;
            void visitUnitEnd(const UnitPtr&) final;
            bool visitModuleStart(const ModulePtr&) final;
            void visitModuleEnd(const ModulePtr&) final;
            bool visitClassDefStart(const ClassDefPtr&) final;
            bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
            bool visitExceptionStart(const ExceptionPtr&) final;
            bool visitStructStart(const StructPtr&) final;
            void visitSequence(const SequencePtr&) final;
            void visitDictionary(const DictionaryPtr&) final;
            void visitEnum(const EnumPtr&) final;
            void visitConst(const ConstPtr&) final;

        private:
            std::string importPrefix(const std::string&) const;
            std::string
            typeToTsString(const TypePtr&, bool nullable = false, bool forParameter = false, bool optional = false)
                const;

            // The module name of the current unit.
            std::string _module;
            // The import prefix for the "ice" module either empty string when building Ice or "__module_ice."
            std::string _iceImportPrefix;
            // A map of imported types to their module name.
            std::map<std::string, std::string> _importedTypes;
        };
    };
}

#endif
