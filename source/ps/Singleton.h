//////////////////////////////////////////////////////////////////////////////
//    AUTHOR: Michael Reiland
//  FILENAME: Singleton.h
//   PURPOSE: Provides a base template class for Singletons
//
//    USEAGE: class myClass : Singleton<myClass>{};
//
//      INFO: This implementation was copied from: 
//
//                           Enginuity, Part II 
//            Memory Management, Error Logging, and Utility Classes; 
//                 or, How To Forget To Explode Your Underwear 
//                        by Richard "superpig" Fine
//
//            hosted at Gamedev.net at
//            http://gamedev.net/reference/articles/article1954.asp
//
//  MODIFIED: 07.09.2003 mreiland
#ifndef _TEMPLATE_SINGLETON
#define _TEMPLATE_SINGLETON

#include <assert.h>


template<typename T>
class Singleton
{
   static T* ms_singleton;

   public:
      Singleton()
      {
         assert( !ms_singleton );

         //use a cunning trick to get the singleton pointing to the start of
         //the whole, rather than the start of the Singleton part of the object
         int offset = (int)(T*)1 - (int)(Singleton<T>*)(T*)1;
         ms_singleton = (T*)((int)this + offset);
      }

      ~Singleton()
      {
         assert( ms_singleton );
         ms_singleton=0;
      }

      static T& GetSingleton()
      {
         assert( ms_singleton );
         return *ms_singleton;
      }

      static T* GetSingletonPtr()
      {
         assert( ms_singleton );
         return ms_singleton;
      }
};

template <typename T>
T* Singleton<T>::ms_singleton = 0;

#endif