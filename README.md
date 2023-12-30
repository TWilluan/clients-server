

# WHOIS request processor

## Overview
This project implements a simple client-server architecture for processing WHOIS requests. The client sends a WHOIS query to the server, the server processes the request, and the output is sent back to the client for further use.

## Usage

- To remove current gcc files (client and server):
```js
make clean
```
- To compile clients.c and server.c
```js
make
```
- To run server.c:
```js
./server <portnumber>
```
- To run client.c:
```js
./client <hostname>:<portnumber> whois [options] argument-list
```
** please run client and server in different terminal

## Features
- **Client-Server Communication:** The project establishes communication between a client and a server using a custom protocol for WHOIS requests.

- **WHOIS Query Processing:** The server is capable of processing WHOIS queries received from the client.

- **Response Handling:** The server sends the processed WHOIS information back to the client, allowing the client to print the output.