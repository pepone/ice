//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_VALUE_H
#define ICE_VALUE_H

#include "Config.h"
#include "SlicedDataF.h"
#include "ValueF.h"

#include <ostream>
#include <string>
#include <type_traits>

namespace Ice
{
    class OutputStream;
    class InputStream;

    /**
     * The base class for instances of Slice-defined classes.
     * \headerfile Ice/Ice.h
     */
    class ICE_API Value
    {
    public:
        // There is no copy constructor, move constructor, copy-assignment operator or move-assignment operator to
        // prevent accidental slicing.
        Value() noexcept = default;
        Value(Value&&) = delete;
        virtual ~Value() = default;

        Value& operator=(const Value&) = delete;
        Value& operator=(Value&&) = delete;

        /**
         * The Ice run time invokes this method prior to marshaling an object's data members. This allows a subclass
         * to override this method in order to validate its data members.
         */
        virtual void ice_preMarshal();

        /**
         * The Ice run time invokes this method after unmarshaling an object's data members. This allows a
         * subclass to override this method in order to perform additional initialization.
         */
        virtual void ice_postUnmarshal();

        /**
         * Obtains the Slice type ID of the most-derived class supported by this object.
         * @return The type ID.
         */
        [[nodiscard]] virtual const char* ice_id() const noexcept;

        /**
         * Obtains the Slice type ID of this type.
         * @return The return value is always "::Ice::Object".
         */
        static const char* ice_staticId() noexcept;

        /**
         * Creates a shallow polymorphic copy of this instance.
         * @return The cloned value.
         */
        [[nodiscard]] ValuePtr ice_clone() const { return _iceCloneImpl(); }

        /**
         * Obtains the sliced data associated with this instance.
         * @return The sliced data if the value has a preserved-slice base class and has been sliced during
         * unmarshaling of the value, nil otherwise.
         */
        [[nodiscard]] SlicedDataPtr ice_getSlicedData() const;

        /// Outputs a description of this instance to the stream. This description includes the type name and the name
        /// and value of all the fields of this instance.
        /// Use ["cpp:custom-print"] to tell the Slice compiler to generate an override implemented by the application.
        /// @param os The output stream.
        virtual void ice_print(std::ostream& os) const;

        /// Outputs the name and value of each field of this instance, including inherited fields, to the stream.
        /// Classes generated by the Slice compiler override this function.
        /// @param os The output stream.
        virtual void ice_printFields([[maybe_unused]] std::ostream& os) const {}

        /// \cond STREAM
        virtual void _iceWrite(Ice::OutputStream*) const;
        virtual void _iceRead(Ice::InputStream*);
        /// \endcond

    protected:
        /// \cond INTERNAL
        Value(const Value&) = default; // for clone

        // Helper class that allows derived classes to clone "this" even though the copy constructor is protected.
        template<class T> struct CloneEnabler : public T
        {
            CloneEnabler(const T& other) : T(other) {}
            static std::shared_ptr<T> clone(const T& other) { return std::make_shared<CloneEnabler>(other); }
        };

        [[nodiscard]] virtual ValuePtr _iceCloneImpl() const;
        /// \endcond

        /// \cond STREAM
        virtual void _iceWriteImpl(Ice::OutputStream*) const {}
        virtual void _iceReadImpl(Ice::InputStream*) {}
        /// \endcond

    private:
        SlicedDataPtr _slicedData;
    };

    /// Outputs the description of a class instance to the stream. This function calls ice_print on value.
    /// @param os The output stream.
    /// @param value The class instance.
    /// @return The output stream.
    inline std::ostream& operator<<(std::ostream& os, const Value& value)
    {
        value.ice_print(os);
        return os;
    }

    /// Outputs the description of a class instance held in a shared_ptr.
    /// @tparam T The class type.
    /// @param os The output stream.
    /// @param value The class instance held in a shared pointer. May be null.
    /// @return The output stream.
    template<class T, std::enable_if_t<std::is_base_of_v<Value, T>, bool> = true>
    inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<T>& value)
    {
        if (value)
        {
            os << *value;
        }
        else
        {
            os << "nullptr";
        }
        return os;
    }
}

#endif
