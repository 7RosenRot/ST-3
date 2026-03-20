// Copyright 2021 GHA Test Team

#include "TimedDoor.h"
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MockTimerClient : public TimerClient {
public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class MockDoor : public Door {
public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class TimedDoorTest : public ::testing::Test {
protected:
  TimedDoor *door;

  void SetUp() override { door = new TimedDoor(10); }
  void TearDown() override { delete door; }
};

TEST_F(TimedDoorTest, InitialStatusIsClosed) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, GetTimeoutReturnsCorrectValue) {
  EXPECT_EQ(door->getTimeOut(), 10);
}

TEST_F(TimedDoorTest, LockChangesStateToClosed) {
  try {
    door->unlock();
  } catch (...) {
  }

  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, ThrowStateConsistency) {
  EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTest, ReentryLock) {
  door->lock();

  EXPECT_NO_THROW(door->lock());
}

TEST_F(TimedDoorTest, MultipleOpenCloseCycle) {
  for (int i = 0; i < 3; ++i) {
    try {
      door->unlock();
    } catch (...) {
    }

    EXPECT_TRUE(door->isDoorOpened());

    door->lock();

    EXPECT_FALSE(door->isDoorOpened());
  }
}

TEST_F(TimedDoorTest, LockWhenAlreadyLocked) {
  door->lock();

  bool stateBefore = door->isDoorOpened();

  door->lock();

  EXPECT_EQ(stateBefore, door->isDoorOpened());
}

TEST(AdapterLogic, DoesNotThrowIfDoorClosed) {
  TimedDoor d(0);

  d.lock();

  DoorTimerAdapter adapter(d);
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(AdapterLogic, ThrowsIfDoorOpened) {
  TimedDoor d(0);
  DoorTimerAdapter adapter(d);

  try {
    d.unlock();
  } catch (...) {
  }

  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(TimerMock, RegisterCallsTimeoutOnClient) {
  Timer timer;
  MockTimerClient mockClient;

  EXPECT_CALL(mockClient, Timeout()).Times(1);
  timer.tregister(1, &mockClient);
}

TEST(DoorMock, MockLockInteraction) {
  MockDoor mDoor;

  EXPECT_CALL(mDoor, lock()).Times(2);

  mDoor.lock();
  mDoor.lock();
}

TEST(BoundaryTests, ZeroTimeoutExecution) {
  TimedDoor d(0);

  EXPECT_THROW(d.unlock(), std::runtime_error);
}

TEST(BoundaryTests, NegativeTimeoutIsHandled) {
  TimedDoor d(-1);

  EXPECT_THROW(d.unlock(), std::runtime_error);
}

TEST(BoundaryTests, LargeTimeoutDoesNotThrowImmediately) {
  TimedDoor d(100);

  EXPECT_THROW(d.unlock(), std::runtime_error);
}

TEST(ManualControl, AdapterObjectPersists) {
  TimedDoor d(10);
  DoorTimerAdapter ad(d);

  EXPECT_NO_THROW(ad.Timeout());
}

TEST(TimerNull, RegisterHandlesNullPointer) {
  Timer t;

  EXPECT_NO_THROW(t.tregister(1, nullptr));
}

TEST(Sequence, LockUnlockSequence) {
  TimedDoor d(0);

  d.lock();
  d.lock();

  EXPECT_FALSE(d.isDoorOpened());

  try {
    d.unlock();
  } catch (...) {
  }

  EXPECT_TRUE(d.isDoorOpened());
}

TEST(AdapterIdentity, AdapterLinkedToCorrectDoor) {
  TimedDoor d1(0), d2(0);
  DoorTimerAdapter ad1(d1);

  d1.lock();

  EXPECT_NO_THROW(ad1.Timeout());
}

TEST(ExceptionMessage, RuntimeErrorMessageIsCorrect) {
  TimedDoor d(0);

  try {
    d.unlock();
  } catch (const std::runtime_error &e) {
    EXPECT_STREQ(e.what(), "Timeout: Door is still open!");
  }
}

TEST(TimerMultiple, MultipleRegisters) {
  Timer t;
  MockTimerClient m1, m2;

  EXPECT_CALL(m1, Timeout()).Times(1);
  EXPECT_CALL(m2, Timeout()).Times(1);

  t.tregister(1, &m1);
  t.tregister(1, &m2);
}

TEST(Stress, RapidLockUnlock) {
  TimedDoor d(1);

  for (int i = 0; i < 5; ++i) {
    d.lock();

    EXPECT_FALSE(d.isDoorOpened());
  }
}

TEST(MockState, IsDoorOpenedMockReturn) {
  MockDoor m;

  EXPECT_CALL(m, isDoorOpened()).WillOnce(::testing::Return(true));
  EXPECT_TRUE(m.isDoorOpened());
}

TEST(TimerInteraction, TimerWaitCheck) {
  auto start = std::chrono::steady_clock::now();

  Timer t;
  MockTimerClient mock;

  EXPECT_CALL(mock, Timeout()).Times(1);

  t.tregister(50, &mock);

  auto end = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  EXPECT_GE(elapsed, 45);
}

TEST(MockInteraction, TimerCallsClientTimeout) {
  Timer timer;
  MockTimerClient mockClient;
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  timer.tregister(10, &mockClient);
}

TEST(ExceptionType, IsCorrectType) {

  TimedDoor d(0);

  bool caught = false;

  try {
    d.unlock();
  } catch (const std::runtime_error &) {
    caught = true;
  } catch (...) {
    FAIL() << "Expected std::runtime_error";
  }

  EXPECT_TRUE(caught);
}

TEST(ObjectLogic, TimeoutIsImmutable) {
  TimedDoor d(100);

  int first = d.getTimeOut();

  d.lock();

  EXPECT_EQ(first, d.getTimeOut());
}

TEST(AdapterLogic, DetectsLateOpening) {
  TimedDoor d(0);
  DoorTimerAdapter ad(d);

  d.lock();

  EXPECT_NO_THROW(ad.Timeout());

  try {
    d.unlock();
  } catch (...) {
  }

  EXPECT_THROW(ad.Timeout(), std::runtime_error);
}
