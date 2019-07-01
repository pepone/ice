// swift-tools-version:5.0
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
  name: "Ice",
  platforms: [
    .macOS(.v10_13),
    .iOS(.v12),
  ],
  products: [
    .library(name: "Ice", targets: ["Ice"]),
    .library(name: "Glacier2", targets: ["Glacier2"]),
    .library(name: "IceStorm", targets: ["IceStorm"]),
    .library(name: "IceGrid", targets: ["IceGrid"])
  ],
  dependencies: [
    .package(url: "https://github.com/mxcl/PromiseKit.git", from: "6.8.5")
  ],
  targets: [
    .target(name: "IceUtilCpp",
            cxxSettings: [.define("ICE_CPP11_MAPPING"),
                          .define("ICE_SWIFT"),
                          .define("ICE_BUILDING_SRC")]),
    .target(name: "IceCpp",
            dependencies: ["IceUtilCpp"],
            cxxSettings: [.define("ICE_CPP11_MAPPING"),
                          .define("ICE_SWIFT"),
                          .define("ICE_BUILDING_SRC")],
            linkerSettings: [.linkedLibrary("iconv"),
                             .linkedLibrary("bz2")]),
    .target(name: "IceSSLCpp",
            dependencies: ["IceCpp", "IceUtilCpp"],
            cxxSettings: [.define("ICE_CPP11_MAPPING"),
                          .define("ICE_SWIFT"),
                          .define("ICE_BUILDING_SRC")]),
    .target(name: "IceDiscoveryCpp",
            dependencies: ["IceCpp", "IceUtilCpp"],
            cxxSettings: [.define("ICE_CPP11_MAPPING"),
                          .define("ICE_SWIFT"),
                          .define("ICE_BUILDING_SRC")]),
    .target(name: "IceLocatorDiscoveryCpp",
            dependencies: ["IceCpp", "IceUtilCpp"],
            cxxSettings: [.define("ICE_CPP11_MAPPING"),
                          .define("ICE_SWIFT"),
                          .define("ICE_BUILDING_SRC")]),
    .target(name: "IceObjc",
            dependencies: ["IceCpp", "IceUtilCpp", "IceSSLCpp", "IceDiscoveryCpp",
                           "IceLocatorDiscoveryCpp"],
            cxxSettings: [.define("ICE_CPP11_MAPPING"),
                          .define("ICE_SWIFT"),
                          .define("ICE_BUILDING_SRC")]),
    .target(name: "Ice", dependencies: ["IceObjc", "PromiseKit"]),
    .target(name: "Glacier2", dependencies: ["Ice"]),
    .target(name: "IceStorm", dependencies: ["Ice"]),
    .target(name: "IceGrid", dependencies: ["Ice", "Glacier2"])
  ],
  cxxLanguageStandard: .cxx14
)
