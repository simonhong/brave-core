/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_promotion.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace braveledger_database {

namespace {

const char kTableName[] = "promotion";

}  // namespace

DatabasePromotion::DatabasePromotion(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger),
    creds_(std::make_unique<DatabasePromotionCreds>(ledger)) {
}

DatabasePromotion::~DatabasePromotion() = default;

bool DatabasePromotion::CreateTableV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "promotion_id TEXT NOT NULL,"
        "version INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "public_keys TEXT NOT NULL,"
        "suggestions INTEGER NOT NULL DEFAULT 0,"
        "approximate_value DOUBLE NOT NULL DEFAULT 0,"
        "status INTEGER NOT NULL DEFAULT 0,"
        "expires_at TIMESTAMP NOT NULL,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (promotion_id)"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotion::CreateIndexV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return this->InsertIndex(transaction, kTableName, "promotion_id");
}

bool DatabasePromotion::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 10: {
      return MigrateToV10(transaction);
    }
    case 13: {
      return MigrateToV13(transaction);
    }
    case 14: {
      return MigrateToV14(transaction);
    }
    case 15: {
      return MigrateToV15(transaction);
    }
    case 18: {
      return MigrateToV18(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabasePromotion::MigrateToV10(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!CreateTableV10(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  if (!CreateIndexV10(transaction)) {
    BLOG(0, "Index couldn't be created");
    return false;
  }

  if (!creds_->Migrate(transaction, 10)) {
    return false;
  }

  return true;
}

bool DatabasePromotion::MigrateToV13(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const char column[] = "claimed_at";
  const std::string query = base::StringPrintf(
      "ALTER TABLE %s ADD %s TIMESTAMP",
      kTableName,
      column);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotion::MigrateToV14(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "UPDATE %s SET approximate_value = "
      "(SELECT (suggestions * 0.25) FROM %s as ps "
      "WHERE ps.promotion_id = %s.promotion_id)",
      kTableName,
      kTableName,
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabasePromotion::MigrateToV15(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  return creds_->Migrate(transaction, 15);
}

bool DatabasePromotion::MigrateToV18(ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const char column[] = "claim_id";
  std::string query = base::StringPrintf(
      "ALTER TABLE %s ADD %s TEXT",
      kTableName,
      column);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  query = base::StringPrintf(
      "UPDATE %s as p SET claim_id = "
      "(SELECT claim_id FROM promotion_creds as pc "
      "WHERE pc.promotion_id = p.promotion_id)",
      kTableName);

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  query = base::StringPrintf(
      "UPDATE %s SET status = 1 WHERE status = 2 OR status = 3",
      kTableName);

  command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  return creds_->Migrate(transaction, 18);
}

void DatabasePromotion::InsertOrUpdate(
    ledger::PromotionPtr info,
    ledger::ResultCallback callback) {
  if (!info) {
    BLOG(0, "Info is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, info->version);
  BindInt(command.get(), 2, static_cast<int>(info->type));
  BindString(command.get(), 3, info->public_keys);
  BindInt64(command.get(), 4, info->suggestions);
  BindDouble(command.get(), 5, info->approximate_value);
  BindInt(command.get(), 6, static_cast<int>(info->status));
  BindInt64(command.get(), 7, info->expires_at);
  BindInt64(command.get(), 8, info->claimed_at);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::GetRecord(
    const std::string& id,
    ledger::GetPromotionCallback callback) {
  if (id.empty()) {
    BLOG(0, "Id is empty");
    return callback({});
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at, claim_id "
      "FROM %s WHERE promotion_id=?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, id);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetRecord,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::OnGetRecord(
    ledger::DBCommandResponsePtr response,
    ledger::GetPromotionCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    BLOG(0, "Record size is not correct");
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = ledger::Promotion::New();
  info->id = GetStringColumn(record, 0);
  info->version = GetIntColumn(record, 1);
  info->type = static_cast<ledger::PromotionType>(GetIntColumn(record, 2));
  info->public_keys = GetStringColumn(record, 3);
  info->suggestions = GetInt64Column(record, 4);
  info->approximate_value = GetDoubleColumn(record, 5);
  info->status = static_cast<ledger::PromotionStatus>(GetIntColumn(record, 6));
  info->expires_at = GetInt64Column(record, 7);
  info->claimed_at = GetInt64Column(record, 8);
  info->claim_id = GetStringColumn(record, 9);

  callback(std::move(info));
}

void DatabasePromotion::GetAllRecords(
    ledger::GetAllPromotionsCallback callback) {
  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at, claim_id "
      "FROM %s",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::OnGetAllRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetAllPromotionsCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  ledger::PromotionMap map;
  for (auto const& record : response->result->get_records()) {
    auto info = ledger::Promotion::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->version = GetIntColumn(record_pointer, 1);
    info->type =
        static_cast<ledger::PromotionType>(GetIntColumn(record_pointer, 2));
    info->public_keys = GetStringColumn(record_pointer, 3);
    info->suggestions = GetInt64Column(record_pointer, 4);
    info->approximate_value = GetDoubleColumn(record_pointer, 5);
    info->status =
        static_cast<ledger::PromotionStatus>(GetIntColumn(record_pointer, 6));
    info->expires_at = GetInt64Column(record_pointer, 7);
    info->claimed_at = GetInt64Column(record_pointer, 8);
    info->claim_id = GetStringColumn(record_pointer, 9);

    map.insert(std::make_pair(info->id, std::move(info)));
  }

  callback(std::move(map));
}

void DatabasePromotion::DeleteRecordList(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    BLOG(0, "List of ids is empty");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  const std::string query = base::StringPrintf(
      "DELETE FROM %s WHERE promotion_id IN (%s)",
      kTableName,
      GenerateStringInCase(ids).c_str());

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::SaveClaimId(
    const std::string& promotion_id,
    const std::string& claim_id,
    ledger::ResultCallback callback) {
  if (promotion_id.empty() || claim_id.empty()) {
    BLOG(0, "Data is empty " << promotion_id << "/" << claim_id);
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET claim_id = ? WHERE promotion_id = ?",
      kTableName);

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, claim_id);
  BindString(command.get(), 1, promotion_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::UpdateStatus(
    const std::string& promotion_id,
    const ledger::PromotionStatus status,
    ledger::ResultCallback callback) {
  if (promotion_id.empty()) {
    BLOG(0, "Promotion id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE promotion_id = ?",
      kTableName);

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, promotion_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::UpdateRecordsStatus(
    const std::vector<std::string>& ids,
    const ledger::PromotionStatus status,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    BLOG(0, "List of ids is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE promotion_id IN (%s)",
      kTableName,
      GenerateStringInCase(ids).c_str());

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::CredentialCompleted(
    const std::string& promotion_id,
    ledger::ResultCallback callback) {
  if (promotion_id.empty()) {
    BLOG(0, "Promotion id is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ?, claimed_at = ? WHERE promotion_id = ?",
      kTableName);

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::RUN;
  command->command = query;

  const uint64_t current_time = braveledger_time_util::GetCurrentTimeStamp();

  BindInt(command.get(), 0,
      static_cast<int>(ledger::PromotionStatus::FINISHED));
  BindInt64(command.get(), 1, current_time);
  BindString(command.get(), 2, promotion_id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::GetRecords(
    const std::vector<std::string>& ids,
    ledger::GetPromotionListCallback callback) {
  if (ids.empty()) {
    BLOG(0, "List of ids is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at, claim_id "
      "FROM %s WHERE promotion_id IN (%s)",
      kTableName,
      GenerateStringInCase(ids).c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetRecords,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::OnGetRecords(
    ledger::DBCommandResponsePtr response,
    ledger::GetPromotionListCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  ledger::PromotionList list;
  ledger::PromotionPtr info;
  for (auto const& record : response->result->get_records()) {
    info = ledger::Promotion::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->version = GetIntColumn(record_pointer, 1);
    info->type =
        static_cast<ledger::PromotionType>(GetIntColumn(record_pointer, 2));
    info->public_keys = GetStringColumn(record_pointer, 3);
    info->suggestions = GetInt64Column(record_pointer, 4);
    info->approximate_value = GetDoubleColumn(record_pointer, 5);
    info->status =
        static_cast<ledger::PromotionStatus>(GetIntColumn(record_pointer, 6));
    info->expires_at = GetInt64Column(record_pointer, 7);
    info->claimed_at = GetInt64Column(record_pointer, 8);
    info->claim_id = GetStringColumn(record_pointer, 9);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabasePromotion::GetRecordsByType(
    const std::vector<ledger::PromotionType>& types,
    ledger::GetPromotionListCallback callback) {
  if (types.empty()) {
    BLOG(0, "List of types is empty");
    callback({});
    return;
  }
  auto transaction = ledger::DBTransaction::New();

  std::vector<std::string> in_case;

  for (const auto& type : types) {
    in_case.push_back(std::to_string(static_cast<int>(type)));
  }

  const std::string query = base::StringPrintf(
      "SELECT promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, expires_at, claimed_at, claim_id "
      "FROM %s WHERE type IN (%s)",
      kTableName,
      base::JoinString(in_case, ",").c_str());

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::DOUBLE_TYPE,
      ledger::DBCommand::RecordBindingType::INT_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::INT64_TYPE,
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePromotion::OnGetRecords,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabasePromotion::UpdateRecordsBlankPublicKey(
    const std::vector<std::string>& ids,
    ledger::ResultCallback callback) {
  if (ids.empty()) {
    BLOG(0, "List of ids is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s as p SET public_keys = "
      "(SELECT PRINTF('[\"%%s\"]', public_key) FROM creds_batch as cb "
      "WHERE cb.trigger_id = p.promotion_id) WHERE p.promotion_id IN (%s)",
      kTableName,
      GenerateStringInCase(ids).c_str());

  auto transaction = ledger::DBTransaction::New();
  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace braveledger_database
