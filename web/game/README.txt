This folder is the target for the Emscripten web build.

Build instructions:
- Install and activate the Emscripten SDK (emsdk).
- From the repo root run: ./scripts/build_wasm.sh
- The build will place dungeoncrawler_wasm.html, dungeoncrawler_wasm.js and dungeoncrawler_wasm.wasm into this directory.

After building, open web/game/dungeoncrawler_wasm.html (serve over HTTP) or embed web/game/index.html in your site.