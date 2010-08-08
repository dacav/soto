/** @file doc.h Documentation header file 

@mainpage Soto - Project for RTOS.

    The SOTO project is focused on a correct, well designed and well
    documented real-time application which reads data from the microphone
    by using Alsa asoundlib.  This manual provides both a reference from
    the user prospective and a technical reference describing how the
    system works internally.

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
    included in the software package, however two special non-standard
    options can be provided to the configure script:

    @arg --enable-debug (which will give a more verbose output on stderr);
    @arg --disable-realtime (which will disable real-time programming).

@section License

    Soto is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    A copy of the GNU General Public License is provided along with
    Soto: see the COPYING file.

@section Dependency 

    In order to compile and run Soto you must satisfy the following
    dependencies:

    @arg Alsa Soundlib (v. 1.0.20-3);
    @arg GNU Plotutils libplot (v. 2.5-4);
    @arg LibDacav (v. 0.4.2);

@section AboutThis About this manual

    You may read this text on both the html reference and the report
    attached to the project: it's basically generated through doxygen from
    the same source.

    This manual explains how the application works. If you are reading the
    PDF version you won't find a reference manual: please refer to the
    html documentation if you need it.

    The application provide two general purpose modules that are heavvily
    used but not strictly related with the application's purpose:

    @arg @ref Thrd;
    @arg @ref GenThrd;

    The following topics will also be discussed for what concerns the
    business logic:

    @arg @ref BizAlsaGw;
    @arg @ref BizPlotting;
    @arg @ref BizPlotThread;
    @arg @ref BizSampling;
    @arg @ref BizSignal;
    @arg @ref BizSpectrum;
    @arg @ref BizOptions;

@section CLI Command Line Usage

    Command line options can be obtained directly from the executable by
    calling it with the -h flag

@verbatim
dacav@mithril:<src>$ ./soto  -h

soto 0.2.3
Usage: ./soto [options]

  --dev={dev} | -d {dev}
        Specify an audio device (default: "hw:0,0");

  --rate={rate} | -r {rate}
        Specify a sample rate for ALSA in Hertz (default: 44100);

  --show-spectrum[={bool}] | -U [{bool}]
        Show the spectrum of the audio stream (default: yes);

  --show-signal[={bool}] | -u [{bool}] 
        Show the signal of the audio stream (default: no);

  --buffer-scale={factor} | -s {factor}
        Provide the proportion between sampling buffer and read buffer
        (default: 10);

  --minprio={priority} | -m {priority}
        Specify the realtime priority for the thread having the longest
        sampling period (default 0, required a positive integer);

  --run-for={time in seconds} | -r {time in seconds}
        Requires the program to run for a certain amount of time.
        By providing 0 (which is the default) the program will run
        until interrupted

  --help  | -h
        Print this help.

@endverbatim

@defgroup Thrd Soft Real Time Threads

    This module provides a wrapper for the Posix Threads implementation
    (pthreads) which enables a soft real time pool with periodic threads.

    Priority assignment policy follows a Rate Monotonic design.

@section Thrd_Initialization Initialization

    A new thread pool can be obtained by calling thrd_new(). After
    this phase the programmer can add an arbitrary number of threads by
    using the thred_add() function.

    A thread behavior is specified through a pointer to a thrd_info_t
    structure, which must contain:

    @arg A callback procedure;
    @arg A user context for the callback procedure;
    @arg A time specification (startup delay + period) for the task;
    @arg Two optional callbacks which are respectively called before and
         after the periodic execution.

@section Thrd_Startup Startup

    When all threads have been subscribed, the brave programmer can start
    the pool by calling the thrd_start() function, which shall assignment
    the priority according with the periods provided for the threads.
    Subsequently the threads will be activated.

    Before enabling the threads, the pool records the current absolute
    time against the monotonic system clock (CLOCK_MONOTONIC): the
    startup delay of all threads is relative to the global activation
    instant.

@section Thrd_Callbacks Callbacks semantics

    As mentioned, each thread is associated with at least one callback
    procedure. Those callbacks are executed by the thread which they are
    associated with.

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
    
    In second place they should respect some real-time related
    constraints:
    
    @arg The worst case execution time of "Start" should be lesser than
         the thread startup delay, thus this callback should run only the
         initialization code which is strictly required to be executed by
         the thread (like, by example, obtaining the thread id by calling
         the pthread_self() function).
        
    @arg The worst case execution time of "Business" should be lesser than
         the period;

    Since activations times are beat by calls to the clock_nanosleep()
    system call, a defiant behavior with respect to these constraints
    shall result in a null waiting time.
    
    @note A bad designed task set may jeopardize, by domino effect, the
          whole operating system stability: if the medium case execution
          time is longer than the period the thread will run as an high
          priority infinite busy loop!

@section Thrd_Limitations Shutting down

    A Thread Pool object is not provided with a shutdown procedure: in
    order to correctly cleanup and free resources the programmer can use
    the thrd_destroy() function, which simply waits for all the running
    thread to be terminated and achieves the cleaning up.

    An elegant solution to this problem is provided by the
    @ref GenThrd module.

@defgroup GenThrd Generic Threads Interface

    The @ref Thrd module provides a nice structure which hides the
    mechanisms allowing a real-time execution policy, however a drawback
    is soon becoming clear to the fearless programmer who uses it: the
    @ref Thrd_Callbacks "return value based shutdown mechanism" can be
    managed only by the thread itself, while we may need a method to ask
    the thread termination from outside.

    Initially my mind was gazing at a Posix-signal oriented mechanism,
    which would have provided a way to gently ask the thread to die.
    Unfortunately the manpages tells us that &ldquo;signal masks are set
    on a per-thread basis, but signal actions and signal handlers [...]
    are shared between all threads&rdquo;. Besides, who need signal when
    there's the pthread_cancel() call does exactly what we need? We just
    need to deal with cancellation points. Unfortunately the developer is
    forced to do this in all modules! Once realized that this can be
    generalized, I had enough cases to make allowance for this code to be
    written.

    A Generic Threads object wraps this mechanism: the thread subscription
    requires as parameter a Threading Pool object and the same
    specification structure used by the @ref Thrd module (namely
    thrd_info_t). It subscribes a private initialization function
    and uses it to retrieve the thread identifier before calling the
    "Start" function provided by the user. The identifier is internally
    used to implement a termination procedure for the thread, and with
    other useful meta-data will be stored into an extension of the context
    provided by the user. Everything is associated to a handle.f

@section GenThrd_Termination Generic Thread Termination

    Through the handle of a Generic Thread, the developer can terminate
    the execution of a real-time task. Internally this is achieved by
    calling the pthread_destroy() system call.

    The module ensures a correct process termination by disabling the
    cancellation of the thread inside its private extension of the
    initialization function. Cancellation requests are explicitly checked
    once per period and the module takes care of executing the user's
    "Finish" procedure in case it's needed. This is achieved by using the
    pthread_cleanup_push() and pthread_cleanup_pop() functions.

@section GenThrd_Context Developing a Specialized Thread

    From the external developer prospective there's a simple pattern to
    specialize a Generic Thread:

    @arg Delegate creation and termination to the genth_subscribe() and
         genth_sendkill() functions respectively;

    @arg Provide further functionalities trough primitives accepting the
         header as parameter. The context provided with the thrd_info_t
         structure can be obtained back from the handle by calling the
         genth_get_context() function.

    @see The following modules use a generic thread mechanism:
         @ref BizSampling, @ref BizPlotting, @ref BizSignal,
         @ref BizSpectrum.

@section GenThrd_Drawback Type Safety Drawback

    You may have noticed that this system is somehow similar to the
    inheritance mechanism of object oriented languages: each module using
    Generic Threads works somehow as a class extending an abstract one.
        
    Unfortunately C++'s <em>virtual</em> keyword is not available to this
    mechanism, hence the toughtless programmer may call functions provided
    by a module on the handler obtained by another module.  This shall
    certainly bring to memory corruption.

@defgroup BizAlsaGw Alsa Gateway 

    This module hides Alsa's weird bogus under a hood, providing a simple
    initialization / finalization / reading interface.

    The alsagw_new() function allocates a new sampler and allows to provide
    some parameters for the underlying library (namely Alsa ASoundLib).

    In order to read data from Alsa, the alsagw_read() function can be
    invoked. Since the module initializes Alsa in a non-locking way, in
    principle this function will return immediately. Sometimes however the
    data may be not available. In those cases the user can specify a
    maximum amount of time to spend waiting for the resource to be
    available. Note that this feature is provided directly by Alsa through
    the snd_pcm_wait() function: this is probably implemented with some
    asynchronous calls.

    Other functions provided by this module allow to obtain additional
    information which can be used to correctly tune other modules.

    The alsagw_destroy() function can be eventually used to release
    resources.

@defgroup BizPlotting Plotting Interface

    As mentioned in the @ref CompileInstall section, this program uses
    GNU Libplot for the graphical representation of the data: this module
    provides a simple interface to the library suited for the
    application's purpose. In order to obtain a good thread-safe code, the
    reentrant version of the library is used. 

    The constructor provided by this module allows to allocate a X Window
    for plotting. The number of functions displayed for each plotting
    window can be configured trough the constructor. A plot_t object
    configured with N graphics will spawn up to N plotgr_t objects, each
    of which corresponds to a graphic.
    
    Since internally they implement a mutex-based semantics, it's
    perfectly safe to manage them from different threads. Two possible
    kind of actions can be performed:

    @arg Updating the graph through the plot_graphic_set() function;
    @arg Refreshing the plotting window trough the plot_redraw() function.

    The latter can be easily assigned to a specialized thread by using a
    @ref BizPlotThread.

@defgroup BizPlotThread Plotting Thread

    This module implements a @ref GenThrd "Generic Thread" which updates
    a plot. It's tought to extend the functionalities provided by the
    @ref BizPlotting module without mixing conceptually different
    execution logics. The thread is in charge of refreshing a plotting
    window.
   
    The refresh operation is performed 28 times per second, which is
    approximatively the frequency detectable by human eyes. The period
    corresponds to 35,714,286 nanoseconds.
   
    Luckily the chosen period is large enough for the plot_redraw()
    function to be executed. A test I run on my own computer shows that
    redrawing a 100-points sized grapic:
   
    @arg In the worst case it takes 28,760,359 nanoseconds;
    @arg In the average case it takes 6,081,251 nanoseconds.

    @note The plotting speed can be somehow tuned by providing a scaling
          parameter to the application. For further details see the
          @ref CLI section.

@defgroup BizSampling Sampling Thread

    This module implements a @ref GenThrd "Generic Thread" which achieves
    the sampling phase.

    The sampling period is obtained by calling the alsagw_get_period(),
    which in turns uses the value computed by Alsa from the parameters
    provided trough the \ref BizAlsaGw interface.

    When allocating a Sampling Thread a scaling factor must be provided:
    this value is used to dimension an internal buffer which size is
    multiple of the actual buffer needed by alsa. The buffer is used as a
    slotted circular array. Each periodic job of this thread achieves one
    non-blocking read: the result replaces the oldest slot.

    An external thread can read the data by using the sampth_get_samples()
    function, which performs a thread-safe reading from the oldest to the
    newest slot of the circular buffer.

@defgroup BizSignal Signal Thread

    This module allows to spawn one (or more) graphical windows showing
    the signal collected by the @ref BizSampling.

    When creating a Signal Thread two plotgr_t objects must be provided to
    the constructor: one for the first channel, one for the second
    channel. The plotgr_t objects can be obtained trough the
    plot_new_graphic() function, provided by the @ref BizPlotting module.
    They are not required to come from the same plot_t object.

@defgroup BizSpectrum Spectrum Thread

    This module allows to spawn one (or more) graphical windows showing
    the spectrum of the signal collected by the @ref BizSampling. The
    spectrum gets computed by using the fftw3 library.

    When creating a Signal Thread two couple of plotgr_t objects must be
    provided to the constructor: the former for the first channel (real and
    imaginary part of the spectrum), the latter for the second channel
    (again, real and imaginary part). The plotgr_t objects can be obtained
    trough the plot_new_graphic() function, provided by the
    @ref BizPlotting module. They are not required to come from the same
    plot_t object.

@defgroup BizOptions Command line options

    This module provides a wrapper for Getopt which extracts the options
    required for the Soto program.

    @see @ref CLI gives further information on the command line options.

*/
