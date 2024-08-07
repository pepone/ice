//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef SWIFT_UTIL_H
#define SWIFT_UTIL_H

#include "../Ice/OutputUtil.h"
#include "../Slice/Parser.h"

typedef std::list<std::pair<std::string, std::string>> StringPairList;

namespace Slice
{
    const int TypeContextInParam = 1;
    const int TypeContextProtocol = 2;
    const int TypeContextLocal = 32;

    std::string getSwiftModule(const ModulePtr&, std::string&);
    std::string getSwiftModule(const ModulePtr&);
    ModulePtr getTopLevelModule(const ContainedPtr&);
    ModulePtr getTopLevelModule(const TypePtr&);

    std::string fixIdent(const std::string&);

    struct ParamInfo
    {
        std::string name;
        TypePtr type;
        std::string typeStr;
        bool optional;
        int tag;
        ParamDeclPtr param; // 0 == return value
    };

    typedef std::list<ParamInfo> ParamInfoList;

    struct DocElements
    {
        StringList overview;
        bool deprecated;
        StringList deprecateReason;
        StringList misc;
        StringList seeAlso;
        StringList returns;
        std::map<std::string, StringList> params;
        std::map<std::string, StringList> exceptions;
    };

    class SwiftGenerator
    {
    public:
        SwiftGenerator() = default;
        SwiftGenerator(const SwiftGenerator&) = delete;
        virtual ~SwiftGenerator() = default;

        SwiftGenerator& operator=(const SwiftGenerator&) = delete;

        static void validateMetaData(const UnitPtr&);

    protected:
        void trimLines(StringList&);
        StringList splitComment(const std::string&);
        bool parseCommentLine(const std::string&, const std::string&, bool, std::string&, std::string&);
        DocElements parseComment(const ContainedPtr&);
        void writeDocLines(
            IceInternal::Output&,
            const StringList&,
            bool commentFirst = true,
            const std::string& space = " ");
        void writeDocSentence(IceInternal::Output&, const StringList&);
        void writeSeeAlso(IceInternal::Output&, const StringList&, const ContainerPtr&);
        void writeDocSummary(IceInternal::Output&, const ContainedPtr&);
        void writeOpDocSummary(IceInternal::Output&, const OperationPtr&, bool);

        void writeProxyDocSummary(IceInternal::Output&, const InterfaceDefPtr&, const std::string&);
        void writeServantDocSummary(IceInternal::Output&, const InterfaceDefPtr&, const std::string&);
        void writeMemberDoc(IceInternal::Output&, const DataMemberPtr&);

        std::string paramLabel(const std::string&, const ParamDeclList&);
        std::string operationReturnType(const OperationPtr&, int typeCtx = 0);
        bool operationReturnIsTuple(const OperationPtr&);
        std::string operationReturnDeclaration(const OperationPtr&);
        std::string operationInParamsDeclaration(const OperationPtr&);

        ParamInfoList getAllInParams(const OperationPtr&, int = 0);
        void getInParams(const OperationPtr&, ParamInfoList&, ParamInfoList&);

        ParamInfoList getAllOutParams(const OperationPtr&, int = 0);
        void getOutParams(const OperationPtr&, ParamInfoList&, ParamInfoList&);

        std::string
        typeToString(const TypePtr&, const ContainedPtr&, const StringList& = StringList(), bool = false, int = 0);

        std::string getAbsolute(const TypePtr&);
        std::string getAbsolute(const ClassDeclPtr&);
        std::string getAbsolute(const ClassDefPtr&);
        std::string getAbsolute(const InterfaceDeclPtr&);
        std::string getAbsolute(const InterfaceDefPtr&);
        std::string getAbsolute(const StructPtr&);
        std::string getAbsolute(const ExceptionPtr&);
        std::string getAbsolute(const EnumPtr&);
        std::string getAbsolute(const ConstPtr&);
        std::string getAbsolute(const SequencePtr&);
        std::string getAbsolute(const DictionaryPtr&);

        std::string getUnqualified(const std::string&, const std::string&);
        std::string modeToString(Operation::Mode);
        std::string getOptionalFormat(const TypePtr&);

        static bool isNullableType(const TypePtr&);
        bool isProxyType(const TypePtr&);

        std::string getValue(const std::string&, const TypePtr&);
        void writeConstantValue(
            IceInternal::Output& out,
            const TypePtr&,
            const SyntaxTreeBasePtr&,
            const std::string&,
            const StringList&,
            const std::string&,
            bool optional = false);
        void writeDefaultInitializer(IceInternal::Output&, bool, bool);
        void writeMemberwiseInitializer(IceInternal::Output&, const DataMemberList&, const ContainedPtr&);
        void writeMemberwiseInitializer(
            IceInternal::Output&,
            const DataMemberList&,
            const DataMemberList&,
            const DataMemberList&,
            const ContainedPtr&,
            bool rootClass);
        void writeMembers(IceInternal::Output&, const DataMemberList&, const ContainedPtr&, int = 0);

        void writeMarshalUnmarshalCode(
            ::IceInternal::Output&,
            const TypePtr&,
            const ContainedPtr&,
            const std::string&,
            bool,
            int = -1);

        bool usesMarshalHelper(const TypePtr&);
        void writeMarshalInParams(::IceInternal::Output&, const OperationPtr&);
        void writeMarshalOutParams(::IceInternal::Output&, const OperationPtr&);
        void writeMarshalAsyncOutParams(::IceInternal::Output&, const OperationPtr&);
        void writeUnmarshalInParams(::IceInternal::Output&, const OperationPtr&);
        void writeUnmarshalOutParams(::IceInternal::Output&, const OperationPtr&);
        void writeUnmarshalUserException(::IceInternal::Output& out, const OperationPtr&);
        void writeSwiftAttributes(::IceInternal::Output&, const StringList&);
        void writeProxyOperation(::IceInternal::Output&, const OperationPtr&);
        void writeDispatchOperation(::IceInternal::Output&, const OperationPtr&);

    private:
        class MetaDataVisitor final : public ParserVisitor
        {
        public:
            bool visitModuleStart(const ModulePtr&) final;
            bool visitClassDefStart(const ClassDefPtr&) final;
            bool visitInterfaceDefStart(const InterfaceDefPtr&) final;
            void visitOperation(const OperationPtr&) final;
            bool visitExceptionStart(const ExceptionPtr&) final;
            bool visitStructStart(const StructPtr&) final;
            void visitSequence(const SequencePtr&) final;
            void visitDictionary(const DictionaryPtr&) final;
            void visitEnum(const EnumPtr&) final;
            void visitConst(const ConstPtr&) final;

        private:
            StringList validate(
                const SyntaxTreeBasePtr&,
                const StringList&,
                const std::string&,
                const std::string&,
                bool local = false,
                bool operationParameter = false);

            typedef std::map<std::string, std::string> ModuleMap;
            typedef std::map<std::string, ModuleMap> ModulePrefix;

            //
            // Each Slice unit has to map all top-level modules to a single Swift module
            //
            ModuleMap _modules;

            //
            // With a given Swift module a Slice module has to map to a single prefix
            //
            ModulePrefix _prefixes;

            static const std::string _msg;
        };
    };
}

#endif
