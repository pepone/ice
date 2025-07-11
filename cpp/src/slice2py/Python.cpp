// Copyright (c) ZeroC, Inc.

#include "../Ice/ConsoleUtil.h"
#include "../Ice/FileUtil.h"
#include "../Ice/Options.h"
#include "../Slice/FileTracker.h"
#include "../Slice/Preprocessor.h"
#include "../Slice/Util.h"
#include "Ice/CtrlCHandler.h"
#include "Ice/StringUtil.h"
#include "PythonUtil.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#    include <direct.h>
#else
#    include <unistd.h>
#endif

using namespace std;
using namespace Slice;
using namespace Slice::Python;
using namespace IceInternal;

namespace
{
    mutex globalMutex;
    bool interrupted = false;

    void interruptedCallback(int /*signal*/)
    {
        lock_guard lock(globalMutex);

        interrupted = true;
    }

    string getPackageInitOutputFile(const string& packageName, const string& outputDir)
    {
        // Create a new output file for this package.
        string fileName = packageName;
        replace(fileName.begin(), fileName.end(), '.', '/');
        fileName += "/__init__.py";

        string outputPath;
        if (!outputDir.empty())
        {
            outputPath = outputDir + "/";
        }
        else
        {
            outputPath = "./";
        }
        createPackagePath(packageName, outputPath);
        outputPath += fileName;

        FileTracker::instance()->addFile(outputPath);

        return outputPath;
    }

    void usage(const string& n)
    {
        consoleErr << "Usage: " << n << " [options] slice-files...\n";
        consoleErr << "Options:\n"
                      "-h, --help               Show this message.\n"
                      "-v, --version            Display the Ice version.\n"
                      "-DNAME                   Define NAME as 1.\n"
                      "-DNAME=DEF               Define NAME as DEF.\n"
                      "-UNAME                   Remove any definition for NAME.\n"
                      "-IDIR                    Put DIR in the include file search path.\n"
                      "-E                       Print preprocessor output on stdout.\n"
                      "--output-dir DIR         Create files in the directory DIR.\n"
                      "-d, --debug              Print debug messages.\n"
                      "--depend                 Generate Makefile dependencies.\n"
                      "--depend-xml             Generate dependencies in XML format.\n"
                      "--depend-file FILE       Write dependencies to FILE instead of standard output.\n"
                      "--no-package             Do not generate Python package hierarchy.\n"
                      "--build-package          Only generate Python package hierarchy.\n";
    }
}

int
Slice::Python::compile(const vector<string>& argv)
{
    IceInternal::Options opts;
    opts.addOpt("h", "help");
    opts.addOpt("v", "version");
    opts.addOpt("D", "", IceInternal::Options::NeedArg, "", IceInternal::Options::Repeat);
    opts.addOpt("U", "", IceInternal::Options::NeedArg, "", IceInternal::Options::Repeat);
    opts.addOpt("I", "", IceInternal::Options::NeedArg, "", IceInternal::Options::Repeat);
    opts.addOpt("E");
    opts.addOpt("", "output-dir", IceInternal::Options::NeedArg);
    opts.addOpt("", "depend");
    opts.addOpt("", "depend-xml");
    opts.addOpt("", "depend-file", IceInternal::Options::NeedArg, "");
    opts.addOpt("d", "debug");
    opts.addOpt("", "no-package");
    opts.addOpt("", "build-package");

    vector<string> args;
    try
    {
        args = opts.parse(argv);
    }
    catch (const IceInternal::BadOptException& e)
    {
        consoleErr << argv[0] << ": error: " << e.what() << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (opts.isSet("help"))
    {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (opts.isSet("version"))
    {
        consoleErr << ICE_STRING_VERSION << endl;
        return EXIT_SUCCESS;
    }

    vector<string> cppArgs;
    vector<string> optargs = opts.argVec("D");
    cppArgs.reserve(optargs.size());
    for (const auto& arg : optargs)
    {
        cppArgs.push_back("-D" + arg);
    }

    optargs = opts.argVec("U");
    for (const auto& arg : optargs)
    {
        cppArgs.push_back("-U" + arg);
    }

    vector<string> includePaths = opts.argVec("I");
    for (const auto& includePath : includePaths)
    {
        cppArgs.push_back("-I" + Preprocessor::normalizeIncludePath(includePath));
    }

    bool preprocess = opts.isSet("E");

    string outputDir = opts.optArg("output-dir");

    bool depend = opts.isSet("depend");

    bool dependxml = opts.isSet("depend-xml");

    string dependFile = opts.optArg("depend-file");

    bool debug = opts.isSet("debug");

    bool noPackage = opts.isSet("no-package");

    bool buildPackage = opts.isSet("build-package");

    if (args.empty())
    {
        consoleErr << argv[0] << ": error: no input file" << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (depend && dependxml)
    {
        consoleErr << argv[0] << ": error: cannot specify both --depend and --dependxml" << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (noPackage && buildPackage)
    {
        consoleErr << argv[0] << ": error: cannot specify both --no-package and --build-package" << endl;
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (!outputDir.empty() && !IceInternal::directoryExists(outputDir))
    {
        consoleErr << argv[0] << ": error: argument for --output-dir does not exist or is not a directory" << endl;
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;

    Ice::CtrlCHandler ctrlCHandler;
    ctrlCHandler.setCallback(interruptedCallback);

    ostringstream os;
    if (dependxml)
    {
        os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<dependencies>" << endl;
    }

    map<string, map<string, set<string>>> packageImports;
    PackageVisitor packageVisitor(packageImports);

    for (const auto& fileName : args)
    {
        if (depend || dependxml)
        {
            PreprocessorPtr icecpp = Preprocessor::create(argv[0], fileName, cppArgs);
            FILE* cppHandle = icecpp->preprocess(false, "-D__SLICE2PY__");

            if (cppHandle == nullptr)
            {
                return EXIT_FAILURE;
            }

            UnitPtr unit = Unit::createUnit("python", false);
            int parseStatus = unit->parse(fileName, cppHandle, debug);
            unit->destroy();

            if (parseStatus == EXIT_FAILURE)
            {
                return EXIT_FAILURE;
            }

            if (!icecpp->printMakefileDependencies(
                    os,
                    depend ? Preprocessor::Python : Preprocessor::SliceXML,
                    includePaths,
                    "-D__SLICE2PY__"))
            {
                return EXIT_FAILURE;
            }

            if (!icecpp->close())
            {
                return EXIT_FAILURE;
            }
        }
        else
        {
            PreprocessorPtr icecpp = Preprocessor::create(argv[0], fileName, cppArgs);
            FILE* cppHandle = icecpp->preprocess(true, "-D__SLICE2PY__");

            if (cppHandle == nullptr)
            {
                return EXIT_FAILURE;
            }

            if (preprocess)
            {
                char buf[4096];
                while (fgets(buf, static_cast<int>(sizeof(buf)), cppHandle) != nullptr)
                {
                    if (fputs(buf, stdout) == EOF)
                    {
                        return EXIT_FAILURE;
                    }
                }
                if (!icecpp->close())
                {
                    return EXIT_FAILURE;
                }
            }
            else
            {
                UnitPtr unit = Unit::createUnit("python", false);
                int parseStatus = unit->parse(fileName, cppHandle, debug);

                if (!icecpp->close())
                {
                    unit->destroy();
                    return EXIT_FAILURE;
                }

                if (parseStatus == EXIT_FAILURE)
                {
                    status = EXIT_FAILURE;
                }
                else
                {
                    try
                    {
                        // If --build-package is specified, we don't generate any code and simply update the __init__.py
                        // files.
                        if (!buildPackage)
                        {
                            // Generate Python code.
                            generate(unit, outputDir);
                        }
                        if (!noPackage)
                        {
                            // Collect the package imports.
                            unit->visit(&packageVisitor);
                        }
                    }
                    catch (const Slice::FileException&)
                    {
                        // If a file could not be created, then clean up any created files.
                        FileTracker::instance()->cleanup();
                        throw;
                    }
                }

                status |= unit->getStatus();
                unit->destroy();
            }
        }

        if (status != EXIT_FAILURE)
        {
            // Emit the package index files.
            for (const auto& [packageName, imports] : packageVisitor.imports())
            {
                Output out{getPackageInitOutputFile(packageName, outputDir).c_str()};
                out << sp;
                printHeader(out);
                out << sp;
                std::list<string> allDefinitions;
                for (const auto& [moduleName, definitions] : imports)
                {
                    for (const auto& name : definitions)
                    {
                        out << nl << "from ." << moduleName << " import " << name;
                        allDefinitions.push_back(name);
                    }
                }
                out << nl;

                out << sp;
                out << nl << "__all__ = [";
                out.inc();
                for (auto it = allDefinitions.begin(); it != allDefinitions.end();)
                {
                    out << nl << ("\"" + *it + "\"");
                    if (++it != allDefinitions.end())
                    {
                        out << ",";
                    }
                }
                out.dec();
                out << nl << "]";
                out << nl;
            }

            // Ensure all package directories have an __init__.py file.
            for (const auto& [packageName, imports] : packageVisitor.imports())
            {
                vector<string> packageParts;
                IceInternal::splitString(string_view{packageName}, ".", packageParts);
                string packagePath = outputDir;
                for (const auto& part : packageParts)
                {
                    packagePath += "/" + part;

                    const string initFile = packagePath + "/__init__.py";
                    if (!IceInternal::fileExists(initFile))
                    {
                        FileTracker::instance()->addDirectory(initFile);
                        // Create an empty __init__.py file in the package directory.
                        Output out{initFile.c_str()};
                        printHeader(out);
                        out << sp;
                    }
                }
            }
        }

        {
            lock_guard lock(globalMutex);
            if (interrupted)
            {
                FileTracker::instance()->cleanup();
                return EXIT_FAILURE;
            }
        }
    }

    if (dependxml)
    {
        os << "</dependencies>\n";
    }

    if (depend || dependxml)
    {
        writeDependencies(os.str(), dependFile);
    }

    return status;
}
