//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Globalization;

namespace ZeroC.Ice
{
    /// <summary>The Ice encoding defines how Slice constructs are marshaled to and later unmarshaled from sequences
    /// of bytes. An Encoding struct holds a version of the Ice encoding.</summary>
    [Serializable]
    public readonly struct Encoding : IEquatable<Encoding>
    {
        // The encodings known to the Ice runtime.

        /// <summary>Version 1.0 of the Ice encoding, supported by Ice 1.0 to Ice 3.7.</summary>
        public static readonly Encoding V1_0 = new Encoding(1, 0);

        /// <summary>Version 1.1 of the Ice encoding, supported since Ice 3.5.</summary>
        public static readonly Encoding V1_1 = new Encoding(1, 1);

        /// <summary>Version 2.0 of the Ice encoding, supported since Ice 4.0.</summary>
        public static readonly Encoding V2_0 = new Encoding(2, 0);

        /// <summary>The most recent version of the Ice encoding.</summary>
        public static readonly Encoding Latest = V2_0;

        /// <summary>The major version number of this version of the Ice encoding.</summary>
        public readonly byte Major;

        /// <summary>The minor version number of this version of the Ice encoding.</summary>
        public readonly byte Minor;

        internal bool IsSupported => this == V1_1 || this == V2_0;

        /// <summary>Parses a string into an Encoding.</summary>
        /// <param name="str">The string to parse.</param>
        /// <returns>A new encoding.</returns>
        public static Encoding Parse(string str)
        {
            int pos = str.IndexOf('.');
            if (pos == -1)
            {
                throw new FormatException($"malformed encoding string `{str}'");
            }

            string majStr = str[..pos];
            string minStr = str[(pos + 1)..];
            try
            {
                byte major = byte.Parse(majStr, CultureInfo.InvariantCulture);
                byte minor = byte.Parse(minStr, CultureInfo.InvariantCulture);
                return new Encoding(major, minor);
            }
            catch (FormatException)
            {
                throw new FormatException($"malformed encoding string `{str}'");
            }
        }

        /// <summary>Attempts to parse a string into an Encoding.</summary>
        /// <param name="str">The string to parse.</param>
        /// <param name="encoding">The resulting encoding.</param>
        /// <returns>True if the parsing succeeded and encoding contains the result; otherwise, false.</returns>
        public static bool TryParse(string str, out Encoding encoding)
        {
            try
            {
                encoding = Parse(str);
                return true;
            }
            catch (FormatException)
            {
                encoding = default;
                return false;
            }
        }

        /// <summary>The equality operator == returns true if its operands are equal, false otherwise.</summary>
        /// <param name="lhs">The left hand side operand.</param>
        /// <param name="rhs">The right hand side operand.</param>
        /// <returns><c>true</c> if the operands are equal, otherwise <c>false</c>.</returns>
        public static bool operator ==(Encoding lhs, Encoding rhs) => Equals(lhs, rhs);

        /// <summary>The inequality operator != returns true if its operands are not equal, false otherwise.</summary>
        /// <param name="lhs">The left hand side operand.</param>
        /// <param name="rhs">The right hand side operand.</param>
        /// <returns><c>true</c> if the operands are not equal, otherwise <c>false</c>.</returns>
        public static bool operator !=(Encoding lhs, Encoding rhs) => !Equals(lhs, rhs);

        /// <summary>Constructs a new Encoding.</summary>
        /// <param name="major">The major version number.</param>
        /// <param name="minor">The minor version number.</param>
        public Encoding(byte major, byte minor)
        {
            Major = major;
            Minor = minor;
        }

        /// <inheritdoc/>
        public override int GetHashCode() => HashCode.Combine(Major, Minor);

        /// <inheritdoc/>
        public bool Equals(Encoding other) => Major.Equals(other.Major) && Minor.Equals(other.Minor);

        /// <inheritdoc/>
        public override bool Equals(object? other) => other is Encoding value && Equals(value);

        /// <inheritdoc/>
        public override string ToString() => $"{Major}.{Minor}";

        internal void CheckSupported()
        {
            if (!IsSupported)
            {
                throw new NotSupportedException(
                    $"Ice encoding `{this}' is not supported by this Ice runtime ({Runtime.StringVersion})");
            }
        }
    }
}
