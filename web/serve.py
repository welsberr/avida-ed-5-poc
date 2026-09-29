# Special serve method to allow atomics and threads in a web browser.
from http.server import SimpleHTTPRequestHandler, HTTPServer
import os

class CORSRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

host = os.environ.get('HOST', '127.0.0.1')
port = int(os.environ.get('PORT', '8000'))
httpd = HTTPServer((host, port), CORSRequestHandler)
display_host = '127.0.0.1' if host == '0.0.0.0' else host
print(f"Serving on http://{display_host}:{port}")
httpd.serve_forever()
