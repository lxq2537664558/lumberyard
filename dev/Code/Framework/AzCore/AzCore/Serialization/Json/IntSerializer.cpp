/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Serialization/Json/IntSerializer.h>

#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonStringConversionUtils.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <cerrno>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(JsonCharSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonShortSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonIntSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonLongSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonLongLongSerializer, SystemAllocator, 0);

    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedCharSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedShortSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedIntSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedLongSerializer, SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(JsonUnsignedLongLongSerializer, SystemAllocator, 0);

    namespace SerializerInternal
    {
        // Below are some additional helper functions which are really only necessary until we can have if constexpr() support from C++17
        // so that our above code will the correct signed or unsigned function calls at compile time instead of branching

        template <typename T, AZStd::enable_if_t<AZStd::is_signed<T>::value, int> = 0>
        static void StoreToValue(rapidjson::Value& outputValue, T inputValue)
        {
            outputValue.SetInt64(inputValue);
        }

        template <typename T, AZStd::enable_if_t<AZStd::is_unsigned<T>::value, int> = 0>
        static void StoreToValue(rapidjson::Value& outputValue, T inputValue)
        {
            outputValue.SetUint64(inputValue);
        }

        template <typename T>
        static JsonSerializationResult::Result LoadInt(T* outputValue, const rapidjson::Value& inputValue,
            StackedString& path, const JsonDeserializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            static_assert(AZStd::is_integral<T>(), "Expected T to be a signed or unsigned type");
            AZ_Assert(outputValue, "Expected a valid pointer to load from json value.");

            switch (inputValue.GetType())
            {
            case rapidjson::kArrayType:
            // fallthrough
            case rapidjson::kObjectType:
            // fallthrough
            case rapidjson::kNullType:
                return JSR::Result(settings, "Unsupported type. Integers can't be read from arrays, objects or null.",
                    Tasks::ReadField, Outcomes::Unsupported, path);

            case rapidjson::kStringType:
                return TextToValue(outputValue, inputValue.GetString(), path, settings);

            case rapidjson::kFalseType:
            // fallthrough
            case rapidjson::kTrueType:
                *outputValue = inputValue.GetBool() ? 1 : 0;
                return JSR::Result(settings, "Successfully converted boolean to integer value.", ResultCode::Success(Tasks::ReadField), path);

            case rapidjson::kNumberType:
            {
                ResultCode result(Tasks::ReadField);
                if (inputValue.IsInt64())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetInt64(), path, settings.m_reporting);
                }
                else if (inputValue.IsDouble())
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetDouble(), path, settings.m_reporting);
                }
                else
                {
                    result = JsonNumericCast<T>(*outputValue, inputValue.GetUint64(), path, settings.m_reporting);
                }

                return JSR::Result(settings, result.GetOutcome() == Outcomes::Success ?
                    "Successfully read integer value from number field." : "Failed to read integer value from number field.", result, path);
            }

            default:
                return JSR::Result(settings, "Unknown json type encountered for integer.",  Tasks::ReadField, Outcomes::Unknown, path);
            }
        }

        template <typename T>
        static JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            StackedString& path, const JsonSerializerSettings& settings)
        {
            using namespace JsonSerializationResult;
            namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

            const T inputValAsType = *reinterpret_cast<const T*>(inputValue);
            if (settings.m_keepDefaults || !defaultValue || (inputValAsType != *reinterpret_cast<const T*>(defaultValue)))
            {
                StoreToValue(outputValue, inputValAsType);
                return JSR::Result(settings, "Successfully stored integer value.", ResultCode::Success(Tasks::WriteValue), path);
            }
            
            return JSR::Result(settings, "Skipped integer value because default was used.", ResultCode::Default(Tasks::WriteValue), path);
        }
    } // namespace SerializerInternal

    JsonSerializationResult::Result JsonCharSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<char>() == outputValueTypeId,
            "Unable to deserialize char to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<char*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonCharSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<char>() == valueTypeId, "Unable to serialize char to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<char>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonShortSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<short>() == outputValueTypeId,
            "Unable to deserialize short to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<short*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonShortSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<short>() == valueTypeId, "Unable to serialize short to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str()); 
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<short>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonIntSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<int>() == outputValueTypeId,
            "Unable to deserialize int to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<int*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonIntSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<int>() == valueTypeId, "Unable to serialize int to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<int>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<long>() == outputValueTypeId,
            "Unable to deserialize long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<long*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonLongSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<long>() == valueTypeId, "Unable to serialize long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<long>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonLongLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<long long>() == outputValueTypeId,
            "Unable to deserialize long long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<long long*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonLongLongSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<long long>() == valueTypeId, "Unable to serialize long long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str()); 
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<long long>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedCharSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned char>() == outputValueTypeId,
            "Unable to deserialize unsigned char to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<unsigned char*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedCharSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned char>() == valueTypeId, "Unable to serialize unsigned char to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str()); 
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned char>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedShortSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned short>() == outputValueTypeId,
            "Unable to deserialize unsigned short to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<unsigned short*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedShortSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned short>() == valueTypeId, "Unable to serialize unsigned short to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned short>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedIntSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned int>() == outputValueTypeId,
            "Unable to deserialize unsigned int to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<unsigned int*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedIntSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned int>() == valueTypeId, "Unable to serialize unsigned int to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned int>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned long>() == outputValueTypeId,
            "Unable to deserialize unsigned long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<unsigned long*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedLongSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned long>() == valueTypeId, "Unable to serialize unsigned long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned long>(outputValue, inputValue, defaultValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedLongLongSerializer::Load(void* outputValue, const Uuid& outputValueTypeId, const rapidjson::Value& inputValue,
        StackedString& path, const JsonDeserializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned long long>() == outputValueTypeId,
            "Unable to deserialize unsigned long long to json because the provided type is %s",
            outputValueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(outputValueTypeId);
        return SerializerInternal::LoadInt(reinterpret_cast<unsigned long long*>(outputValue), inputValue, path, settings);
    }

    JsonSerializationResult::Result JsonUnsignedLongLongSerializer::Store(rapidjson::Value& outputValue, rapidjson::Document::AllocatorType& /*allocator*/,
        const void* inputValue, const void* defaultValue, const Uuid& valueTypeId, StackedString& path, const JsonSerializerSettings& settings)
    {
        AZ_Assert(azrtti_typeid<unsigned long long>() == valueTypeId, "Unable to serialize unsigned long long to json because the provided type is %s",
            valueTypeId.ToString<AZStd::string>().c_str());
        AZ_UNUSED(valueTypeId);
        return SerializerInternal::Store<unsigned long long>(outputValue, inputValue, defaultValue, path, settings);
    }
} // namespace AZ
