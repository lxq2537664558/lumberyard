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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzToolsFramework/Process/ProcessWatcher.h>
#include <AzToolsFramework/Process/ProcessCommunicator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/StringFunc/StringFunc.h>

using namespace AzFramework;


namespace UnitTest
{
    class ProcessLaunchParseTests
        : public AllocatorsFixture
    {
    public:
        using ParsedArgMap = AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>>;
        static ParsedArgMap ParseParameters(const AZStd::string& processOutput);
    };

    ProcessLaunchParseTests::ParsedArgMap ProcessLaunchParseTests::ParseParameters(const AZStd::string& processOutput)
    {
        ParsedArgMap parsedArgs;

        AZStd::string currentSwitch;
        bool inSwitches{ false };
        AZStd::vector<AZStd::string> parsedLines;
        AzFramework::StringFunc::Tokenize(processOutput.c_str(), parsedLines, "\r\n");

        for (const AZStd::string& thisLine : parsedLines)
        {
            if (thisLine == "Switch List:")
            {
                inSwitches = true;
                continue;
            }
            else if (thisLine == "End Switch List:")
            {
                inSwitches = false;
                continue;
            }

            if (thisLine.empty())
            {
                continue;
            }
            
            if(inSwitches)
            {
                if (thisLine[0] != ' ')
                {
                    currentSwitch = thisLine;
                }
                else
                {
                    parsedArgs[currentSwitch].push_back(thisLine.substr(1));
                }
            }
        }
        return parsedArgs;
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_LaunchBasicProcess_Success)
    {
        AzToolsFramework::ProcessOutput processOutput;
        AzToolsFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters = "ProcessLaunchTest";
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzToolsFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzToolsFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);
        EXPECT_EQ(processOutput.outputResult.empty(), false);
    }
   
    TEST_F(ProcessLaunchParseTests, ProcessLauncher_BasicParameter_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzToolsFramework::ProcessOutput processOutput;
        AzToolsFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters = "ProcessLaunchTest -param1 param1val -param2=param2val";
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzToolsFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzToolsFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 1);
        EXPECT_EQ(param1[0], "param1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 1);
        EXPECT_EQ(param2[0], "param2val");
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_StringsWithCommas_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzToolsFramework::ProcessOutput processOutput;
        AzToolsFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters = R"(ProcessLaunchTest -param1 "\"param,1val\"" -param2="\"param2v,al\"")";
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzToolsFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzToolsFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 1);
        EXPECT_EQ(param1[0], "param,1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 1);
        EXPECT_EQ(param2[0], "param2v,al");
    }
    
    TEST_F(ProcessLaunchParseTests, ProcessLauncher_StringsWithSpaces_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzToolsFramework::ProcessOutput processOutput;
        AzToolsFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters = R"(ProcessLaunchTest -param1 "\"param 1val\"" -param2="\"param2v al\"")";
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzToolsFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzToolsFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 1);
        EXPECT_EQ(param1[0], "param 1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 1);
        EXPECT_EQ(param2[0], "param2v al");
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_StringsWithSpacesAndComma_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzToolsFramework::ProcessOutput processOutput;
        AzToolsFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters = R"(ProcessLaunchTest -param1 "\"par,am 1val\"" -param2="\"param,2v al\"")";
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzToolsFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzToolsFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 1);
        EXPECT_EQ(param1[0], "par,am 1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 1);
        EXPECT_EQ(param2[0], "param,2v al");
    }

    TEST_F(ProcessLaunchParseTests, ProcessLauncher_CommaStringNoQuotes_Success)
    {
        ProcessLaunchParseTests::ParsedArgMap argMap;
        AzToolsFramework::ProcessOutput processOutput;
        AzToolsFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;

        processLaunchInfo.m_commandlineParameters = "ProcessLaunchTest -param1 param,1val -param2=param2v,al";
        processLaunchInfo.m_showWindow = false;
        bool launchReturn = AzToolsFramework::ProcessWatcher::LaunchProcessAndRetrieveOutput(processLaunchInfo, AzToolsFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_STDINOUT, processOutput);

        EXPECT_EQ(launchReturn, true);

        argMap = ProcessLaunchParseTests::ParseParameters(processOutput.outputResult);

        auto param1itr = argMap.find("param1");
        EXPECT_NE(param1itr, argMap.end());
        AZStd::vector<AZStd::string> param1{ param1itr->second };

        EXPECT_EQ(param1.size(), 2);
        EXPECT_EQ(param1[0], "param");
        EXPECT_EQ(param1[1], "1val");

        auto param2itr = argMap.find("param2");
        EXPECT_NE(param2itr, argMap.end());
        AZStd::vector<AZStd::string> param2{ param2itr->second };

        EXPECT_EQ(param2.size(), 2);
        EXPECT_EQ(param2[0], "param2v");
        EXPECT_EQ(param2[1], "al");
    }
}   // namespace UnitTest
