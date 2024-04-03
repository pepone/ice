//
// Copyright (c) ZeroC, Inc. All rights reserved.
//


namespace Ice;

public delegate Value ValueFactory(string type);

public interface ValueFactoryManager
{
    /// <summary>
    /// Add a value factory.
    /// Attempting to add a factory with an id for which a factory is already registered throws
    ///  AlreadyRegisteredException.
    ///  When unmarshaling an Ice value, the Ice run time reads the most-derived type id off the wire and attempts to
    ///  create an instance of the type using a factory. If no instance is created, either because no factory was found,
    ///  or because all factories returned nil, the behavior of the Ice run time depends on the format with which the
    ///  value was marshaled:
    ///  If the value uses the "sliced" format, Ice ascends the class hierarchy until it finds a type that is recognized
    ///  by a factory, or it reaches the least-derived type. If no factory is found that can create an instance, the run
    ///  time throws NoValueFactoryException.
    ///  If the value uses the "compact" format, Ice immediately raises NoValueFactoryException.
    ///  The following order is used to locate a factory for a type:
    ///
    ///  The Ice run-time looks for a factory registered specifically for the type.
    ///  If no instance has been created, the Ice run-time looks for the default factory, which is registered with
    ///  an empty type id.
    ///  If no instance has been created by any of the preceding steps, the Ice run-time looks for a factory that
    ///  may have been statically generated by the language mapping for non-abstract classes.
    ///
    /// </summary>
    ///  <param name="factory">The factory to add.
    ///  </param>
    /// <param name="id">The type id for which the factory can create instances, or an empty string for the default factory.</param>
    void add(ValueFactory factory, string id);

    /// <summary>
    /// Find an value factory registered with this communicator.
    /// </summary>
    /// <param name="id">The type id for which the factory can create instances, or an empty string for the default factory.
    ///  </param>
    /// <returns>The value factory, or null if no value factory was found for the given id.</returns>
    ValueFactory find(string id);
}
