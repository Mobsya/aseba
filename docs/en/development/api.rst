==================================
Developing applications for Thymio
==================================


Connection to the device manager
================================

Instead of communicating directly with a Thymio, applications communicate with the Thymio Device Manager.

While developing an application, make sure the Thymio Device Manager is running.

It is possible to control more than one Thymio through the same connection, therefore, an application should only open a single connection
to the device manager.

The device manager exposes 2 endpoints
 * A TCP endpoint on an ephemeral port
 * A WebSocket endpoint on a fixed port: ``8597``

Both endpoints use the same flatbuffer-based protocol.

Device Manager Discovery
------------------------

To discover the available Device Manager, client application must scan the ``_mobsya._tcp`` service using Bonjour or avahi.
Each Device Manager is uniquely identifiable through the ``uuid`` service txt record, which is helpful as a given Device Manager might be
visible on multiple interfaces.

.. note::  The Qt API handles discovery automatically.

.. note::  The JS API does not provide a discovery facility as Web browsers can't access the zero-conf records. Instead, Web applications need to obtain the connection url out-of-band, for example by passing it as an URL query parameter.


Protocol Overview
=================

The protocol is based on Flatbuffer which is a binary serialization format based on a schema.
`The schema is available on the GitHub repository <https://github.com/Mobsya/aseba/blob/master/aseba/flatbuffers/thymio.fbs>`_, and is always authoritative.
(if the schema and the documentation disagree, trust the schema!).


When using the WebSocket endpoint, each WebSocket message corresponds to a Flatbuffer ``Message``,
while using the TCP endpoint, each message is prefixed by the size of the message (excluding its size)
as a 32 bits (little endian) unsigned integer.

Some messages also embed a Flexbuffer payload, Flexbuffer being a schema-less version of Flatbuffer.
However, Flexbuffer is only available for C++ and Swift. We provide a WebAssembly port for JavaScript.
People looking to port The Device Manager to another programming language would have to provide a Flexbuffer binding.

.. note::  Use the provided Qt or JS API to avoid dealing with the low-level details of the protocol.

The protocol is fully duplex, which means both endpoint can send more messages before getting a reply.
Not all messages will get a reply.

Rate Limiting And Error Management
----------------------------------
Sending over-sized messages, invalid or corrupted messages will lead to the connection being dropped.
While not yet implemented, the protocol will be rate limited. at some point.

Handshake
---------

When connecting, the client must send a ``ConnectionHandshake`` message, with the current version of the protocol, as well as the minimum supported version of the protocol.
The client might also specify a ``maxMessageSize`` representing the maximum size of message it will accept from the server.

The server will reply with the same message, except ``protocolVersion`` will design the actual version of the protocol that needs to be used by both parties.
If ``protocolVersion`` is 0, the client and server have no common supported protocol version and the communication will be dropped.

Requests
--------

Messages which take a ``request_id`` field are requests: They expect a response. The type of Response is message specific, but the response will have the same
``request_id``. Response might not be received in the order the Request where sent.
When sending a request, you must fill ``request_id`` with a unique number, by maintaining a pool of number or using a random number generator.
All request can fail and result in an ``Error`` message.

.. note::  The Qt and JS APIs use promise or promise-like objects to handle requests.

Node Status
-----------

Once the Handshake is complete, the Device Manager will send ``NodesChanged`` messages, containing a list of ``Nodes``.
Each `Node` describes a thymio, its status, type, name and ``node_id``.
When nodes are added, modified or disconnected, the Device Manager will send more ``NodesChanged`` automatically.

.. note::  The Qt and JS APIs maintain a list of connected Thymios.

.. note::  A ``RequestListOfNodes`` message can be send to request a list of all nodes, but most applications should not have to do that.


Node Id
-------

The API identifies each Thymio using a 128 bits Unique Identifier.
This id is used through the protocol to send request to a thymio.


Node Locking and Unlocking
--------------------------

To send code to a Thymio, set variable, events or shared variable, it needs to be locked.
This is done through the ``LockNode`` request.
If the node is already in use by somebody else ( its ``status`` is ``busy``) the node can not be locked.
Nodes can be unlocked with the ``UnlockNode`` request, or by dropping the connection.

Watching Node
-------------

Some infos can be send at high frequency and therefore require to explicitely subscribe to them in order not to
saturate the bandwith between the Device Manager and the Thymios.

To start monitoring some infos of a Thymios send a ``WatchNode`` message, containing a bitflag of the infos you want to watch.
Watchable infos include variable change, vm status changes and events.















