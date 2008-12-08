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

import tango.io.Stdout;

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

            // if we had nothing to do, ask somebody else
            if (_action is null) {
                _parent.foreachThread(this, (APThread other) {
                    if (other._queueLock.tryLock()) {
                        if (other._queue.length > 1) {
                            // try to steal half of his work
                            if (_queueLock.tryLock()) {
                                // OK, both queues are locked, so steal
                                uint tosteal = other._queue.length / 2;
                                _queue ~= other._queue[tosteal..$];
                                other._queue.length = tosteal;
                                _action = _queue[$-1];
                                _queue.length = _queue.length - 1;

                                _queueLock.unlock();

                            } else {
                                // just steal one
                                _action = other._queue[$-1];
                                other._queue.length = other._queue.length - 1;

                            }

                        }
                        other._queueLock.unlock();
                    }

                    // if we found something, we're done
                    return (_action !is null);
                });
            }

            // if todo is still null, maybe everything's done
            if (_action is null) {
                bool done = true;

                // check other threads
                _parent.foreachThread(this, (APThread other) {
                    if (other._queueLock.tryLock()) {
                        if (other._queue.length != 0 || other._action !is null) {
                            // somebody still has work
                            done = false;
                        }
                        other._queueLock.unlock();
                    } else {
                        done = false;
                    }

                    return (done == false);
                });

                // if we're done, stop
                if (done) {
                    debugOut("quitting.");

                    return;
                }

                // otherwise, yield for more work
                yield();

            } else {
                debugOut("doing " ~ _action.ast.toXML());

                // mark it as running
                synchronized (_action) {
                    _action.queued = false;
                    switch (_action.state) {
                        case ActionState.None:
                        case ActionState.Canceled:
                            _action.state = ActionState.Running;
                            break;

                        case ActionState.Destroyed:
                            // can't run this!
                            _action = null;
                            break;

                        case ActionState.Done:
                            // commit this
                            _action.state = ActionState.Committing;
                            break;

                        default:
                            synchronized (Stderr)
                                Stderr("Action ")(_action.ast.toXML())(" in bad state for running: ")(_action.state).newline;
                    }
                }

                if (_action is null) continue;

                // OK, do it
                try {
                    _action.run();

                } catch (APInterpFailure ex) {
                    // failed to interpret, need to re-enqueue
                    _action.cancel();

                } catch (APUnimplementedException ex) {
                    // just mention it
                    synchronized (Stderr) Stderr("Unimplemented AST node: ")(ex.msg).newline;
                }

                // now see if we need to re-queue it
                synchronized (_action) {
                    switch (_action.state) {
                        case ActionState.Running:
                            // notify the commit thread
                            _action.doneMutex.lock();
                            _action.state = ActionState.Done;
                            _action.doneCondition.notify();
                            _action.doneMutex.unlock();
                            break;

                        case ActionState.Canceled:
                            // re-enqueue it
                            _parent.enqueue([_action]);
                            break;

                        case ActionState.Destroyed:
                            // who cares
                            break;

                        case ActionState.Committing:
                            _action.state = ActionState.Committed; // should never change again
                            break;

                        default:
                            synchronized (Stderr)
                                Stderr("Action ")(_action.ast.toXML())(" in bad state for completion: ")(_action.state).newline;
                    }
                }

                debugOut("done.");

                _queueLock.lock();
                _action = null;
                _queueLock.unlock();

            }
        }
    }

    /// Thread debugging is only provided with the proper version()
    version (ThreadDebug) {
        final void debugOut(char[] msg) {
            synchronized (Stderr) Stderr("Thread ")(_tnum)(" ")(msg).newline;
        }
    } else {
        final void debugOut(char[] msg) {}
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
    this() {
        commitThread = new CommitThread(this);
    }

    /// Start the whole pool with the given action
    void start(Action initAction) {
        enqueue([initAction]);
        commitThread._initAction = initAction;
        _running = true;
        foreach (thread; _threads) {
            thread.start();
        }
        commitThread.start();
    }

    /// Join the whole pool
    void join() {
        foreach (thread; _threads) {
            thread.join();
        }
        // FIXME: kill the commit thread
        _running = false;
    }

    /// Add a thread to the pool
    void addThread(APThread toadd) {
        if (!_running) {
            toadd._tnum = _threads.length;
            _threads ~= toadd;
        }
    }

    /// Get the number of the current thread
    uint getThis() {
        APThread caller = cast(APThread) Thread.getThis();
        if (caller !is null) {
            return caller._tnum;
        } else {
            return 0; // BS, but hopefully irrelevant
        }
    }

    /// Get the total number of threads
    uint threadCount() {
        return _threads.length;
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

        // try to add it to the calling thread's queue
        APThread caller = cast(APThread) Thread.getThis();
        if (caller !is null) {
            t = caller._tnum;

        } else {
            // otherwise just use a global counter
            synchronized (this) {
                // choose a thread to start adding to
                t = (toEnqueue++) % _threads.length;
            }

        }

        // now assign to the appropriate thread if possible
        for (;; t = (t + 1) % _threads.length) {
            APThread at = _threads[t];

            // try to give it the work
            if (at._queueLock.tryLock()) {
                // enqueue in reverse order (the inner queues are really a stack)
                for (int i = work.length - 1; i >= 0; i--) {
                    work[i].queued = true;
                    at._queue ~= work[i];
                }
                at._queueLock.unlock();

                break;
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

    /// Heuristically determine whether we should inline functions
    bool shouldInline() {
        /* we don't need to lock the queues since we're just guessing here. The
         * only change this makes is what thread runs an action, not the result */
        foreach (thread; _threads) {
            if (thread._queue.length == 0)
                return false;
        }
        return true;
    }

    private {
        APThread[] _threads;
        bool _running = false;
        uint toEnqueue = 0;
        CommitThread commitThread;
    }
}

/// The commit thread
class CommitThread : Thread {
    this(APThreadPool tp) {
        _tp = tp;
        super(&run);
    }

    /// Find things to commit, and commit them
    void run() {
        eventuallyCommit(_initAction);
    }

    /// Commit the given action and its children
    void eventuallyCommit(Action act) {
        // wait for it to be done
        act.doneMutex.lock();
        while (act.state < ActionState.Done)
            act.doneCondition.wait();
        act.doneMutex.unlock();

        // commit it
        _tp.enqueue([act]);

        // only the normal run can create new children, so we're safe to use them without locking
        for (int ci = 0; ci < act.children.length; ci++) {
            Action child = act.children[ci];
            eventuallyCommit(child);
        }
    }

    private {
        APThreadPool _tp;
        Action _initAction; // set by ThreadPool
    }
}
