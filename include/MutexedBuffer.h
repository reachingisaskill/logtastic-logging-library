
#ifndef LOGTASTIC_MUTEXED_BUFFER_H_
#define LOGTASTIC_MUTEXED_BUFFER_H_

#include <mutex>
#include <atomic>
#include <condition_variable>


/*
 * A singly-linked queue implementation where the entry and exit are mutexed.
 * This allows parallel pushes AND pops to happen.
 * List interrogation is not permitted. No iterators.
 */
template < class TYPE >
class MutexedBuffer
{
  private :
    // Define the element container struct
    struct Element
    {
      std::mutex mutex;
      TYPE data;
      Element* next;

      Element( TYPE d ) : data( d ), next( nullptr ) {}
    };

    // Control the start and end only
    std::mutex _mutexStart;
    std::mutex _mutexEnd;
    // Start and end markers
    Element* _start;
    Element* _end;

    // Count the number of elements
    size_t _elementCount;
    std::mutex _mutexCounter;

    // Condition variables to wait on
    std::condition_variable _waitData;
    std::condition_variable _waitEmpty;

    // Flag to cause the wait functions to break, so threads can be joined following an error.
    std::atomic<bool> _flag;

  public:
    MutexedBuffer();

    ~MutexedBuffer();

    // Push onto front of the buffer
    void push( TYPE );

    // Pop from back of the buffer
    bool pop( TYPE& );

    // Waits for something to pop - blocks the calling thread
    bool waitPop( TYPE& );

    // Clears all the data from the buffer using the calling thread.
    // Other threads may still push behind this operation
    void clear();

    // Returns the size stored in the internal counter
    size_t size();

    // Returns true if the size is zero
    bool empty();

    // Blocks the calling thread until data is entered. Then a single waiting thread is notified
    void waitForData();

    // Blocks the calling thread until size is zero. Then all waiting threads are notified
    void waitForEmpty();

    // Set the error flag and notify the condition variables. Restarts all waiting threads
    void setFlag( bool );

    // Return the value of the error flag
    bool flag() const { return _flag; }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Template member function definitions

template < class TYPE >
MutexedBuffer< TYPE >::MutexedBuffer() :
  _mutexStart(),
  _mutexEnd(),
  _start( nullptr ),
  _end( nullptr ),
  _elementCount( 0 ),
  _flag( false )
{
}


template < class TYPE >
MutexedBuffer< TYPE >::~MutexedBuffer()
{
  this->clear();
}


template < class TYPE >
void MutexedBuffer< TYPE >::clear()
{
  // Lock the other side of the buffer. No one can push
  std::unique_lock<std::mutex> startLock( _mutexStart );

  // Lock the counter
  std::unique_lock<std::mutex> counterLock( _mutexCounter );

  // Lock the end of the buffer - No one can pop
  std::unique_lock<std::mutex> endLock( _mutexEnd );

  // Remove the pointers to the buffer so that a new one might be started in the mean time
  MutexedBuffer::Element* current = _end;
  MutexedBuffer::Element* next = nullptr;
  _start = nullptr;
  _end = nullptr;

  // Set the size to zero
  _elementCount = 0;

  // Release the start
  endLock.unlock();

  // Release the counter
  counterLock.unlock();

  // Notify anyone waiting that the queue is now empty
  _waitEmpty.notify_all();

  // Release the end
  startLock.unlock();

  // Now we don't need to mutex anything. No one can have a reference to any of this data
  while ( current != nullptr )
  {
    // Cache the next element
    next = current->next;

    // Delete the current element
    delete current;
    current = next;
  }
}


template < class TYPE >
size_t MutexedBuffer< TYPE >::size()
{
  std::lock_guard<std::mutex> lock( _mutexCounter );
  return _elementCount;
}


template < class TYPE >
bool MutexedBuffer< TYPE >::empty()
{
  std::lock_guard<std::mutex> lock( _mutexCounter );
  return _elementCount == 0;
}


template < class TYPE >
void MutexedBuffer< TYPE >::push( TYPE val )
{
  // Create a new element
  MutexedBuffer::Element* element = new MutexedBuffer::Element( val );

  // Lock the start of the buffer
  std::unique_lock<std::mutex> startLock( _mutexStart );

  // Lock the counter to check the status
  std::unique_lock<std::mutex> counterLock( _mutexCounter );

  switch ( _elementCount )
  {
    case 0 :
      {
        std::unique_lock<std::mutex> endLock( _mutexEnd );
        _start = element;
        _end = element;
        ++_elementCount;
        endLock.unlock();
        counterLock.unlock();
      }
      break;

    case 1 : // If there's only one element - block the pop
      {
        std::unique_lock<std::mutex> endLock( _mutexEnd );
        ++_elementCount;
        counterLock.unlock();

        // Update the start/end pointer and release the end lock - no longer critical
        _start->next = element;
        endLock.unlock();

        // set the start element
        _start = element;
      }
      break;

    default :
      {
        ++_elementCount;
        // Update the start/end pointer and release the end lock - no longer critical
        _start->next = element;
        counterLock.unlock();

        // set the start element
        _start = element;
      }
      break;
  }

  // Notify anyone waiting that an element was pushed.
  _waitData.notify_one();

  startLock.unlock();
}


template < class TYPE >
bool MutexedBuffer< TYPE >::pop( TYPE& data )
{
  // Grab the counter
  std::unique_lock<std::mutex> counterLock( _mutexCounter );

  if ( _elementCount == 0 )
  {
    counterLock.unlock();
    return false;
  }
  else
  {
    // Aquire the end pointer - no one else can pop
    std::unique_lock<std::mutex> endLock( _mutexEnd );

    // Update and release the counter
    --_elementCount;

    if ( _elementCount == 0 )
    {
      counterLock.unlock();
      _waitEmpty.notify_all();
    }
    else
    {
      counterLock.unlock();
    }

    // Grab the element
    MutexedBuffer::Element* element = _end;

    // update the end pointer
    _end = _end->next;

    // Release the end pointer
    endLock.unlock();

    // Copy the data and remove the element
    data = element->data;
    delete element;
    return true;
  }
}



template < class TYPE >
bool MutexedBuffer< TYPE >::waitPop( TYPE& data )
{
  // Grab the counter
  std::unique_lock<std::mutex> counterLock( _mutexCounter );

  if ( _elementCount == 0 )
  {
    _waitData.wait( counterLock, [&]()->bool{ return (_elementCount > 0) || _flag; } );
  }

  // If there's an error return false
  if ( _flag )
  {
    counterLock.unlock();
    return false;
  }

  // Aquire the end pointer - no one else can pop
  std::unique_lock<std::mutex> endLock( _mutexEnd );

  // Update and release the counter
  --_elementCount;

  if ( _elementCount == 0 )
  {
    counterLock.unlock();
    _waitEmpty.notify_all();
  }
  else
  {
    counterLock.unlock();
  }

  // Grab the element
  MutexedBuffer::Element* element = _end;

  // update the end pointer
  _end = _end->next;

  // Release the end pointer
  endLock.unlock();

  // Copy the data and remove the element
  data = element->data;
  delete element;
  return true;

}


template < class TYPE >
void MutexedBuffer< TYPE >::waitForData()
{
  std::unique_lock<std::mutex> counterLock( _mutexCounter );

  _waitData.wait( counterLock, [&]()->bool{ return (_elementCount > 0) || _flag; } );

  counterLock.unlock();
}


template < class TYPE >
void MutexedBuffer< TYPE >::waitForEmpty()
{
  std::unique_lock<std::mutex> counterLock( _mutexCounter );

  _waitEmpty.wait( counterLock, [&]()->bool{ return (_elementCount == 0) || _flag; } );

  counterLock.unlock();
}


template < class TYPE >
void MutexedBuffer< TYPE >::setFlag( bool value )
{
  _flag = value;

  _waitEmpty.notify_all();
  _waitData.notify_all();
}

#endif // LOGTASTIC_MUTEXED_BUFFER_H_

