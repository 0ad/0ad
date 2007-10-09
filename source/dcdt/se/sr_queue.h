
# ifndef SR_QUEUE_H
# define SR_QUEUE_H

/** \file sr_queue.h 
 * Template for a queue based on SrArray. */

# include <SR/sr_array.h> 

/*! \class SrQueue sr_queue.h
    \brief Queue based on two SrArray 

    SrQueue implements a FIFO queue using two internal SrArray objects,
    one for elements being inserted, and the other for elements to output.
    Both SrArray work as stacks. Note that SrQueue is oriented for speed,
    and does not honors constructors or destructors of its elements X, see
    SrArray documentation for additional comments on this. To overcome
    this limitation, use SrQueue with pointers. */
template <class X>
class SrQueue
 { private :
    SrArray<X> _in;
    SrArray<X> _out;
   public :

    /*! Constructor receives the capacity for both input and output arrays. */
    SrQueue ( int c=0 ) : _in(0,c), _out(0,c) {}

    /*! Copy constructor. */
    SrQueue ( const SrQueue& q ) : _in(q._in), _out(q._out) {}

    /*! Returns true if the queue is empty, false otherwise. */
    bool empty () const { return _in.empty() && _out.empty()? true:false; }

    /*! Returns the number of elements in the queue. */
    int size () const { return _in.size()+_out.size(); }

    /*! Compress both input and output arrays. */
    void compress () { _in.compress(); _out.compress(); }

    /*! Returns a reference to the first element; the queue must not be empty. */
    X& first () { return _out.empty()? _in[0]:_out.top(); }

    /*! Returns a reference to the last element; the queue must not be empty. */
    X& last () { return _in.empty()? _out[0]:_in.top(); }

    /*! Inserts an element at the end of the queue. */
    X& insert () { return _in.push(); }

    /*! Removes and returns a reference to the first element of the queue;
        the queue must not be empty. */
    X& remove ()
     { if ( _out.empty() )
        { _out.size(_in.size()-1);
          for ( int i=0; i<_out.size(); i++ ) _out[i]=_in.pop();
          return _in.pop();
        }
       else return _out.pop();
     }

    /*! Empties the input stack, correctly moving its elements to the output stack. */
    void flush ()
     { if ( _in.empty() ) return;
       _out.insert ( 0, _in.size() );
       for ( int i=0; _in.size()>0; i++ ) _out[i]=_in.pop();
     }

    /*! Makes this queue be the same as q, which becomes an empty queue. */
    void take_data ( SrQueue<X>& q )
     { _in.take_data ( q._in );
       _out.take_data ( q._out );
     }

    /*! Outputs all elements of the queue in format [e0 e1 ... en]. */
    friend SrOutput& operator<< ( SrOutput& o, const SrQueue<X>& q )
     {
       int i, si, so;
       si = q._in.size();
       so = q._out.size();

       o << '[';
       for ( i=0; i<si; i++ )
        { o << q._in[i];
          if ( i<si-1 || so>0 ) o << srspc;
        }
       for ( i=so-1; i>=0; i-- )
        { o << q._out[i];
          if ( i>0 ) o << srspc;
        }
       return o << ']';
     }
 };

//============================== end of file ===============================

#endif // SR_QUEUE_H

