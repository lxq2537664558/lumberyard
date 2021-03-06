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
#pragma once

#include <AzCore/EBus/EBus.h>

namespace EditorPythonBindings
{
    //! An interface to track exported Python symbols 
    class PythonSymbolEvents
        : public AZ::EBusTraits
    {
    public:
        //! logs a behavior class type
        virtual void LogClass(AZStd::string_view moduleName, AZ::BehaviorClass* behaviorClass) = 0;

        //! logs a static class method with a specified global method name
        virtual void LogClassMethod(AZStd::string_view moduleName, AZStd::string_view globalMethodName, AZ::BehaviorClass* behaviorClass, AZ::BehaviorMethod* behaviorMethod) = 0;

        //! logs a behavior bus with a specified bus name
        virtual void LogBus(AZStd::string_view moduleName, AZStd::string_view busName, AZ::BehaviorEBus* behaviorEBus) = 0;

        //! logs a global method from the behavior context registry with a specified method name
        virtual void LogGlobalMethod(AZStd::string_view moduleName, AZStd::string_view methodName, AZ::BehaviorMethod* behaviorMethod) = 0;

        //! logs a global property, enum, or constant from the behavior context registry with a specified property name
        virtual void LogGlobalProperty(AZStd::string_view moduleName, AZStd::string_view propertyName, AZ::BehaviorProperty* behaviorProperty) = 0;

        //! signals the end of the logging of symbols
        virtual void Finalize() = 0;
    };
    using PythonSymbolEventBus = AZ::EBus<PythonSymbolEvents>;

} // namespace EditorPythonBindings

