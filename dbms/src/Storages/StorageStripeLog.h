#pragma once

#include <map>
#include <shared_mutex>

#include <ext/shared_ptr_helper.h>

#include <Core/Defines.h>
#include <Storages/IStorage.h>
#include <Common/FileChecker.h>
#include <Common/escapeForFileName.h>


namespace DB
{
/** Implements a table engine that is suitable for small chunks of the log.
  * In doing so, stores all the columns in a single Native file, with a nearby index.
  */
class StorageStripeLog : public ext::shared_ptr_helper<StorageStripeLog>, public IStorage
{
    friend class StripeLogBlockInputStream;
    friend class StripeLogBlockOutputStream;
    friend struct ext::shared_ptr_helper<StorageStripeLog>;

public:
    String getName() const override { return "StripeLog"; }
    String getTableName() const override { return table_name; }
    String getDatabaseName() const override { return database_name; }

    BlockInputStreams read(
        const Names & column_names,
        const SelectQueryInfo & query_info,
        const Context & context,
        QueryProcessingStage::Enum processed_stage,
        size_t max_block_size,
        unsigned num_streams) override;

    BlockOutputStreamPtr write(const ASTPtr & query, const Context & context) override;

    void rename(
        const String & new_path_to_table_data,
        const String & new_database_name,
        const String & new_table_name,
        TableStructureWriteLockHolder &) override;

    CheckResults checkData(const ASTPtr & /* query */, const Context & /* context */) override;

    Strings getDataPaths() const override { return {DB::fullPath(disk, table_path)}; }

    void truncate(const ASTPtr &, const Context &, TableStructureWriteLockHolder &) override;

protected:
    StorageStripeLog(
        DiskPtr disk_,
        const String & relative_path_,
        const String & database_name_,
        const String & table_name_,
        const ColumnsDescription & columns_,
        const ConstraintsDescription & constraints_,
        bool attach,
        size_t max_compress_block_size_);

private:
    struct ColumnData
    {
        String data_file_path;
    };
    using Files = std::map<String, ColumnData>; /// file name -> column data

    DiskPtr disk;
    String table_path;
    String database_name;
    String table_name;

    size_t max_compress_block_size;

    FileChecker file_checker;
    mutable std::shared_mutex rwlock;

    Logger * log;
};

}
