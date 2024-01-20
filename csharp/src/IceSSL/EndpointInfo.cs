//
// Copyright (c) ZeroC, Inc. All rights reserved.
//
//
// Ice version 3.7.10
//
// <auto-generated>
//
// Generated from file `EndpointInfo.ice'
//
// Warning: do not edit this file.
//
// </auto-generated>
//

using _System = global::System;

#pragma warning disable 1591

namespace IceSSL
{
    [global::System.Runtime.InteropServices.ComVisible(false)]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1707")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1709")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1710")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1711")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1715")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1716")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1720")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1722")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1724")]
    [global::System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1012")]
    public abstract partial class EndpointInfo : global::Ice.EndpointInfo
    {
        partial void ice_initialize();

        #region Constructors

        [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
        protected EndpointInfo() : base()
        {
            ice_initialize();
        }

        [global::System.CodeDom.Compiler.GeneratedCodeAttribute("slice2cs", "3.7.10")]
        protected EndpointInfo(global::Ice.EndpointInfo underlying, int timeout, bool compress) : base(underlying, timeout, compress)
        {
            ice_initialize();
        }

        #endregion
    }
}
