// swift-tools-version: 5.9

import PackageDescription

let cxxSettings: [CXXSetting] = [
    .define("ICE_BUILDING_SRC"),
    .define("ICE_STATIC_LIBS"),
    .define("ICE_SWIFT"),
    .define("ICE_CPP11_MAPPING"),
    CXXSetting.headerSearchPath("src"),
    CXXSetting.headerSearchPath("include"),
    CXXSetting.headerSearchPath("include/generated")]

let package = Package(
  name: "Ice",
  defaultLocalization: "en",
  platforms: [
    .macOS(.v14),
    .iOS(.v12),
  ],
  products: [
    .library(name: "Ice", targets: ["Ice"]),
    .library(name: "Glacier2", targets: ["Glacier2"]),
    .library(name: "IceStorm", targets: ["IceStorm"]),
    .library(name: "IceGrid", targets: ["IceGrid"])
  ],
  dependencies: [
    .package(url: "https://github.com/mxcl/PromiseKit.git", from: "6.8.10")
  ],
  targets: [
    .target(
        name: "IceCpp",
        path: "cpp/",
        exclude: ["src/Ice/msbuild", "src/Ice/build", "src/Ice/Ice.rc", "src/Ice/Makefile.mk"],
        sources: ["src/Ice"],
        publicHeadersPath: "include/Ice",
        cxxSettings: cxxSettings,
        linkerSettings: [.linkedLibrary("iconv"), .linkedLibrary("bz2"), .linkedFramework("Foundation")]),
    .target(
        name: "IceSSLCpp", 
        dependencies: ["IceCpp"],
        path: "cpp/",
        exclude: [
            "src/IceSSL/msbuild",
            "src/IceSSL/build",
            "src/IceSSL/IceSSL.rc",
            "src/IceSSL/IceSSLOpenSSL.rc",
            "src/IceSSL/Makefile.mk",
            "src/IceSSL/SChannelCertificateI.cpp",
            "src/IceSSL/SChannelEngine.cpp", 
            "src/IceSSL/SChannelPluginI.cpp", 
            "src/IceSSL/SChannelTransceiverI.cpp", 
            "src/IceSSL/OpenSSLCertificateI.cpp",
            "src/IceSSL/OpenSSLEngine.cpp", 
            "src/IceSSL/OpenSSLPluginI.cpp", 
            "src/IceSSL/OpenSSLTransceiverI.cpp", 
            "src/IceSSL/OpenSSLUtil.cpp"],
        sources: ["src/IceSSL"],
        publicHeadersPath: "include/IceSSL",
        cxxSettings: cxxSettings),
    .target(
        name: "IceDiscoveryCpp", 
        dependencies: ["IceCpp"],
        path: "cpp/",
        exclude: [
            "src/IceDiscovery/msbuild",
            "src/IceDiscovery/build",
            "src/IceDiscovery/IceDiscovery.rc",
            "src/IceDiscovery/Makefile.mk"],
        sources: ["src/IceDiscovery"],
        publicHeadersPath: "include/IceDiscovery",
        cxxSettings: cxxSettings),
    .target(
        name: "IceLocatorDiscoveryCpp",
        dependencies: ["IceCpp"], 
        path: "cpp/",
        exclude: [
            "src/IceLocatorDiscovery/msbuild",
            "src/IceLocatorDiscovery/build",
            "src/IceLocatorDiscovery/IceLocatorDiscovery.rc",
            "src/IceLocatorDiscovery/Makefile.mk"],
        sources: ["src/IceLocatorDiscovery"],
        publicHeadersPath: "include/IceLocatorDiscovery",
        cxxSettings: cxxSettings),
    .target(
        name: "IceImpl",
        dependencies: ["IceCpp", "IceSSLCpp", "IceDiscoveryCpp", "IceLocatorDiscoveryCpp"],
        path: "swift/src/IceImpl",
        cxxSettings: [
            .define("ICE_BUILDING_SRC"),
            .define("ICE_STATIC_LIBS"),
            .define("ICE_SWIFT"),
            .define("ICE_CPP11_MAPPING"),
            CXXSetting.headerSearchPath("../../../cpp/include"),
            CXXSetting.headerSearchPath("../../../cpp/src"),
            CXXSetting.headerSearchPath("../../../cpp/include/generated")],
        linkerSettings: [.linkedFramework("ExternalAccessory")]),
    .target(
        name: "Ice",
        dependencies: ["IceImpl", "PromiseKit"],
        path: "swift/src/Ice"),
    .target(
        name: "Glacier2",
        dependencies: ["Ice"],
        path: "swift/src/Glacier2"),
    .target(
        name: "IceStorm",
        dependencies: ["Ice"],
        path: "swift/src/IceStorm"),
    .target(
        name: "IceGrid",
        dependencies: ["Ice", "Glacier2"],
        path: "swift/src/IceGrid")
  ],
  cxxLanguageStandard: .cxx17
)