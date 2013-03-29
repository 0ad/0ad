//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2011. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SHM_NAMED_CONDITION_HPP
#define BOOST_INTERPROCESS_SHM_NAMED_CONDITION_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/static_assert.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/detail/managed_open_or_create_impl.hpp>
#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/sync/shm/named_creation_functor.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/permissions.hpp>
#if defined (BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#endif


//!\file
//!Describes process-shared variables interprocess_condition class

namespace boost {
namespace interprocess {
namespace ipcdetail {

/// @cond
class interprocess_tester;
/// @endcond

//! A global condition variable that can be created by name.
//! This condition variable is designed to work with named_mutex and
//! can't be placed in shared memory or memory mapped files.
class shm_named_condition
{
   /// @cond
   //Non-copyable
   shm_named_condition();
   shm_named_condition(const shm_named_condition &);
   shm_named_condition &operator=(const shm_named_condition &);
   /// @endcond
   public:
   //!Creates a global condition with a name.
   //!If the condition can't be created throws interprocess_exception
   shm_named_condition(create_only_t create_only, const char *name, const permissions &perm = permissions());

   //!Opens or creates a global condition with a name.
   //!If the condition is created, this call is equivalent to
   //!shm_named_condition(create_only_t, ... )
   //!If the condition is already created, this call is equivalent
   //!shm_named_condition(open_only_t, ... )
   //!Does not throw
   shm_named_condition(open_or_create_t open_or_create, const char *name, const permissions &perm = permissions());

   //!Opens a global condition with a name if that condition is previously
   //!created. If it is not previously created this function throws
   //!interprocess_exception.
   shm_named_condition(open_only_t open_only, const char *name);

   //!Destroys *this and indicates that the calling process is finished using
   //!the resource. The destructor function will deallocate
   //!any system resources allocated by the system for use by this process for
   //!this resource. The resource can still be opened again calling
   //!the open constructor overload. To erase the resource from the system
   //!use remove().
   ~shm_named_condition();

   //!If there is a thread waiting on *this, change that
   //!thread's state to ready. Otherwise there is no effect.*/
   void notify_one();

   //!Change the state of all threads waiting on *this to ready.
   //!If there are no waiting threads, notify_all() has no effect.
   void notify_all();

   //!Releases the lock on the named_mutex object associated with lock, blocks
   //!the current thread of execution until readied by a call to
   //!this->notify_one() or this->notify_all(), and then reacquires the lock.
   template <typename L>
   void wait(L& lock);

   //!The same as:
   //!while (!pred()) wait(lock)
   template <typename L, typename Pr>
   void wait(L& lock, Pr pred);

   //!Releases the lock on the named_mutex object associated with lock, blocks
   //!the current thread of execution until readied by a call to
   //!this->notify_one() or this->notify_all(), or until time abs_time is reached,
   //!and then reacquires the lock.
   //!Returns: false if time abs_time is reached, otherwise true.
   template <typename L>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time);

   //!The same as:   while (!pred()) {
   //!                  if (!timed_wait(lock, abs_time)) return pred();
   //!               } return true;
   template <typename L, typename Pr>
   bool timed_wait(L& lock, const boost::posix_time::ptime &abs_time, Pr pred);

   //!Erases a named condition from the system.
   //!Returns false on error. Never throws.
   static bool remove(const char *name);

   /// @cond
   private:

   struct condition_holder
   {
      interprocess_condition cond_;
      //If named_mutex is implemented using semaphores
      //we need to store an additional mutex
      #if defined (BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
      interprocess_mutex mutex_;
      #endif
   };

   interprocess_condition *condition() const
   {  return &static_cast<condition_holder*>(m_shmem.get_user_address())->cond_; }

   template <class Lock>
   class lock_inverter
   {
      Lock &l_;
      public:
      lock_inverter(Lock &l)
         :  l_(l)
      {}
      void lock()    {   l_.unlock();   }
      void unlock()  {   l_.lock();     }
   };

   //If named mutex uses POSIX semaphores, then the shm based condition variable
   //must use it's internal lock to wait, as sem_t does not store a pthread_mutex_t
   //instance needed by pthread_mutex_cond_t
   #if defined (BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
      interprocess_mutex *mutex() const
      {  return &static_cast<condition_holder*>(m_shmem.get_user_address())->mutex_; }

      template <class Lock>
      void do_wait(Lock& lock)
      {
         //shm_named_condition only works with named_mutex
         BOOST_STATIC_ASSERT((is_convertible<typename Lock::mutex_type&, named_mutex&>::value == true));
        
         //lock internal before unlocking external to avoid race with a notifier
         scoped_lock<interprocess_mutex>     internal_lock(*this->mutex());
         lock_inverter<Lock> inverted_lock(lock);
         scoped_lock<lock_inverter<Lock> >   external_unlock(inverted_lock);

         //unlock internal first to avoid deadlock with near simultaneous waits
         scoped_lock<interprocess_mutex>     internal_unlock;
         internal_lock.swap(internal_unlock);
         this->condition()->wait(internal_unlock);
      }

      template <class Lock>
      bool do_timed_wait(Lock& lock, const boost::posix_time::ptime &abs_time)
      {
         //shm_named_condition only works with named_mutex
         BOOST_STATIC_ASSERT((is_convertible<typename Lock::mutex_type&, named_mutex&>::value == true));
         //lock internal before unlocking external to avoid race with a notifier 
         scoped_lock<interprocess_mutex>     internal_lock(*this->mutex(), abs_time); 
         if(!internal_lock) return false;
         lock_inverter<Lock> inverted_lock(lock); 
         scoped_lock<lock_inverter<Lock> >   external_unlock(inverted_lock); 

         //unlock internal first to avoid deadlock with near simultaneous waits 
         scoped_lock<interprocess_mutex>     internal_unlock; 
         internal_lock.swap(internal_unlock); 
         return this->condition()->timed_wait(internal_unlock, abs_time); 
      }
   #else //defined (BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)
      template<class Lock>
      class lock_wrapper
      {
         typedef void (lock_wrapper::*unspecified_bool_type)();
         public:

         typedef interprocess_mutex mutex_type;

         lock_wrapper(Lock &l)
            : l_(l)
         {}

         mutex_type* mutex() const
         {  return l_.mutex()->mutex();  }

         void lock()    { l_.lock(); }

         void unlock()  { l_.unlock(); }

         operator unspecified_bool_type() const
         {  return l_ ? &lock_wrapper::lock : 0;  }

         private:
         Lock &l_;
      };
   #endif   //defined (BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)

   friend class boost::interprocess::ipcdetail::interprocess_tester;
   void dont_close_on_destruction();

   managed_open_or_create_impl<shared_memory_object> m_shmem;

   template <class T, class Arg> friend class boost::interprocess::ipcdetail::named_creation_functor;
   typedef boost::interprocess::ipcdetail::named_creation_functor<condition_holder> construct_func_t;
   /// @endcond
};

/// @cond

inline shm_named_condition::~shm_named_condition()
{}

inline shm_named_condition::shm_named_condition(create_only_t, const char *name, const permissions &perm)
   :  m_shmem  (create_only
               ,name
               ,sizeof(condition_holder) +
                  managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(DoCreate)
               ,perm)
{}

inline shm_named_condition::shm_named_condition(open_or_create_t, const char *name, const permissions &perm)
   :  m_shmem  (open_or_create
               ,name
               ,sizeof(condition_holder) +
                  managed_open_or_create_impl<shared_memory_object>::
                     ManagedOpenOrCreateUserOffset
               ,read_write
               ,0
               ,construct_func_t(DoOpenOrCreate)
               ,perm)
{}

inline shm_named_condition::shm_named_condition(open_only_t, const char *name)
   :  m_shmem  (open_only
               ,name
               ,read_write
               ,0
               ,construct_func_t(DoOpen))
{}

inline void shm_named_condition::dont_close_on_destruction()
{  interprocess_tester::dont_close_on_destruction(m_shmem);  }

#if defined(BOOST_INTERPROCESS_NAMED_MUTEX_USES_POSIX_SEMAPHORES)

inline void shm_named_condition::notify_one()
{
   scoped_lock<interprocess_mutex> internal_lock(*this->mutex());
   this->condition()->notify_one();
}

inline void shm_named_condition::notify_all()
{
   scoped_lock<interprocess_mutex> internal_lock(*this->mutex());
   this->condition()->notify_all();
}

template <typename L>
inline void shm_named_condition::wait(L& lock)
{
   if (!lock)
      throw lock_exception();
   this->do_wait(lock);
}

template <typename L, typename Pr>
inline void shm_named_condition::wait(L& lock, Pr pred)
{
   if (!lock)
      throw lock_exception();
   while (!pred())
      this->do_wait(lock);
}

template <typename L>
inline bool shm_named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock);
      return true;
   }
   if (!lock)
      throw lock_exception();
   return this->do_timed_wait(lock, abs_time);
}

template <typename L, typename Pr>
inline bool shm_named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
{
   if(abs_time == boost::posix_time::pos_infin){
      this->wait(lock, pred);
      return true;
   }
   if (!lock)
      throw lock_exception();

   while (!pred()){
      if(!this->do_timed_wait(lock, abs_time)){
         return pred();
      }
   }
   return true;
}

#else

inline void shm_named_condition::notify_one()
{  this->condition()->notify_one();  }

inline void shm_named_condition::notify_all()
{  this->condition()->notify_all();  }

template <typename L>
inline void shm_named_condition::wait(L& lock)
{
   lock_wrapper<L> newlock(lock);
   this->condition()->wait(newlock);
}

template <typename L, typename Pr>
inline void shm_named_condition::wait(L& lock, Pr pred)
{
   lock_wrapper<L> newlock(lock);
   this->condition()->wait(newlock, pred);
}

template <typename L>
inline bool shm_named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time)
{
   lock_wrapper<L> newlock(lock);
   return this->condition()->timed_wait(newlock, abs_time);
}

template <typename L, typename Pr>
inline bool shm_named_condition::timed_wait
   (L& lock, const boost::posix_time::ptime &abs_time, Pr pred)
{
   lock_wrapper<L> newlock(lock);
   return this->condition()->timed_wait(newlock, abs_time, pred);
}

#endif

inline bool shm_named_condition::remove(const char *name)
{  return shared_memory_object::remove(name); }

/// @endcond

}  //namespace ipcdetail
}  //namespace interprocess
}  //namespace boost

#include <boost/interprocess/detail/config_end.hpp>

#endif // BOOST_INTERPROCESS_SHM_NAMED_CONDITION_HPP
