# LAppS protocol

NOTE: This document uses http://www.jsonrpc.org/specification as a base. 

## 1. Overview

  LAppS Protocol is made after JSON-RPC, therefore it is a stateless, light-weight
remote procedure call (RPC) protocol. This specification defines several data 
structures in JSON data format and the rules around their processing. It is an 
application level protocol, thus makes it transport agnostic. However the LAppS
Protocol in this specification is developed and optimized to be used over 
WebSockets (RFC 6455).

  It is designed to be a little more simple then JSON-RPC. LAppS Protocol is a 
binary protocol, all transmissions are encoded in CBOR format as specified in 
RFC 7049.

## 2. Conventions

  The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", "SHOULD", 
"SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this document are to be 
interpreted as described in RFC 2119.

  Since LAppS protocol utilizes CBOR which MUST support all JSON data types 
and must support conversion from JSON and to JSON (as per 1.1.6 of the RFC 7049), 
all the data structures in this specification are represented in human readable 
form in JSON format.


  All member names exchanged between the Client and the Server that are 
considered for matching of any kind should be considered to be case-sensitive. 
The terms function, method, and procedure can be assumed to be interchangeable.

  The terms "Out of Order Notification (OON)", "Message" or "Notification" can
be assumed to be interchangeable.

  The Client is defined as the origin of Request objects and the handler of 
Response objects. The Server is defined as the origin of Response objects
and the handler of Request objects. 

  The Server is defined as well as the origin of the Broadcast messages (or 
notifications) or the origin of any Out of Order Notifications (OON) to the Client.

  One implementation of this specification could easily fill both of those roles, 
even at the same time, to other different clients or the same client. This 
specification does not address that layer of complexity.

## 3. Protocol 
  All communications between Client and Server are initiated by client. For any 
Request of the Client, the Server MUST provide a response. The Server may send
OONs to the client after first Request-Response exchange. The Server Responses
and Notifications MAY take place out of order. Therefore the Server MUST 
identify its Response with CID (Channel ID) object {"cid" : number}. All 
Responses to Client Requests are taking place on Command Channel (CCH) with ID 0: 
{"cid" : 0}. Responses on CCH MAY NOT appear out of order. All OONs MUST take 
place on different channels then CCH. Thus CCH is reserved for Request-Response
half duplex communications. The purpose of the channel is to provide the Client 
with possibility to distinguish Server Responses from OON, those MAY take place 
out of requested order. For example the server  MAY broadcast messages to the 
clients in the  same time some Clients are awaiting Response on CCH. The 
broadcast message MAY appear before the Response to expected Request, though the
OON MAY NOT appear on the CCH. 


## 4. Request object

  The Request is expressed as a single JSON Object with the following members:

  *lapps*

    A String specifying the version of the LAppS protocol. MUST be exactly "1".
  
  *method*

    A String containing the name of the method to be invoked. Method names MAY NOT
    contain a period character (U+002E or ASCII 46).  Method names those 
begin with underscore (`_', ASCII 95) are reserved for internal methods and 
extensions and MUST NOT be used for anything else.

  *params*

    An Array that holds the parameter values to be used during the invocation of 
the method. This member MAY be omitted. Each element of this Array will become
the function argument in the preserved order. Therefore `params' must be an Array
containing the values in the Server expected order.

  *cid*

    A Number. This member MAY NOT be used for CCH communications. This member
MAY be used for Client Notifications. Client Notifications MAY NOT be answered
by the Server. The Server and Client MUST agree on specific channels for Client
Notifications, if and ONLY if the Server supports Client's Notifications. This 
specification does not define any specifics for Client's Notifications.

## 5. The Response object
  The Response is expressed as a single JSON Object, with the following members:

  *status*

    A Number MUST be 1 or 0, defining success or error of the remote method 
invocation respectively. This member is MANDATORY.

  *error*

    An Object specifying the error code and the error message. This Object 
appears in Response only when the status is 0 (error).

  *error.code*

    A Number of the error code (MAY NOT BE null or empty). The error codes from 
and including -32768 to -32000 are reserved for pre-defined errors and exactly
the same as specified in [JSON-RPC v2.0 Specification](http://www.jsonrpc.org/specification)

  *error.message*

    A String describing the error (MAY NOT be null, MAY BE an empty string).

  *error.data*

    A Primitive or Structured value that contains additional information about 
the error. This may be omitted. The value of this member is defined by the 
Server (e.g. detailed error information, nested errors etc.).

  *result*

    An Array with results of the remote method invocation. This Object appears 
in Response only when status is 1 (success) and cid is CCH (equals 0). This member MAY
be empty. This member MAY NOT be null.

  *cid*

    A Number representing communication channel. MUST BE exactly 0 for CCH. This
member is MANDATORY for any type of Response including OON.

    
## 6. Out of Order Notification (OON) Object
  The OON is expressed as a single JSON Object, with the following MANDATORY 
members:

  *cid*

    A Number representing communication channel.

  *message*

    An Array defined by the Server.


