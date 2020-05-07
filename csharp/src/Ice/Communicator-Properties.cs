//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace Ice
{
    public sealed partial class Communicator
    {
        private readonly Dictionary<string, PropertyValue> _properties = new Dictionary<string, PropertyValue>();

        private class PropertyValue
        {
            internal string Val;
            internal bool Used;

            internal PropertyValue(string v, bool u)
            {
                Val = v;
                Used = u;
            }
        }

        /// <summary>Gets the value of a property. If the property is not set, returns null.</summary>
        /// <param name="name">The property name.</param>
        /// <returns>The property value.</returns>
        public string? GetProperty(string name)
        {
            lock (_properties)
            {
                if (_properties.TryGetValue(name, out PropertyValue? pv))
                {
                    pv.Used = true;
                    return pv.Val;
                }
                return null;
            }
        }

        /// <summary>Gets the value of a property as a bool. If the property is not set, returns null.</summary>
        /// <param name="name">The property name.</param>
        /// <returns>True if the property value is parsed into an integer greater than 0; false of the property value
        /// is parsed into an integer smaller or equal to 0.</returns>
        public bool? GetPropertyAsBool(string name) =>
            GetPropertyAsInt(name) is int intValue ? intValue > 0 : (bool?)null;

        /// <summary>Gets the value of a property as an integer. If the property is not set, returns null.</summary>
        /// <param name="name">The property name.</param>
        /// <returns>The property value parsed into an integer or null.</returns>
        public int? GetPropertyAsInt(string name)
        {
            lock (_properties)
            {
                if (_properties.TryGetValue(name, out PropertyValue? pv))
                {
                    try
                    {
                        pv.Used = true;
                        return int.Parse(pv.Val, CultureInfo.InvariantCulture);
                    }
                    catch (FormatException ex)
                    {
                        throw new InvalidConfigurationException(
                            $"the value of property `{name}' is not an integer", ex);
                    }
                }
                return null;
            }
        }

        /// <summary>
        /// Gets the value of a property as an array of strings. If the property is not set, returns null.
        /// The value must contain strings separated by whitespace or comma. These strings can contain
        /// whitespace and commas if they are enclosed in single or double quotes. Within single quotes or
        /// double quotes, you can escape the quote in question with \, e.g. O'Reilly can be written as
        /// O'Reilly, "O'Reilly" or 'O\'Reilly'.
        /// </summary>
        /// <param name="name">The property name.</param>
        /// <returns>The property value parsed into an array of strings or null.</returns>
        public string[]? GetPropertyAsList(string name)
        {
            lock (_properties)
            {
                if (_properties.TryGetValue(name, out PropertyValue? pv))
                {
                    pv.Used = true;
                    return IceUtilInternal.StringUtil.SplitString(pv.Val, ", \t\r\n");
                }
                return null;
            }
        }

        /// <summary>Gets all properties whose keys begins with forPrefix.
        /// If forPrefix is an empty string, then all properties are returned.
        /// </summary>
        /// <param name="forPrefix">The prefix to search for (empty string if none).</param>
        /// <returns>The matching property set.</returns>
        public Dictionary<string, string> GetProperties(string forPrefix = "")
        {
            lock (_properties)
            {
                var result = new Dictionary<string, string>();

                foreach (string name in _properties.Keys)
                {
                    if (forPrefix.Length == 0 || name.StartsWith(forPrefix, StringComparison.Ordinal))
                    {
                        PropertyValue pv = _properties[name];
                        pv.Used = true;
                        result[name] = pv.Val;
                    }
                }
                return result;
            }
        }

        /// <summary>Gets the value of a property as a proxy. If the property is not set, returns null.</summary>
        /// <param name="name">The property name. The property name is also used as the prefix for proxy options.</param>
        /// <param name="factory">The proxy factory. Use IAPrx.Factory to create IAPrx proxies.</param>
        /// <returns>The property value parsed into a proxy or null.</returns>
        public T? GetPropertyAsProxy<T>(string name, ProxyFactory<T> factory) where T : class, IObjectPrx
        {
            if (GetProperty(name) is string value)
            {
                try
                {
                    return factory(Reference.Parse(value, name, this));
                }
                catch (FormatException ex)
                {
                    throw new InvalidConfigurationException($"the value of property `{name}' is not a proxy", ex);
                }
            }
            else
            {
                return null;
            }
        }

        /// <summary>Insert a new property or change the value of an existing property.
        /// Setting the value of a property to the empty string removes this property
        /// if it was present, and does nothing otherwise.</summary>
        /// <param name="name">The property name.</param>
        /// <param name="value">The property value.</param>
        public void SetProperty(string name, string value)
        {
            ValidatePropertyName(name);

            lock (_properties)
            {
                _ = SetPropertyImpl(name, value);
            }
        }

        /// <summary>Insert new properties or change the value of existing properties.
        /// Setting the value of a property to the empty string removes this property
        /// if it was present, and does nothing otherwise.</summary>
        /// <param name="updates">A dictionary of properties. This methods removes properties that did not change
        /// anything from this dictionary.</param>
        public void SetProperties(Dictionary<string, string> updates)
        {
            foreach (KeyValuePair<string, string> entry in updates)
            {
                ValidatePropertyName(entry.Key);
            }

            lock (_properties)
            {
                foreach (KeyValuePair<string, string> entry in updates)
                {
                    if (!SetPropertyImpl(entry.Key, entry.Value))
                    {
                        // Starting with .NET 3.0, Remove does not invalidate enumerators
                        updates.Remove(entry.Key);
                    }
                }
            }
        }

        /// <summary>Remove a property.</summary>
        /// <param name="name">The property name.</param>
        /// <returns>true if the property is successfully found and removed; otherwise false.</returns>
        public bool RemoveProperty(string name)
        {
            lock (_properties)
            {
                return _properties.Remove(name);
            }
        }

        /// <summary>Get all properties that were not read.</summary>
        /// <returns>The properties that were not read as a list of keys.</returns>
        public List<string> GetUnusedProperties()
        {
            lock (_properties)
            {
                return _properties.Where(p => !p.Value.Used).Select(p => p.Key).ToList();
            }
        }

        internal void CheckForUnknownProperties(string prefix)
        {
            // Do not warn about unknown properties if Ice prefix, ie Ice, Glacier2, etc
            foreach (string name in IceInternal.PropertyNames.clPropNames)
            {
                if (prefix.StartsWith(string.Format("{0}.", name), StringComparison.Ordinal))
                {
                    return;
                }
            }

            var unknownProps = new List<string>();
            Dictionary<string, string> props = GetProperties(forPrefix: $"{prefix}.");
            foreach (string prop in props.Keys)
            {
                bool valid = false;
                for (int i = 0; i < _suffixes.Length; ++i)
                {
                    string pattern = "^" + Regex.Escape(prefix + ".") + _suffixes[i] + "$";
                    if (new Regex(pattern).Match(prop).Success)
                    {
                        valid = true;
                        break;
                    }
                }

                if (!valid)
                {
                    unknownProps.Add(prop);
                }
            }

            if (unknownProps.Count != 0)
            {
                var message = new StringBuilder("found unknown properties for proxy '");
                message.Append(prefix);
                message.Append("':");
                foreach (string s in unknownProps)
                {
                    message.Append("\n    ");
                    message.Append(s);
                }
                Logger.Warning(message.ToString());
            }
        }

        // SetPropertyImpl sets a property and returns true when the property
        // was added, changed or removed, and false otherwise.
        private bool SetPropertyImpl(string name, string value)
        {
            // Must be called with a validated property and with _properties locked

            name = name.Trim();
            Debug.Assert(name.Length > 0);
            if (value.Length == 0)
            {
                return _properties.Remove(name);
            }
            else if (_properties.TryGetValue(name, out PropertyValue? pv))
            {
                if (pv.Val != value)
                {
                    pv.Val = value;
                    return true;
                }
                // else Val == value, nothing to do
            }
            else
            {
                // These properties are always marked "used"
                bool used = name == "Ice.ConfigFile" || name == "Ice.ProgramName";
                _properties[name] = new PropertyValue(value, used);
                return true;
            }
            return false;
        }

        private void ValidatePropertyName(string name)
        {
            name = name.Trim();
            if (name.Length == 0)
            {
                throw new ArgumentException("attempt to set property with empty key", nameof(name));
            }

            int dotPos = name.IndexOf('.');
            if (dotPos != -1)
            {
                string prefix = name.Substring(0, dotPos);
                foreach (IceInternal.Property[] validProps in IceInternal.PropertyNames.validProps)
                {
                    string pattern = validProps[0].Pattern();
                    dotPos = pattern.IndexOf('.');
                    Debug.Assert(dotPos != -1);
                    string propPrefix = pattern.Substring(1, dotPos - 2);
                    bool mismatchCase = false;
                    string otherKey = "";
                    if (!propPrefix.ToUpper().Equals(prefix.ToUpper()))
                    {
                        continue;
                    }

                    bool found = false;
                    foreach (IceInternal.Property prop in validProps)
                    {
                        var r = new Regex(prop.Pattern());
                        Match m = r.Match(name);
                        found = m.Success;
                        if (found)
                        {
                            if (prop.Deprecated())
                            {
                                Logger.Warning($"deprecated property: `{name}'");
                                string? deprecatedBy = prop.DeprecatedBy();
                                if (deprecatedBy != null)
                                {
                                    name = deprecatedBy;
                                }
                            }
                            break;
                        }

                        if (!found)
                        {
                            r = new Regex(prop.Pattern().ToUpper());
                            m = r.Match(name.ToUpper());
                            if (m.Success)
                            {
                                found = true;
                                mismatchCase = true;
                                otherKey = prop.Pattern().Replace("\\", "").Replace("^", "").Replace("$", "");
                                break;
                            }
                        }
                    }
                    if (!found)
                    {
                        Logger.Warning($"unknown property: `{name}'");
                    }
                    else if (mismatchCase)
                    {
                        Logger.Warning($"unknown property: `{name}'; did you mean `{otherKey}'");
                    }
                }
            }
        }
    }

    public static class PropertiesExtensions
    {
        /// <summary>Extract the reserved Ice properties from command-line args.</summary>
        /// <param name="into">The property dictionary into which the properties are added.</param>
        /// <param name="args">The command-line args.</param>
        public static void ParseIceArgs(this Dictionary<string, string> into, ref string[] args)
        {
            foreach (string name in IceInternal.PropertyNames.clPropNames)
            {
                into.ParseArgs(ref args, name);
            }
        }

        /// <summary>Extract properties from command-line args.</summary>
        /// <param name="into">The property dictionary into which the parsed properties are added.</param>
        /// <param name="args">The command-line args.</param>
        /// <param name="prefix">Only arguments that start with --prefix are extracted.</param>
        public static void ParseArgs(this Dictionary<string, string> into, ref string[] args, string prefix = "")
        {
            if (prefix.Length > 0 && !prefix.EndsWith("."))
            {
                prefix += '.';
            }
            prefix = "--" + prefix;

            if ((prefix == "--" || prefix == "--Ice.") && Array.Find(args, arg => arg == "--Ice.Config") != null)
            {
                throw new ArgumentException("--Ice.Config requires a value", nameof(args));
            }

            var remaining = new List<string>();
            var parsedArgs = new Dictionary<string, string>();
            foreach (string arg in args)
            {
                if (arg.StartsWith(prefix, StringComparison.Ordinal))
                {
                    (string name, string value) = ParseLine((arg.IndexOf('=') == -1 ? $"{arg}=1" : arg).Substring(2));
                    if (name.Length > 0)
                    {
                        parsedArgs[name] = value;
                        continue;
                    }
                }
                remaining.Add(arg);
            }

            if ((prefix == "--" || prefix == "--Ice.") &&
                    parsedArgs.TryGetValue("Ice.Config", out string? configFileList))
            {
                foreach (string file in configFileList.Split(","))
                {
                    into.LoadIceConfigFile(file);
                }
            }

            foreach (KeyValuePair<string, string> p in parsedArgs)
            {
                into[p.Key] = p.Value;
            }

            args = remaining.ToArray();
        }

        /// <summary>Load Ice configuration file.</summary>
        /// <param name="into">The property dictionary into which the loaded properties are added.</param>
        /// <param name="configFile">The path to the Ice configuration file to load.</param>
        public static void LoadIceConfigFile(this Dictionary<string, string> into, string configFile)
        {
            using var input = new System.IO.StreamReader(configFile.Trim());
            string? line;
            while ((line = input.ReadLine()) != null)
            {
                (string name, string value) = ParseLine(line);
                if (name.Length > 0)
                {
                    into[name] = value;
                }
            }
        }

        internal enum ParseState : byte
        {
            Key,
            Value
        }

        private static (string Name, string Value) ParseLine(string line)
        {
            var name = new StringBuilder();
            var value = new StringBuilder();

            ParseState state = ParseState.Key;

            var whitespace = new StringBuilder();
            var escapedspace = new StringBuilder();
            bool finished = false;
            for (int i = 0; i < line.Length; ++i)
            {
                char c = line[i];
                switch (state)
                {
                    case ParseState.Key:
                        {
                            switch (c)
                            {
                                case '\\':
                                    if (i < line.Length - 1)
                                    {
                                        c = line[++i];
                                        switch (c)
                                        {
                                            case '\\':
                                            case '#':
                                            case '=':
                                                name.Append(whitespace);
                                                whitespace.Clear();
                                                name.Append(c);
                                                break;

                                            case ' ':
                                                if (name.Length != 0)
                                                {
                                                    whitespace.Append(c);
                                                }
                                                break;

                                            default:
                                                name.Append(whitespace);
                                                whitespace.Clear();
                                                name.Append('\\');
                                                name.Append(c);
                                                break;
                                        }
                                    }
                                    else
                                    {
                                        name.Append(whitespace);
                                        name.Append(c);
                                    }
                                    break;

                                case ' ':
                                case '\t':
                                case '\r':
                                case '\n':
                                    if (name.Length != 0)
                                    {
                                        whitespace.Append(c);
                                    }
                                    break;

                                case '=':
                                    whitespace.Clear();
                                    state = ParseState.Value;
                                    break;

                                case '#':
                                    finished = true;
                                    break;

                                default:
                                    name.Append(whitespace);
                                    whitespace.Clear();
                                    name.Append(c);
                                    break;
                            }
                            break;
                        }

                    case ParseState.Value:
                        {
                            switch (c)
                            {
                                case '\\':
                                    if (i < line.Length - 1)
                                    {
                                        c = line[++i];
                                        switch (c)
                                        {
                                            case '\\':
                                            case '#':
                                            case '=':
                                                value.Append(value.Length == 0 ? escapedspace : whitespace);
                                                whitespace.Clear();
                                                escapedspace.Clear();
                                                value.Append(c);
                                                break;

                                            case ' ':
                                                whitespace.Append(c);
                                                escapedspace.Append(c);
                                                break;

                                            default:
                                                value.Append(value.Length == 0 ? escapedspace : whitespace);
                                                whitespace.Clear();
                                                escapedspace.Clear();
                                                value.Append('\\');
                                                value.Append(c);
                                                break;
                                        }
                                    }
                                    else
                                    {
                                        value.Append(value.Length == 0 ? escapedspace : whitespace);
                                        value.Append(c);
                                    }
                                    break;

                                case ' ':
                                case '\t':
                                case '\r':
                                case '\n':
                                    if (value.Length != 0)
                                    {
                                        whitespace.Append(c);
                                    }
                                    break;

                                case '#':
                                    finished = true;
                                    break;

                                default:
                                    value.Append(value.Length == 0 ? escapedspace : whitespace);
                                    whitespace.Clear();
                                    escapedspace.Clear();
                                    value.Append(c);
                                    break;
                            }
                            break;
                        }
                }
                if (finished)
                {
                    break;
                }
            }
            value.Append(escapedspace);

            if ((state == ParseState.Key && name.Length != 0) || (state == ParseState.Value && name.Length == 0))
            {
                throw new FormatException($"invalid config file entry: \"{line}\"");
            }

            return (name.ToString(), value.ToString());
        }
    }
}
