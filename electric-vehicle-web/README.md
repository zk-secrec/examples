# ZKSC Electric Vehicle WASM demo
This project contains the WebAssembly portion of the EV use case demonstrator.
As the full demonstrator uses a device for coordinate logging, signing, and path selection,
certain features have been simplified. The signing portion has been omitted and two preselected
paths have been added.

To build this project, you need to be on the `x86_64-unknown-linux-gnu` architecture.

For the prover server, in the `prover_webserver` folder run first `npm install` and then `ng serve`.
For the verifier, run `zksc_macncheese_websocket` from the `verifier` folder.

You can choose the two given paths with the buttons. 

Enabling SSL (optional) will speed up the proof considerably as it allows the use of `SharedArrayBuffer`.
To do so:

* on the prover side add the necessary changes to `prover_webserver/src/angular.json` and then run `ng serve --ssl`

* on the verifier side you can add the necessary arguments with `./zksc_macncheese_websocket --key <key_path> --cert <cert_path> --addr <ip:port>`
