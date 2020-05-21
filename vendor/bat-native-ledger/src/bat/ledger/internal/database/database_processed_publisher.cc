/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/database/database_processed_publisher.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "processed_publisher";

}  // namespace

namespace braveledger_database {

DatabaseProcessedPublisher::DatabaseProcessedPublisher(
    bat_ledger::LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabaseProcessedPublisher::~DatabaseProcessedPublisher() = default;

bool DatabaseProcessedPublisher::CreateTableV22(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s ("
        "publisher_key TEXT PRIMARY KEY NOT NULL,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
      ")",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  return true;
}

bool DatabaseProcessedPublisher::Migrate(
    ledger::DBTransaction* transaction,
    const int target) {
  DCHECK(transaction);

  switch (target) {
    case 22: {
      return MigrateToV22(transaction);
    }
    default: {
      return true;
    }
  }
}

bool DatabaseProcessedPublisher::MigrateToV22(
    ledger::DBTransaction* transaction) {
  DCHECK(transaction);

  if (!DropTable(transaction, kTableName)) {
    BLOG(0, "Table couldn't be dropped");
    return false;
  }

  if (!CreateTableV22(transaction)) {
    BLOG(0, "Table couldn't be created");
    return false;
  }

  return true;
}

void DatabaseProcessedPublisher::InsertOrUpdateList(
    const std::vector<std::string>& list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    BLOG(0, "List is empty");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s (publisher_key) VALUES (?);",
      kTableName);

  for (const auto& publisher_key : list) {
    auto command = ledger::DBCommand::New();
    command->type = ledger::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, publisher_key);

    transaction->commands.push_back(std::move(command));
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseProcessedPublisher::WasProcessed(
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (publisher_key.empty()) {
    BLOG(0, "Publisher key is empty");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = ledger::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT publisher_key FROM %s WHERE publisher_key = ?",
      kTableName);

  auto command = ledger::DBCommand::New();
  command->type = ledger::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      ledger::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseProcessedPublisher::OnWasProcessed,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseProcessedPublisher::OnWasProcessed(
    ledger::DBCommandResponsePtr response,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != ledger::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (response->result->get_records().empty()) {
    callback(ledger::Result::NOT_FOUND);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_database
