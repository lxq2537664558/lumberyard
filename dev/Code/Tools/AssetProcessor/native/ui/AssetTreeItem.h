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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <QString>

class QFileIconProvider;
class QIcon;
class QVariant;

namespace AssetProcessor
{
    class AssetTreeItemData
    {
    public:
        AZ_RTTI(AssetTreeItemData, "{5660BA97-C4B0-4E3B-A03B-9ACD9C67841B}");

        AssetTreeItemData(const AZStd::string& assetDbName, QString name, bool isFolder, const AZ::Uuid& uuid);
        virtual ~AssetTreeItemData() {}

        AZStd::string m_assetDbName;
        QString m_name;
        QString m_extension;
        AZ::Uuid m_uuid;
        bool m_isFolder = false;
    };

    enum class AssetTreeColumns
    {
        Name,
        Extension,
        Max
    };

    class AssetTreeItem
    {
    public:
        explicit AssetTreeItem(AZStd::shared_ptr<AssetTreeItemData> data, AssetTreeItem* parentItem = nullptr);
        virtual ~AssetTreeItem();

        AssetTreeItem* CreateChild(AZStd::shared_ptr<AssetTreeItemData> data);
        AssetTreeItem* GetChild(int row) const;

        void EraseChild(AssetTreeItem* child);

        int getChildCount() const;
        int GetColumnCount() const;
        int GetRow() const;
        QVariant GetDataForColumn(int column) const;
        QIcon GetIcon(const QFileIconProvider& iconProvider) const;
        AssetTreeItem* GetParent() const;
        AssetTreeItem* GetChildFolder(QString folder) const;

        AZStd::shared_ptr<AssetTreeItemData> GetData() const { return m_data; }

    private:
        AZStd::vector<AZStd::unique_ptr<AssetTreeItem>> m_childItems;
        AZStd::shared_ptr<AssetTreeItemData> m_data;
        AssetTreeItem* m_parent = nullptr;
    };
}
