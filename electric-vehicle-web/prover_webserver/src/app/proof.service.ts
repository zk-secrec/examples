import { Injectable } from '@angular/core';
import { BehaviorSubject, Subject } from 'rxjs';
import { MapService } from './map.service';
import { ToastsService } from './toasts.service';

type WorkerView = {
  read_buffer: Uint8Array;
  rb_first: Uint32Array;
  rb_last: Uint32Array;
  write_buffer: Uint8Array;
  wb_first: Uint32Array;
  wb_last: Uint32Array;
  write_lock: Uint32Array;
}

function make_websocket_url() {
  var l = window.location;
  return ((l.protocol === "https:") ? "wss://" : "ws://") + l.hostname + ":" + "8080";
}

@Injectable({
  providedIn: 'root'
})
export class ProofService {

  private progressValue = new BehaviorSubject<number>(0);
  progressValue$ = this.progressValue.asObservable();

  private proofDone = new Subject<boolean>;
  proofDone$ = this.proofDone.asObservable();

  private proofStatus = new Subject<boolean>;
  proofStatus$ = this.proofStatus.asObservable();

  read_buffer: SharedArrayBuffer;
  rb_first: SharedArrayBuffer;
  rb_last: SharedArrayBuffer;

  write_buffer: SharedArrayBuffer;
  wb_first: SharedArrayBuffer;
  wb_last: SharedArrayBuffer;

  write_lock: SharedArrayBuffer;

  progress: SharedArrayBuffer;
  progressView: Uint32Array;

  websocketWorker?: Worker;
  macandcheeseWorker?: Worker;

  viewsWebsocket: WorkerView;
  viewsMacandcheese: WorkerView;

  compiledModule?: WebAssembly.Module;

  timer?: DOMHighResTimeStamp;

  constructor(
    private mapService: MapService,
    private toastsService: ToastsService
  ) {
    this.read_buffer = new SharedArrayBuffer(Uint8Array.BYTES_PER_ELEMENT * 20_000_000);
    this.rb_first = new SharedArrayBuffer(Uint32Array.BYTES_PER_ELEMENT);
    this.rb_last = new SharedArrayBuffer(Uint32Array.BYTES_PER_ELEMENT);

    this.write_buffer = new SharedArrayBuffer(Uint8Array.BYTES_PER_ELEMENT * 20_000_000);
    this.wb_first = new SharedArrayBuffer(Uint32Array.BYTES_PER_ELEMENT);
    this.wb_last = new SharedArrayBuffer(Uint32Array.BYTES_PER_ELEMENT);

    this.write_lock = new SharedArrayBuffer(Uint32Array.BYTES_PER_ELEMENT);

    this.progress = new SharedArrayBuffer(Uint32Array.BYTES_PER_ELEMENT);
    this.progressView = new Uint32Array(this.progress);

    this.viewsWebsocket = this.create_views();
    this.viewsMacandcheese = this.create_views();

    WebAssembly.compileStreaming(fetch('/assets/zksc_macncheese_bg.wasm'))
      .then(mod => { this.compiledModule = mod })
      .catch(reason => { console.log("Compiling failed: " + reason) });

  }

  create_views(): WorkerView {
    return {
      read_buffer: new Uint8Array(this.read_buffer),
      rb_first: new Uint32Array(this.rb_first),
      rb_last: new Uint32Array(this.rb_last),
      write_buffer: new Uint8Array(this.write_buffer),
      wb_first: new Uint32Array(this.wb_first),
      wb_last: new Uint32Array(this.wb_last),
      write_lock: new Uint32Array(this.write_lock)
    };
  }

  executeProof() {
    this.createWorkers();
    this.progressValue.next(0);
    this.timer = performance.now();
    this.proofDone.next(false);
    const witnessJSON = pathToWitnessJSON(<number[][]>this.mapService.getPath());
    this.macandcheeseWorker?.postMessage({
      command: "exec",
      content: this.compiledModule,
      witness: witnessJSON
    });
    setInterval(this.updateProgress.bind(this), 500);
  }

  createWorkers() {
    this.websocketWorker = new Worker('/assets/worker_websocket.js', { name: "my_websocket_worker" });
    this.macandcheeseWorker = new Worker('/assets/worker_macandcheese.js', { name: "macandcheese_worker" });

    Atomics.store(this.viewsWebsocket.rb_first, 0, 0);
    Atomics.store(this.viewsWebsocket.rb_last, 0, 0);
    Atomics.store(this.viewsWebsocket.wb_first, 0, 0);
    Atomics.store(this.viewsWebsocket.wb_last, 0, 0);
    Atomics.store(this.viewsWebsocket.write_lock, 0, 0);

    Atomics.store(this.progressView, 0, 0);

    this.websocketWorker.postMessage({ arrays: this.viewsWebsocket, progress: this.progressView, websocket_url: make_websocket_url(), });
    this.macandcheeseWorker.postMessage({ arrays: this.viewsMacandcheese });

    this.macandcheeseWorker.onmessage = (msg) => {
      if ("flush" in msg.data) {
        this.websocketWorker?.postMessage({ flush: true });
      }
      if ("verif_status" in msg.data) {
        this.proofStatus.next(msg.data.verif_status);
        this.proofDone.next(true);
        if (msg.data.verif_status) {
          this.toastsService.show(`All checks passed.\nTotal time: ${((performance.now() - this.timer!) / 1000).toFixed(2)} seconds.`, {
            classname: 'text-bg-success',
            header: 'Proof Accepted'
          });
        } else {
          this.toastsService.show(`Assertion failed:\n${msg.data.verif_message}`, {
            classname: 'text-bg-danger',
            header: 'Proof Rejected'
          });
        }
        delete this.macandcheeseWorker;
        delete this.websocketWorker;
      }
    }
  }


  //Total communication amount, use val below to set it for use case
  total = 10137050 

  updateProgress() {
    const val = Atomics.load(this.progressView, 0);
    //console.log(val);
    this.progressValue.next(
      val / this.total * 100
    );
  }
}

function pathToWitnessJSON(path: number[][]): string {
  return JSON.stringify({
    coordinates: path.map(point => point.map(val => Math.round(val).toString()))
  });
}
