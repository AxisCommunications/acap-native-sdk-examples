*Copyright (C) 2021, Axis Communications AB, Lund, Sweden. All Rights Reserved.*

# Web server examples introduction

This folder contains ACAP Native SDK examples that show two ways to expose HTTP endpoints and web content on an Axis device.

## Available examples

1. **Reverse proxy using fixed port**
 The reverse proxy example using CivetWeb demonstrates how to setup the Axis device web server (Apache) in a Reverse Proxy configuration, where HTTP requests to the application are routed to a web server CivetWeb running inside the ACAP application and acting as a CGI. See [reverse-proxy-using-fixed-port](./reverse-proxy-using-fixed-port/).

2. **HTTP requests using FastCGI**
 The HTTP request example using FastCGI explains how to build an ACAP application that can handle HTTP requests sent to the Axis device. The application uses FastCGI to handle the request and response, and uriparser to parse the received query parameters. See [http-requests-using-fastcgi](./http-requests-using-fastcgi/).

## Which one to use

- Choose the **reverse proxy** example when you want an embedded web server with flexible routing and easy reuse of existing server-side C code.
- Choose the **FastCGI** example when you want tighter integration with Apache and a native ACAP web serving flow.
