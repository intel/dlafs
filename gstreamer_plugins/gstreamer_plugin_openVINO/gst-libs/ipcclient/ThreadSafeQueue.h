#ifndef __THREADSAFEQUEUE_H
#define __THREADSAFEQUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <memory>

using namespace std;


template <typename T>
class ThreadSafeQueue
{
 private:
  mutable mutex _mLock;
  queue<shared_ptr<T> > _mQueue;
  condition_variable _mCond;
 public:
  ThreadSafeQueue()= default;
  ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;
  ThreadSafeQueue(const ThreadSafeQueue &other) = delete;
  void push(T& value)
  {
      shared_ptr<T> pData(make_shared<T>(move(value)));
      std::lock_guard<mutex> lock(_mLock);
      _mQueue.push(pData);
      _mCond.notify_one();
  }

  void waitAndPop(T& value)
  {
      unique_lock<mutex> lock(_mLock);
      _mCond.wait(lock, [this] { return !_mQueue.empty();});
      value = move(*_mQueue.front());
      _mQueue.pop();
  }

  shared_ptr<T> waitAndPop()
  {
      unique_lock<mutex> lock(_mLock);
      _mCond.wait(lock, [this] { return !_mQueue.empty();});
      std::shared_ptr<T> pRet = _mQueue.front();
      _mQueue.pop();
      return pRet;

  }

  bool tryPop(T& value) {
      lock_guard<mutex> lock(_mLock);
      if (_mQueue.empty())
          return false;
      value = std::move(*_mQueue.front());
      _mQueue.pop();
      return true;
  }


  shared_ptr<T> tryPop()
  {
      lock_guard<mutex> lock(_mLock);
      if(_mQueue.empty())
          return std::shared_ptr<T>();
      std::shared_ptr<T> pRet=_mQueue.front();
      _mQueue.pop();
      return pRet;
  }


  bool empty() const
  {
      lock_guard<mutex> lock(_mLock);
      return _mQueue.empty();
  }


};


#endif

