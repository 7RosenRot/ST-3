// Copyright 2021 GHA Test Team
#include "TimedDoor.h"
#include <thread>
#include <chrono>
#include <stdexcept>

void Timer::sleep(int timeout) {
  std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
}

void Timer::tregister(int timeout, TimerClient* client) {
  this->client = client;
  
  if (client) {
    sleep(timeout);

    client->Timeout();
  }
}

DoorTimerAdapter::DoorTimerAdapter(TimedDoor& d) : door(d) {}

void DoorTimerAdapter::Timeout() {
  if (door.isDoorOpened()) {
    door.throwState();
  }
}

TimedDoor::TimedDoor(int timeout) : iTimeout(timeout), isOpened(false) {
  adapter = new DoorTimerAdapter(*this);
}

bool TimedDoor::isDoorOpened() {
  return isOpened;
}

void TimedDoor::unlock() {
  isOpened = true;

  Timer timer;

  timer.tregister(iTimeout, adapter);
}

void TimedDoor::lock() {
  isOpened = false;
}

int TimedDoor::getTimeOut() const {
  return iTimeout;
}

void TimedDoor::throwState() {
  throw std::runtime_error("Timeout: Door is still open!");
}
