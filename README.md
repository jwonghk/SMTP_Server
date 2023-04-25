# SMTP_Server

SMTP: A protocol used by emails clients to send emails. The email messages will be saved to the SMTP server locally and will be 
able to be retrieved later. 

This project involves:
- develop an email server that implements the SMTP protocol
- TCP sockets in C
- programming and debugging skills as they relate to the use of sockets in C
- implement the server side of a protocol
- develop general networking experimental skills
- develop a further understanding of what TCP does, and how to manage TCP connections from the server perspective

The SMTP protocol, described in RFC 5321, is used by mail clients to send email messages to a specific email user. 
The server side of this protocol is implemented here. More specifically, the following commands are supported:

    HELO and EHLO (client identification)
    MAIL (message initialization)
    RCPT (recipient specification)
    DATA (message contents)
    RSET (reset transmission)
    VRFY (check username)
    NOOP (no operation)
    QUIT (session termination)

    
 This works together with the POP3 server which will be able to retrieve email message. 
