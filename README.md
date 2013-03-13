#Cloud Cache

Cloud cache is a library for sharing memory across compute nodes.  It utilizes a simpler version of the memcached protocol.  While currently running on the Linux kernel, the goal is to implement this on [Composite](https://github.com/gparmer/Composite), a component-based operating system.

This code is under development at this time and unstable.  I'll add more information once this is closer to deployment.

There are three commands currently working: GET, SET, DELETE.
They expect the follow formats;
GET: get <key>*
SET: sets <key> <flags> <bytes> \r\n
<data block>\r\n
DELETE: delete <key>

The * means 1 or more keys are acceptable.

Possible error return values:
"ERROR\r\n": client sent a nonexistent command name
"CLIENT_ERROR <error>\r\n": Something wrong on the client side - wrong number of arguments for example
"SERVER_ERROR <error>\r\n": 

More info and expected server responses can be found in the full [memcached protocol description](https://github.com/memcached/memcached/blob/master/doc/protocol.txt).


Feel free to send me questions at jcluck@gwmail.gwu.edu.
