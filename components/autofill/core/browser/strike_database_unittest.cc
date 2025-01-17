// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/strike_database.h"

#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/autofill/core/browser/proto/strike_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {
namespace {

// Note: This class is NOT the same as test_strike_database.h. This is an
// actual implementation of StrikeDatabase, but with helper functions
// added for easier test setup. If you want a TestStrikeDatabase, please use the
// one in test_strike_database.h.  This one is purely for this unit test class.
class TestStrikeDatabase : public StrikeDatabase {
 public:
  TestStrikeDatabase(const base::FilePath& database_dir)
      : StrikeDatabase(database_dir) {
    database_initialized_ = true;
  }

  void AddProtoEntries(
      std::vector<std::pair<std::string, StrikeData>> entries_to_add,
      const SetValueCallback& callback) {
    std::unique_ptr<leveldb_proto::ProtoDatabase<StrikeData>::KeyEntryVector>
        entries(new leveldb_proto::ProtoDatabase<StrikeData>::KeyEntryVector());
    for (std::pair<std::string, StrikeData> entry : entries_to_add) {
      entries->push_back(entry);
    }
    db_->UpdateEntries(
        /*entries_to_save=*/std::move(entries),
        /*keys_to_remove=*/std::make_unique<std::vector<std::string>>(),
        callback);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestStrikeDatabase);

  // Do not use. This virtual function needed to be implemented but
  // TestStrikeDatabase is not a project class.
  std::string GetProjectPrefix() override {
    NOTIMPLEMENTED();
    return " ";
  }

  // Do not use. This virtual function needed to be implemented but
  // TestStrikeDatabase is not a project class.
  int GetMaxStrikesLimit() override {
    NOTIMPLEMENTED();
    return 0;
  }

  // Do not use. This virtual function needed to be implemented but
  // TestStrikeDatabase is not a project class.
  long long GetExpiryTimeMicros() override {
    NOTIMPLEMENTED();
    return 0;
  }
};

}  // anonymous namespace

// Runs tests against the actual StrikeDatabase class, complete with
// ProtoDatabase.
class StrikeDatabaseTest : public ::testing::Test {
 public:
  StrikeDatabaseTest() : strike_database_(InitFilePath()) {}

  void AddProtoEntries(
      std::vector<std::pair<std::string, StrikeData>> entries_to_add) {
    base::RunLoop run_loop;
    strike_database_.AddProtoEntries(
        entries_to_add,
        base::BindRepeating(&StrikeDatabaseTest::OnAddProtoEntries,
                            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  void OnAddProtoEntries(base::RepeatingClosure run_loop_closure,
                         bool success) {
    run_loop_closure.Run();
  }

  void OnGetProtoStrikes(base::RepeatingClosure run_loop_closure,
                         int num_strikes) {
    num_strikes_ = num_strikes;
    run_loop_closure.Run();
  }

  int GetProtoStrikes(std::string key) {
    base::RunLoop run_loop;
    strike_database_.GetProtoStrikes(
        key,
        base::BindRepeating(&StrikeDatabaseTest::OnGetProtoStrikes,
                            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
    return num_strikes_;
  }

  void OnClearAllProtoStrikesForKey(base::RepeatingClosure run_loop_closure,
                                    bool success) {
    run_loop_closure.Run();
  }

  void ClearAllProtoStrikesForKey(const std::string key) {
    base::RunLoop run_loop;
    strike_database_.ClearAllProtoStrikesForKey(
        key,
        base::BindRepeating(&StrikeDatabaseTest::OnClearAllProtoStrikesForKey,
                            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  void OnClearAllProtoStrikes(base::RepeatingClosure run_loop_closure,
                              bool success) {
    run_loop_closure.Run();
  }

  void ClearAllProtoStrikes() {
    base::RunLoop run_loop;
    strike_database_.ClearAllProtoStrikes(
        base::BindRepeating(&StrikeDatabaseTest::OnClearAllProtoStrikesForKey,
                            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

 protected:
  base::HistogramTester* GetHistogramTester() { return &histogram_tester_; }
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestStrikeDatabase strike_database_;

 private:
  static const base::FilePath InitFilePath() {
    base::ScopedTempDir temp_dir_;
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath file_path =
        temp_dir_.GetPath().AppendASCII("StrikeDatabaseTest");
    return file_path;
  }

  base::HistogramTester histogram_tester_;
  int num_strikes_;
  std::unique_ptr<StrikeData> strike_data_;
};

TEST_F(StrikeDatabaseTest, GetStrikesForMissingKeyTest) {
  const std::string key = "12345";
  int strikes = GetProtoStrikes(key);
  EXPECT_EQ(0, strikes);
}

TEST_F(StrikeDatabaseTest, GetStrikeForNonZeroStrikesTest) {
  // Set up database with 3 pre-existing strikes at |key|.
  const std::string key = "12345";
  std::vector<std::pair<std::string, StrikeData>> entries;
  StrikeData data;
  data.set_num_strikes(3);
  entries.push_back(std::make_pair(key, data));
  AddProtoEntries(entries);

  int strikes = GetProtoStrikes(key);
  EXPECT_EQ(3, strikes);
}

TEST_F(StrikeDatabaseTest, ClearStrikesForMissingKeyTest) {
  const std::string key = "12345";
  ClearAllProtoStrikesForKey(key);
  int strikes = GetProtoStrikes(key);
  EXPECT_EQ(0, strikes);
}

TEST_F(StrikeDatabaseTest, ClearStrikesForNonZeroStrikesTest) {
  // Set up database with 3 pre-existing strikes at |key|.
  const std::string key = "12345";
  std::vector<std::pair<std::string, StrikeData>> entries;
  StrikeData data;
  data.set_num_strikes(3);
  entries.push_back(std::make_pair(key, data));
  AddProtoEntries(entries);

  int strikes = GetProtoStrikes(key);
  EXPECT_EQ(3, strikes);
  ClearAllProtoStrikesForKey(key);
  strikes = GetProtoStrikes(key);
  EXPECT_EQ(0, strikes);
}

TEST_F(StrikeDatabaseTest, ClearStrikesForMultipleNonZeroStrikesEntriesTest) {
  // Set up database with 3 pre-existing strikes at |key1|, and 5 pre-existing
  // strikes at |key2|.
  const std::string key1 = "12345";
  const std::string key2 = "13579";
  std::vector<std::pair<std::string, StrikeData>> entries;
  StrikeData data1;
  data1.set_num_strikes(3);
  entries.push_back(std::make_pair(key1, data1));
  StrikeData data2;
  data2.set_num_strikes(5);
  entries.push_back(std::make_pair(key2, data2));
  AddProtoEntries(entries);

  int strikes = GetProtoStrikes(key1);
  EXPECT_EQ(3, strikes);
  strikes = GetProtoStrikes(key2);
  EXPECT_EQ(5, strikes);
  ClearAllProtoStrikesForKey(key1);
  strikes = GetProtoStrikes(key1);
  EXPECT_EQ(0, strikes);
  strikes = GetProtoStrikes(key2);
  EXPECT_EQ(5, strikes);
}

TEST_F(StrikeDatabaseTest, ClearAllProtoStrikesTest) {
  // Set up database with 3 pre-existing strikes at |key1|, and 5 pre-existing
  // strikes at |key2|.
  const std::string key1 = "12345";
  const std::string key2 = "13579";
  std::vector<std::pair<std::string, StrikeData>> entries;
  StrikeData data1;
  data1.set_num_strikes(3);
  entries.push_back(std::make_pair(key1, data1));
  StrikeData data2;
  data2.set_num_strikes(5);
  entries.push_back(std::make_pair(key2, data2));
  AddProtoEntries(entries);

  EXPECT_EQ(3, GetProtoStrikes(key1));
  EXPECT_EQ(5, GetProtoStrikes(key2));
  ClearAllProtoStrikes();
  EXPECT_EQ(0, GetProtoStrikes(key1));
  EXPECT_EQ(0, GetProtoStrikes(key2));
}

}  // namespace autofill
