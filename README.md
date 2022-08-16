# easytcp
An easy to use TCP client and server library.

This library uses a packet type method to send data.  The data is prefixed with a 8 byte header that
has a 4 byte pattern and 4 byte length.

The pattern is so you know this is the start of the data and the length is so that you do not have to
read data byte by byte.

Also see my UDP library called easyudp.
