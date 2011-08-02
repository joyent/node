## Events

Many objects in Node emit events: a `net.Server` emits an event each time
a peer connects to it, a `fs.readStream` emits an event when the file is
opened. All objects which emit events are instances of `events.EventEmitter`.
You can access this module by doing: `require("events");`

Typically, event names are represented by a camel-cased string, however,
there aren't any strict restrictions on that, as any string will be accepted.

Functions can then be attached to objects, to be executed when an event
is emitted. These functions are called _listeners_.


**Namespaces** with **Wildcards**
To use namespaces/wildcards, pass the `wildcard` option into the EventEmitter constructor.
When namespaces/wildcards are enabled, events can either be strings (`foo.bar`) separated
by a delimiter or arrays (`['foo', 'bar']`). The delimiter is also configurable as a 
constructor option.

An event name passed to any event emitter method can contain a wild card (the `*` character).
If the event name is a string, a wildcard may appear as `foo.*`. If the event name is an array, 
the wildcard may appear as `['foo', '*']`.

If either of the above described events were passed to the `on` method, subsequent emits such 
as the following would be observed...


   emitter.emit(['foo.bazz']);
   emitter.emit(['foo', 'bar']);


### events.EventEmitter

To access the EventEmitter class, `require('events').EventEmitter`.

When an `EventEmitter` instance experiences an error, the typical action is
to emit an `'error'` event.  Error events are treated as a special case in node.
If there is no listener for it, then the default action is to print a stack
trace and exit the program.

All EventEmitters emit the event `'newListener'` when new listeners are
added.

#### emitter.addListener(event, listener)
#### emitter.on(event, listener)

Adds a listener to the end of the listeners array for the specified event.


    server.on('connection', function (stream) {
      console.log('someone connected!');
    });


#### emitter.onAny(listener)

Adds a listener that will be fired when any event is emitted.

    function f(value) { 
      console.log('This event will be listened to exactly four times.');
    };

    server.onAny(f);


#### emitter.unAny(listener)

Removes the listener that will be fired when any event is emitted.


    server.unAny(f);


#### emitter.once(event, listener)

Adds a **one time** listener for the event. The listener is invoked only the first time the event is fired, after which it is removed.


    server.once('connection', function (value) {
      console.log('Ah, we have our first value!');
    });


#### emitter.many(event, timesToListen, listener)

Adds a listener that will execute **n times** for the event before being removed. The listener is invoked only the first time the event is fired, after which it is removed.


    server.many('connection', 4, function (value) {
      console.log('Ah, we have captured a user!');
    });


#### emitter.removeListener(event, listener)
#### emitter.un(event, listener)

Remove a listener from the listener array for the specified event. **Caution**: changes array indices in the listener array behind the listener.


    var callback = function(stream) {
      console.log('someone connected!');
    };
    server.on('connection', callback);
    // ...
    server.un('connection', callback);


#### emitter.removeAllListeners([event])

Removes all listeners, or those of the specified event.


#### emitter.setMaxListeners(n)

By default EventEmitters will print a warning if more than 10 listeners are added to it. This is a useful default which helps finding memory leaks. Obviously not all Emitters should be limited to 10. This function allows that to be increased. Set to zero for unlimited.


#### emitter.listeners(event)

Returns an array of listeners for the specified event. This array can be manipulated, e.g. to remove listeners.


    server.on('connection', function (stream) {
      console.log('someone connected!');
    });
    
    console.log(util.inspect(server.listeners('connection')); // [ [Function] ]


#### emitter.listenersAny(event)

Returns an array of listeners that are listening for any event that is specified. This array can be manipulated, e.g. to remove listeners.


    server.onAny(function(value) {
      console.log('someone connected!');
    });
    
    console.log(console.log(server.listenersAny()[0]); // [ [Function] ] // someone connected!


#### emitter.emit(event, [arg1], [arg2], [...])

Execute each of the listeners that may be listening for the specified event name in order with the list of arguments.

#### Event: 'newListener'

`function (event, listener) { }`

This event is emitted any time someone adds a new listener.

