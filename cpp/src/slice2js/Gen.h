// Copyright (c) ZeroC, Inc.

#ifndef GEN_H
#define GEN_H

#include "JsUtil.h"

namespace Slice
{
    struct DocSummaryOptions
    {
        /// If false, `@@deprecated` tags in the Slice doc-comment will be ignored.
        bool generateDeprecated{true};

        /// If false, `@@remarks` tags in the Slice doc-comment will be ignored. This should always be 'true' for
        /// typescript generated code, and 'false' for javascript generated code.
        bool includeRemarks{true};

        std::optional<std::string> generatedType{std::nullopt};
    };

    class JsVisitor : public ParserVisitor
    {
    public:
        JsVisitor(::IceInternal::Output& output);
        ~JsVisitor() override;

    protected:
        void writeMarshalDataMembers(const DataMemberList&, const DataMemberList&);
        void writeUnmarshalDataMembers(const DataMemberList&, const DataMemberList&);
        void writeOneShotConstructorArguments(const DataMemberList& members);

        std::string getValue(const TypePtr&);

        std::string writeConstantValue(const TypePtr&, const SyntaxTreeBasePtr&, const std::string&);

        /// Generates and outputs a doc-summary for Slice definition @p p.
        /// @param p The Slice definition to be documented.
        /// @param options Options that control how the doc-summary is generated.
        void writeDocSummary(const ContainedPtr& p, const DocSummaryOptions& options = {});

        ::IceInternal::Output& _out;
    };

    /// Visitor that collects nested module paths across all files in the unit.
    /// This handles the case where modules are reopened across multiple files (including transitive includes).
    /// The collected data is used by ImportVisitor to generate proper nested module aggregation.
    class ModuleVisitor final : public JsVisitor
    {
    public:
        /// Constructs a ModuleVisitor.
        /// @param out Output stream (unused but required by JsVisitor base).
        /// @param jsModule The JavaScript module name for the current unit.
        /// @param topLevelFile The top-level Slice file being compiled.
        /// @param fileToDirectInclude A map from each file to its direct include ancestor.
        ///        For direct includes, the value is the file itself. For transitive includes,
        ///        the value is the direct include that brings them in.
        ModuleVisitor(
            ::IceInternal::Output& out,
            const std::string& jsModule,
            const std::string& topLevelFile,
            std::map<std::string, std::string> fileToDirectInclude);

        bool visitModuleStart(const ModulePtr&) final;

        /// Returns the collected nested module paths.
        /// Structure: jsImportFile -> topLevelModule -> set of nested paths (relative to top-level).
        /// For example: nestedModulePaths["./First.js"]["Outer"] = {"Inner", "Inner.Deep", "Inner.Transitive"}
        [[nodiscard]] const std::map<std::string, std::map<std::string, std::set<std::string>>>&
        getNestedModulePaths() const;

        /// Override to visit included definitions (needed to collect nested module paths from all files).
        [[nodiscard]] bool shouldVisitIncludedDefinitions() const final { return true; }

    private:
        std::string _jsModule;
        std::string _topLevelFile;
        std::map<std::string, std::string> _fileToDirectInclude;
        std::map<std::string, std::map<std::string, std::set<std::string>>> _nestedModulePaths;
        std::string _currentDirectInclude; // Tracks the current direct include during traversal
    };

    class Gen final
    {
    public:
        Gen(const std::string&, const std::string&, bool);

        Gen(const std::string&, const std::string&, bool, std::ostream&);

        ~Gen();

        void generate(const UnitPtr&);

    private:
        IceInternal::Output _javaScriptOutput;
        IceInternal::Output _typeScriptOutput;

        std::string _fileBase;
        bool _useStdout;
        bool _typeScript;

        class ImportVisitor final : public JsVisitor
        {
        public:
            ImportVisitor(::IceInternal::Output&);

            bool visitClassDefStart(const ClassDefPtr&) final;
            bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
            bool visitStructStart(const StructPtr&) final;
            void visitOperation(const OperationPtr&) final;
            bool visitExceptionStart(const ExceptionPtr&) final;
            void visitSequence(const SequencePtr&) final;
            void visitDictionary(const DictionaryPtr&) final;
            void visitEnum(const EnumPtr&) final;

            // Emit the import statements for the given unit and return a list of the imported modules.
            // The nestedModulePaths parameter provides pre-collected nested module paths from ModuleVisitor.
            std::set<std::string> writeImports(
                const UnitPtr&,
                const std::map<std::string, std::map<std::string, std::set<std::string>>>& nestedModulePaths);

        private:
            bool _seenClass{false};
            bool _seenInterface{false};
            bool _seenOperation{false};
            bool _seenStruct{false};
            bool _seenUserException{false};
            bool _seenEnum{false};
            bool _seenSeq{false};
            bool _seenDict{false};
            bool _seenObjectSeq{false};
            bool _seenObjectProxySeq{false};
            bool _seenObjectDict{false};
            bool _seenObjectProxyDict{false};
        };

        class ExportsVisitor final : public JsVisitor
        {
        public:
            ExportsVisitor(::IceInternal::Output&, std::set<std::string>);

            bool visitModuleStart(const ModulePtr&) final;

            [[nodiscard]] std::set<std::string> exportedModules() const;

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
            [[nodiscard]] std::string importPrefix(const std::string&) const;
            [[nodiscard]] std::string
            typeToTsString(const TypePtr&, bool nullable = false, bool forParameter = false, bool optional = false)
                const;
            void writeOpDocSummary(::IceInternal::Output& out, const OperationPtr& op, bool forDispatch);
            void writeImportedTypeReExports(const std::string& currentModulePath);

            // The module name of the current unit.
            std::string _module;
            // The import prefix for the "ice" module either empty string when building Ice or "__module__zeroc_ice."
            std::string _iceImportPrefix;
            // A map of imported types to their module name.
            std::map<std::string, std::string> _importedTypes;
            // Track types that have been re-exported to avoid duplicates.
            std::set<std::string> _reExportedTypes;
        };
    };
}

#endif
