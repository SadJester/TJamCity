#include "stdafx.h"

#include <core/store_models/idata_model.h>

using namespace tjs::core;

class TestEntry : public IStoreEntry {
public:
	static std::type_index get_type() { return typeid(TestEntry); }

	void init() override {
		inited = true;
		++init_count;
	}
	void release() override {
		released = true;
		++release_count;
	}
	void reinit() override { ++reinit_count; }

	bool inited { false };
	bool released { false };
	int init_count { 0 };
	int release_count { 0 };
	int reinit_count { 0 };
};

class AnotherEntry : public IStoreEntry {
public:
	static std::type_index get_type() { return typeid(AnotherEntry); }

	void init() override {
		inited = true;
		++init_count;
	}
	void release() override {
		released = true;
		++release_count;
	}
	void reinit() override { ++reinit_count; }

	bool inited { false };
	bool released { false };
	int init_count { 0 };
	int release_count { 0 };
	int reinit_count { 0 };
};

class GenericStoreTest : public ::testing::Test {
protected:
	GenericStore<IStoreEntry> store;
};

TEST_F(GenericStoreTest, CreateAndGet) {
	auto& entry = store.create<TestEntry>();
	EXPECT_EQ(store.get_entry<TestEntry>(), &entry);
	EXPECT_EQ(store.entries().size(), 1u);
}

TEST_F(GenericStoreTest, DuplicateCreateReturnsExisting) {
	auto& first = store.create<TestEntry>();
	auto& second = store.create<TestEntry>();
	EXPECT_EQ(&first, &second);
	EXPECT_EQ(store.entries().size(), 1u);
}

TEST_F(GenericStoreTest, InitCallsExistingEntries) {
	auto& entry = store.create<TestEntry>();
	EXPECT_FALSE(entry.inited);
	store.init();
	EXPECT_TRUE(entry.inited);
	EXPECT_EQ(entry.init_count, 1);
}

TEST_F(GenericStoreTest, CreateAfterInitCallsInitImmediately) {
	store.init();
	auto& entry = store.create<AnotherEntry>();
	EXPECT_TRUE(entry.inited);
	EXPECT_EQ(entry.init_count, 1);
}

TEST_F(GenericStoreTest, ReleaseCallsEntriesAndDisablesInit) {
	auto& entry = store.create<TestEntry>();
	store.init();
	store.release();
	EXPECT_TRUE(entry.released);
	auto& another = store.create<AnotherEntry>();
	EXPECT_FALSE(another.inited);
}

TEST_F(GenericStoreTest, ReinitForwardsToEntries) {
	auto& entry = store.create<TestEntry>();
	store.reinit();
	EXPECT_EQ(entry.reinit_count, 1);
}
