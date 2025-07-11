// Copyright (c) ZeroC, Inc.

#ifndef SLICE_PYTHON_UTIL_H
#define SLICE_PYTHON_UTIL_H

#include "../Ice/OutputUtil.h"
#include "../Slice/Parser.h"

namespace Slice::Python
{
    /// Determine the name of a Python source file for use in an import statement.
    /// The return value does not include the .py extension.
    std::string getImportFileName(const std::string&, const std::vector<std::string>&);

    /// Generate Python code for a translation unit.
    /// @p unit is the Slice unit to generate code for.
    /// @p outputDir The base-directory to write the generated Python files to.
    void generate(const Slice::UnitPtr& unit, const std::string& outputDir);

    /// Return the package specified by metadata for the given definition, or an empty string if no metadata was found.
    std::string getPackageMetadata(const Slice::ContainedPtr&);

    /// Get the fully-qualified name of the given definition, including any package defined via metadata.
    std::string getAbsolute(const Slice::ContainedPtr& p);

    /// Get the fully-qualified name of the given definition, including any package defined via metadata,
    /// but "_M_" is prepended to the first name segment, indicating that this is a an explicit reference.
    std::string getTypeReference(const Slice::ContainedPtr& p);

    int compile(const std::vector<std::string>&);

    /// Returns a DocString formatted link to the provided Slice identifier.
    std::string
    pyLinkFormatter(const std::string& rawLink, const ContainedPtr& source, const SyntaxTreeBasePtr& target);

    void validatePythonMetadata(const UnitPtr&);
}

#endif
