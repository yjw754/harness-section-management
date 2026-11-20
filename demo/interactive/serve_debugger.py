"""Serve the generated Stage 2 debugger on the local machine."""

import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import re
from urllib.parse import urlsplit


class DebuggerHandler(SimpleHTTPRequestHandler):
    def fingerprinted_asset(self):
        return re.fullmatch(r"/(app|hrn_interactive_solver)\.[0-9a-f]{64}\.(css|js|wasm)",
                            urlsplit(self.path).path) is not None

    def send_head(self):
        if not self.fingerprinted_asset():
            # A same-second rebuild must still return the new index bytes.
            del self.headers["If-Modified-Since"]
            del self.headers["If-None-Match"]
        return super().send_head()

    def send_response(self, code, message=None):
        super().send_response(code, message)
        if code in (200, 304) and self.fingerprinted_asset():
            self.send_header("Cache-Control", "public, max-age=31536000, immutable")
        else:
            self.send_header("Cache-Control", "no-store")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, default=Path("build-wasm/interactive"))
    parser.add_argument("--port", type=int, default=8002)
    args = parser.parse_args()
    if not (args.directory / "index.html").is_file():
        parser.error(f"missing {args.directory / 'index.html'}; build hrn_interactive_solver first")

    handler = partial(DebuggerHandler, directory=str(args.directory.resolve()))
    with ThreadingHTTPServer(("127.0.0.1", args.port), handler) as server:
        print(f"Stage 2 debugger: http://127.0.0.1:{server.server_port}/", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
