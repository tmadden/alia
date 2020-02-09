#!/usr/bin/env python
import os
os.chdir('www')

import BaseHTTPServer, SimpleHTTPServer

port = 8000
print "Running on port %d" % port

SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map[
    '.wasm'] = 'application/wasm'

httpd = BaseHTTPServer.HTTPServer(('localhost', port),
                                  SimpleHTTPServer.SimpleHTTPRequestHandler)

print "Mapping \".wasm\" to \"%s\"" % \
    SimpleHTTPServer.SimpleHTTPRequestHandler.extensions_map['.wasm']
httpd.serve_forever()
