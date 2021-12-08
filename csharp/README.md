# Building Ice for .NET

This page describes how to build Ice for .NET from source and package the
resulting binaries. As an alternative, you can download and install the
[zeroc.ice.net][1] NuGet package.

* [Building on Windows](#building-on-windows)
  * [Windows Build Requirements](#windows-build-requirements)
  * [Compiling Ice for \.NET on Windows](#compiling-ice-for-net-on-windows)
    * [Strong Name Signatures for \.NET Framework 4\.5 Assemblies](#strong-name-signatures-for-net-framework-45-assemblies)
    * [Authenticode Signatures](#authenticode-signatures)
    * [Building only the Test Suite](#building-only-the-test-suite)
* [Building on Linux or macOS](#building-on-linux-or-macos)
  * [Linux and macOS Build Requirements](#linux-and-macos-build-requirements)
  * [Compiling Ice for \.NET on Linux or macOS](#compiling-ice-for-net-on-linux-or-macos)
* [Running the Tests](#running-the-tests)
* [NuGet Package](#nuget-package)
* [Building Ice for Xamarin Test Suite](#building-ice-for-xamarin-test-suite)

## Building on Windows

A source build of Ice for .NET on Windows produces three sets of assemblies:
 - assemblies for [.NET 6.0][8]
 - assemblies for the .NET Framework 4.5
 - assemblies for [.NET Standard 2.0][2]

### Windows Build Requirements

In order to build Ice for .NET from source, you need all of the following:
 - a [supported version][3] of Visual Studio
 - the [.NET Core 2.1 SDK][4], if you use Visual Studio 2017
 - the [.NET Core 3.1 SDK][5], if you use Visual Studio 2019
 - the [.NET Core 3.1 SDK][5] or the  [.NET Core 6.0 SDK][6], if you use Visual Studio 2022

> Note: Visual Studio 2022 version or higher is required for .NET 6.0 builds.
> Note: Visual Studio 2017 version 15.3 or higher is required for .NET Core builds.

### Compiling Ice for .NET on Windows

Open a Visual Studio command prompt and change to the `csharp` subdirectory:
```
cd csharp
```

To build all Ice assemblies and the associated test suite, run:
```
msbuild msbuild\ice.proj
```

Upon completion, the Ice assemblies for .NET 6.0, the .NET Framework 4.5 and .NET Standard
2.0 are placed in the `lib\net6.0`, `lib\net45` and `lib\netstandard2.0` folders respectively.

> Note: the assemblies for .NET 6.0 are created only when you build with Visual Studio 2022
> or greater.

> Note: the assemblies for .NET Standard 2.0 are created only when you build with Visual Studio
> 2017 or greater.

You can skip the build of the test suite with the `BuildDist` target:
```
msbuild msbuild\ice.proj /t:BuildDist
```

The `Net6Build`, `Net6BuildDist`, `Net45Build`, `Net45BuildDist`, `NetStandardBuild` and
`NetStandardBuildDist` targets allow you to build assemblies only for .NET 6.0, the .NET
Framework 4.5 or .NET Standard 2.0, with or without the test suite.

The .NET Standard build of iceboxnet and test applications target `netcoreapp3.1` when using
Visual Studio 2019 and `netcoreapp2.1` when using Visual Studio 2017. You can change
the target framework by setting the `AppTargetFramework` property to a different
Target Framework Moniker value, for example:

```
msbuild msbuild\ice.proj /p:"AppTargetFramework=net462"
```

This builds the test programs for `net462`. The target frameworks you specify
must implement .NET Standard 2.0.

#### Strong Name Signatures

You can add Strong Naming signatures to the Ice assemblies by setting the
following environment variables before building these assemblies:

 - `PUBLIC_KEYFILE` Identity public key used to delay sign the assembly
 - `KEYFILE` Identity full key pair used to sign the assembly

If only `PUBLIC_KEYFILE` is set, the assemblies are delay-signed during the
build and you must re-sign the assemblies later with the full identity key pair.

If only `KEYFILE` is set, the assemblies are fully signed during the build using
`KEYFILE`.

If both `PUBLIC_KEYFILE` and `KEYFILE` are set, assemblies are delay-signed
during the build using `PUBLIC_KEYFILE` and re-signed after the build using
`KEYFILE`. This can be used for generating [Enhanced Strong Naming][7]
signatures.

*Strong Name Signatures can be generated only from Windows builds.*

#### Authenticode Signatures

You can sign the Ice binaries with Authenticode by setting the following
environment variables before building these assemblies:
 - `SIGN_CERTIFICATE` to your Authenticode certificate
 - `SIGN_PASSWORD` to the certificate password

*Authenticode can be generated only from Windows builds.*

#### Building only the Test Suite

You can build only the test suite with this command:
```
msbuild msbuild\ice.proj /p:ICE_BIN_DIST=all
```

This build retrieves and installs the `zeroc.ice.net` NuGet package if
necessary.

## Building on Linux or macOS

### Linux and macOS Build Requirements

You need the [.NET Core 2.1 SDK][4], [.NET Core 3.1 SDK][5] or [.NET 6.0 SDK][6]
to build Ice for .NET from source.

### Compiling Ice for .NET on Linux or macOS

Open a command prompt and change to the `csharp` directory:
```
cd csharp
```

Then run:
```
dotnet msbuild msbuild/ice.proj
```

Upon completion, the Ice assemblies for .NET 6.0 and .NET Standard 2.0 are placed
in the `lib/net6.0` and `lib/netstandard2.0` directory respectively.

You can skip the build of the test suite with the `BuildDist` target:
```
dotnet msbuild msbuild/ice.proj /t:BuildDist
```

The `Net6Build`, `Net6BuildDist`, `NetStandardBuild` and `NetStandardBuildDist` targets
allow you to build assemblies only for .NET 6.0, or .NET Standard 2.0, with or without the
test suite.

The .NET Standard build of iceboxnet and test applications target `netcoreapp3.1` when using
.NET Core 3.1 SDK and `netcoreapp2.1` when using .NET Core 2.1 SDK. You can change the target
framework by setting the `AppTargetFramework` property to a different Target Framework Moniker
value, for example:

```
dotnet msbuild msbuild/ice.proj /p:"AppTargetFramework=netcoreapp2.2"
```

## Running the Tests

Python is required to run the test suite. Additionally, the Glacier2 tests
require the Python module `passlib`, which you can install with the command:
```
pip install passlib
```

To run the tests, open a command window and change to the top-level directory.
At the command prompt, execute:
```
python allTests.py
```

If everything worked out, you should see lots of `ok` messages. In case of a
failure, the tests abort with `failed`.

On Windows, `allTests.py` executes by default the tests for .NET Framework 4.5.
In order to execute the tests with .NET Core framework add the `--dotnetcore`
option. For example:
```
python allTests.py --dotnetcore
```

If you want to run the test with .NET 6.0 you must use `--framework` option
with `net6.0` target framework.


For example:

```
python allTests.py --framework=net6.0
```

And to run test build against .NET Core 3.1:
```
python allTests.py --dotnetcore --framework=netcoreapp3.1
```

## NuGet Package

### Creating NuGet Packages on Windows

To create a NuGet package, open a Visual Studio command prompt and run the
following command:
```
msbuild msbuild\ice.proj /t:NuGetPack
```

This creates the `zeroc.ice.net` Nuget package in the `msbuild\zeroc.ice.net`
directory.

> Note: The NuGet package always includes assemblies for the .NET Framework 4.5.
>
> If you build with Visual Studio 2022, the NuGet package also includes assemblies
> for .NET 6.0.
>
> If you build with Visual Studio 2017 or Visual Studio 2019, the NuGet package
> also includes assemblies for .NET Standard 2.0.
>
> If you build with Visual Studio 2022 the NuGet package include iceboxnet
> executable targeting .NET 6.0, .NET Framework 4.5, .NET Core 3.1 and .NET
> Core 2.1.
>
> If you build with Visual Studio 2017 or Visual Studio 2019  the NuGet package
> include iceboxnet executables targeting .NET Framework 4.5 and .NET Core 2.1.
>

### Creating NuGet Packages on Linux or macOS

To create a NuGet package, open a command prompt and run the
following command:

```
dotnet msbuild msbuild/ice.proj /t:NuGetPack
```

This creates the `zeroc.ice.net` Nuget package in the `msbuild/zeroc.ice.net`
directory.

## Building Ice for Xamarin Test Suite

The `msbuild\ice.xamarin.test.sln` Visual Studio solution allows building
the Ice test suite as a Xamarin application that can be deployed on iOS, Android
or UWP platforms.

The Xamarin test suite uses the Ice assemblies for .NET Standard 2.0. either
from the source distribution or using the `zeroc.ice.net` NuGet package. If
using the assembles from the source distribution, they must be built before this
application.

### Building on Windows

#### Windows Build Requirements

* Visual Studio 2017 or Visual Studio 2019 with following workloads:
  * Universal Windows Platform development
  * Mobile development with .NET
  * .NET Core cross-platform development

#### Building the Android test controller

Open a Visual Studio 2017 or Visual Studio 2019 command prompt:

```
MSBuild msbuild\ice.proj /t:AndroidXamarinBuild
```

#### Building the UWP test controller

Open a Visual Studio 2019 or Visual Studio 2017 command prompt:

```
MSBuild msbuild\ice.proj /t:UWPXamarinBuild
```

#### Running the Android test suite

```
set PATH=%LOCALAPPDATA%\Android\sdk\tools\bin;%PATH%
set PATH=%LOCALAPPDATA%\Android\sdk\platform-tools;%PATH%
set PATH=%LOCALAPPDATA%\Android\sdk\emulator;%PATH%

python allTests.py --android --controller-app --config Release --platform x64
```

#### Running the UWP test suite

```
python allTests.py --uwp --controller-app --config Release --platform x64
```

### Building on macOS

#### macOS Build Requirements

* Visual Studio for Mac

#### Building the Android test controller

```
msbuild msbuild/ice.proj /t:AndroidXamarinBuild
```

#### Building the iOS test controller

```
msbuild msbuild/ice.proj /t:iOSXamarinBuild
```

#### Running the Android test suite

```
export PATH=~/Library/Android/sdk/tools/bin:$PATH
export PATH=~/Library/Android/sdk/platform-tools:$PATH
export PATH=~/Library/Android/sdk/emulator:$PATH

python allTests.py --android --controller-app --config Release --platform x64
```

#### Running the iOS test suite

```
python allTests.py --controller-app --config Release --platform iphonesimulator
```

[1]: https://zeroc.com/downloads/ice
[2]: https://blogs.msdn.microsoft.com/dotnet/2017/08/14/announcing-net-standard-2-0
[3]: https://doc.zeroc.com/ice/3.7/release-notes/supported-platforms-for-ice-3-7-7
[4]: https://dotnet.microsoft.com/download/dotnet-core/2.1
[5]: https://dotnet.microsoft.com/download/dotnet-core/3.1
[6]: https://dotnet.microsoft.com/en-us/download/dotnet/6.0
[7]: https://docs.microsoft.com/en-us/dotnet/framework/app-domains/enhanced-strong-naming
[8]: https://devblogs.microsoft.com/dotnet/announcing-net-6/
