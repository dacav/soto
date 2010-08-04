/** @file doc.h Documentation header file */

/** @mainpage Soto - Project for RTOS.

  The SOTO project is focused on a correct, well designed and well
  documented real-time application which reads data from the (in)famous
  Alsa asoundlib.  This manual provides both a reference from the user
  prospective and a technical reference describing how the system works
  internally.

@section CompileInstall Compilation and installation

     This software uses the GNU Autotools build system: in order to
     compile and install the application the standard installation
     method must be used:
@verbatim
./configure
make
make install
@endverbatim

     More information is provided by the INSTALL file, which is
     included in the software package.

@section License

     Soto is free software: you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     A copy of the GNU General Public License is provided along with
     Soto: see the COPYING file.

@section Dependency 

     In order to compile and run Soto you must satisfy the following
     dependencies:

     \arg Alsa Soundlib (v. 1.0.20-3);
     \arg GNU Plotutils libplot (v. 2.5-4);
     \arg LibDacav (v. 0.4.2);

*/

/** @defgroup AlsaGw Alsa Interface

  This module hides Alsa's weird bogus under a hood, providing a simple
  initialization / finalization / reading interface.

  The samp_new() function allocates a new sampler and allows to provide
  some parameters for the underlying library (namely Alsa ASoundLib.

  In order to read data from Alsa, the samp_read() function can be
  invoked. Since the module initializes Alsa in a non-locking way, in
  principle this function will return immediately. Sometimes however the
  data may be not available. In those cases the user can specify a maximum
  amount of time to spend waiting for the resource to be available. Note
  that this feature is provided directly by Alsa through the
  snd_pcm_wait() function: this is probably implemented with some
  asynchronous calls.

  Other functions provided by this module allow to obtain additional
  information which can be used to correctly tune other modules.

  The samp_destroy() function can be eventually used to release
  resources.

*/

/** @defgroup Thrd Soft Real Time Threads.

  This module provides a soft real time pool with periodic threads managed
  with a Rate Monotonic priority assignment.

@section Thrd_Initialization Initialization

    A new thread pool can be obtained by calling thrd_new(). After
    this phase the programmer can add an arbitrary number of threads by
    using the thred_add() function.

    A thread behavior is specified by providing a thrd_info_t structure,
    which must contain:

    @arg A callback procedure;
    @arg A user context for the callback procedure;
    @arg A time specification (startup delay + period) for the task;
    @arg Two optional callbacks which are respectively called before and
         after the periodic execution.

@section Thrd_Startup Startup

    When all threads have been subscribed, the brave programmer can startup
    the pool by calling the thrd_start() function, which shall achieve
    the priority assignment (according with the periods provided for the
    threads) and, subsequently, enable all threads.

    Before enabling the threads, the pool records the current absolute
    time against the monotonic system clock ("CLOCK_MONOTONIC"): the
    startup delay of all threads is re

@section Thrd_Callbacks Callbacks semantics

    The callbacks are supposed to be the only controlling method with
    respect to the threading pool.

    They are executed by the thread which they are associated with.

    @arg The initialization callback (henceforth "Start") gets executed
         before waiting for the startup delay;
    @arg The business logic callback (henceforth "Business") is called
         once a period;
    @arg The finalization callback (henceforth "Finish") gets executed
         after the thread has been required to terminate.

    While the latter works just as a simple cleanup procedure for the user
    context, "Start" and "Business" are characterized by a stricter
    semantics.

    In the first place they can, at any time, require the thread to be
    aborted by simply returning a non-zero value.
    
    In second place they should respect some obvious time constraints:
    
    @arg The worst case execution time of "Start" should be lesser than
         the thread startup delay, thus this callback should run only the
         initialization code which is strictly required to be executed by
         the thread.
        
    @arg The worst case execution time of "Business" should be lesser than
         the period.

    Since activations times are beat by calls to the clock_nanosleep()
    system call, a defiant behavior with respect to these constraints
    shall result in a null waiting time. For a bad designed task set this
    entails the execution to be jeopardized by domino effect.

@section Thrd_Limitations Shutting down

    For the moment the Thread Pool module is not provided with a shutdown
    procedure: in order to correctly cleanup and free resources the
    programmer can use the thrd_destroy() function, which simply waits for
    all the running thread to be terminated and achieves the cleaning up.

    An elegant solution to this problem is provided by the
    \ref GenThrd module.

*/

/** @defgroup GenThrd Generic Threads

  The \ref Thrd module provides a nice structure which hides the
  mechanisms allowing a Real-time execution policy, however a drawback
  is soon becoming clear to the fearless programmer who uses it: the
  \ref Thrd_Callbacks "return value based shutdown mechanism" is weak.

  Initially my mind was gazing at a Posix-signal oriented mechanism, which
  would have provided a way to gently ask the thread to die. Unfortunately
  the manpages tells us that &ldquo;signal masks are set on a per-thread
  basis, but signal actions and signal handlers, [...], are shared between
  all threads&rdquo;. Besides, who need signal when there's the
  pthread_cancel() call does exactly what we need? We just need to deal
  with cancellation points. Unfortunately the developer is forced to do
  this in all modules!

  Once realized that this can be generalized, I had enough cases to make
  allowance for this module to be written.

         Other parts may be executed con the user context
         before subscribing the thread; + standard thread init operation,
         handle for kill
         --->

*/
