# SimpleChat Version 0.1

## Main Features

1. **A basic asyncio framework**
- **How to use?**
  - 1.1 **Define an async function**
  - Keywords: `ASYNC_DEF`, `ASYNC_END_DEF`, `ASYNC_RETURN`
  - An async function will be associated with an eventloop, and the behavior of the function will be controlled by it. Async function cannot be called directly, but should be called with `AsyncCall`
  - Warning: If you want to use the grammar `switch-case`, you should never contain an `AWAIT` in it (use `if-elseif-else` instead)
  - Usage:
    ```c
    ASYNC_DEF(funcname, typeof(param1), nameof(param1), ...)
        // content
        ASYNC_RETURN(retval) 
        // return 0 to quit normally, return -1 to abort it
    ASYNC_END_DEF
    ```
  - 1.2. **Create an coroutine**
  - Keywords: `COROUTINE`
  - Coroutine is an async function to be called asynchronously
  - Warning: Never assign an Coroutine to an Variable. Use `Await` or `Detach` directly
  - Usage:
    ```c
    COROUTINE(funcname, typeof(param1), nameof(param1), ...)
    ```
  - 1.3. **Await an coroutine**
  - Keywords: `AWAIT`, `ASYNC_ARG`
  - Give control back to the eventloop and ask the eventloop to call the coroutines, and the eventloop will awake the function as all the awaited coroutines finished, the function will be run where it quit
  - Warning: Since it involves an function context switch, you need to denote all the local variables you want to keep by `ASYNC_ARG`, all the variables are saved through an simple assignment `=`
  - Usage:
    ```c
    AWAIT(
        ASYNC_ARG(typeof(var1), nameof(vaar1), ...),
        COROUTINE(funcname1, ...),
        COROUTINE(funcname2, ...),
        ...
    )
    ```
  - 1.4. **Detach an coroutine**
  - Keywords: `DETACH`
  - Ask the eventloop to call the coroutines without giving control back to the eventloop. If you want to give back the control, Add `AWAIT(ASYNC_ARG(),COROUTINE(AsyncSleep, 0))` after it
  - Usage:
    ```c
    DETACH(
        COROUTINE(funcname1, ...),
        COROUTINE(funcname2, ...),
        ...
    )
    ```
  - 1.5. **Builtin functions**
    - 1.5.1. `AsyncSleep`
      - Suspends the execution of the current thread for the specified number of milliseconds
      - param: `int millisecond`: delay time
      - no return
    - 1.5.2. `AsyncAccept`
      - Accept the client socket, the client socket would be a receive socket
      - param `AsyncSockt* async_socket`: the listen socket
      - param `AsyncSockt** result_socket`: the client socket for return
      - return `(*result_socket)`, NULL if error
    - 1.5.3. `AsyncRecv`
      - Receive one message from the socket
      - param `AsyncSocket* async_socket`: the socket to receive from
      - param `char** msg`: the message it returns
      - return `(*msg)`: NULL if connection closed
    - 1.5.4. `AsyncSend`
      - Send a message through the socket
      - param `AsyncSocket* async_socket`: the socket to send to
      - param `const char* msg`: the msg to be send
      - param `int* result`: the error notifocation
      - return `(*result)`: 0 if success, -1 if error
  - 1.6. **How to call**
    - 1.6.1. **Init**
    - `InitErrorStream()`: return 0 if success, -1 if error
    - `syncIOInit()`: return 0 if success, -1 if error
    - `EventLoop* evlp = CreateEventLoop()`: NULL if error
    - `RegisterAsyncFunction(evlp, funcname)`: register async function, return 0 if success, -1 if error
    - `ASYNC_CALL(evlp, funcname, param1, param2, ...)`: ask the eventloop to call the async function after the eventloop being run
    - `CreateAsyncSocket(ip, port, send_time_out, recv_time_out, listen_mode, receive_mode, use_ipv6)`: Create an async socket
    - 1.6.2. **Run**
    - `EventLoopRun(evlp, delay_time)`: run the event loop, it won't return until all tasks finished or something strange happens
    - 1.6.3. **Clean**
    - `ReleaseEventLoop(evlp)`: return 0 if success, -1 if error
2. **A basic error stream**
  - Warning: only use it in the main thread (async framework is surely compatible with it)