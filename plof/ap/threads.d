/**
 * Underlying threadpools for AP threads.
 *
 * License:
 *  Copyright (c) 2008  Gregor Richards
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

module plof.ap.threads;

import tango.core.Thread;
import tango.core.sync.Mutex;

// we can uses sysconf to get the ideal number of threads on everything but Windows
version (Windows) {} else {
    import tango.stdc.posix.unistd;
}

import plof.ap.apobject;
import plof.ap.interp;

/// An individual AP thread
class APThread : Thread {
    this(APThreadPool parent) {
        _queueLock = new Mutex();

        _parent = parent;
        _parent.addThread(this);

        super(&run);
    }

    /// Thread main method
    void run() {
        while (true) {
            // get something off of our queue
            _queueLock.lock();
            if (_queue.length > 0) {
                _action = _queue[$-1];
                _queue.length = _queue.length - 1;
            }
            _queueLock.unlock();

            /* if we had nothing to do, ask somebody else
            _parent.foreachThread(this, (APThread other) {
                other._queueLock.lock();
                if (other._queue.length > 0) {
                    _action = other._queue[$-1];
                    other._queue.length = other._queue.length - 1;
                }
                other._queueLock.unlock();

                // if we found something, we're done
                if (_action !is null)
                    return true;
                else
                    return false;
            }); */

            // if todo is still null, maybe everything's done
            if (_action is null) {
                bool done = true;

                // check other threads
                _parent.foreachThread(this, (APThread other) {
                    other._queueLock.lock();
                    if (other._queue.length != 0 || other._action !is null) {
                        // somebody still has work
                        done = false;
                    }
                    other._queueLock.unlock();

                    return (done == false);
                });

                // if we're done, stop
                if (done) return;

                // otherwise, yield for more work
                yield();

            } else {
                // mark it as running
                synchronized (_action) {
                    _action.state = ActionState.Running;
                }

                // OK, do it
                try {
                    _action.ast.accept(new APInterpVisitor(_action));
                } catch (APInterpFailure ex) {
                    // failed to interpret, need to re-enqueue
                    _action.cancel();
                }

                // now see if we need to re-queue it
                synchronized (_action) {
                    if (_action.state == ActionState.Canceled) {
                        _parent.enqueue([_action]);
                    } else {
                        _action.state = ActionState.Done;
                    }
                }

                _action = null;

            }
        }
    }

    /// The threadpool that owns this
    private APThreadPool _parent;

    /// Queue and lock
    private Action[] _queue;
    /// ditto
    private Mutex _queueLock;

    /// Current action being run (null if none)
    private Action _action;

    /// The thread number, only set by APThreadPool
    private uint _tnum;
}

/// A pool of threads
class APThreadPool {
    /// Start the whole pool
    void start() {
        _running = true;
        foreach (thread; _threads) {
            thread.start();
        }
    }

    /// Add a thread to the pool
    void addThread(APThread toadd) {
        if (!_running) {
            toadd._tnum = _threads.length;
            _threads ~= toadd;
        }
    }

    /// Foreach over all the threads, starting with the given one (or null to start at 0)
    void foreachThread(APThread from, bool delegate(APThread) each) {
        uint start = 0;
        if (from !is null)
            start = from._tnum;

        // now just loop around them
        for (uint i = (start + 1) % _threads.length;
             i != start;
             i = (i + 1) % _threads.length) {
            if (each(_threads[i])) break;
        }
    }

    /// Enqueue new work (note: every piece of work must either by currently unshared or locked)
    void enqueue(Action[] work) {
        uint t;
        synchronized (this) {
            // choose a thread to start adding to
            t = (toEnqueue++) % _threads.length;
        }

        // try to divide the work evenly
        int wi = 0;
        int wperthread = work.length / _threads.length + 1;

        // now start assigning
        for (; wi < work.length; t = (t + 1) % _threads.length) {
            APThread at = _threads[t];

            // try to give it some work
            if (at._queueLock.tryLock()) {
                // enqueue in reverse order (the inner queues are really a stack
                int i = wi + wperthread - 1;
                if (i >= work.length) i = work.length - 1;

                for (; i >= wi; i--) {
                    work[i].state = ActionState.Queued;
                    at._queue ~= work[i];
                }

                at._queueLock.unlock();

                // now make sure we keep track of the work that's been queued
                wi += wperthread;
            }
        }
    }

    /// Create a new threadpool of a seemingly-logical size
    static APThreadPool create(uint count = 0) {
        uint hardwareThreadCount;

        version (Windows) {
            // no clue!
            hardwareThreadCount = 4;

        } else version (linux) {
            // get the number from sysconf
            hardwareThreadCount = sysconf(_SC_NPROCESSORS_ONLN);

        } else version (darwin) {
            // sysconf, but Tango doesn't have the #define :(
            hardwareThreadCount = sysconf(58);

        } else {
            hardwareThreadCount = 4;

        }

        // now make about 1.5*hardwareThreadCount-many threads
        APThreadPool ret = new APThreadPool();
        uint threadCount = hardwareThreadCount + (hardwareThreadCount>>1);

        // unless specified
        if (count != 0)
            threadCount = count;

        for (uint i = 0; i < threadCount; i++) {
            new APThread(ret);
        }

        return ret;
    }

    private APThread[] _threads;
    private bool _running = false;
    private uint toEnqueue = 0;
}
