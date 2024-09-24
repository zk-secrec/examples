importScripts('/assets/shared_arrays.js');
// importScripts('/pkg/web_mac_and_cheese_prover.js'); cant do that because of the `export`,
// so instead we have to coppy manually
importScripts('/assets/zksc_macncheese.js');
// importScripts('/pkg/web_mac_and_cheese_prover.js')

/* In arrs, the following keys:
var read_buffer;
var rb_first;
var rb_last;

var write_buffer;
var wb_first;
var wb_last;
var write_lock;
*/
var arrs;

function sleep(milliseconds) {
  var start = new Date().getTime();
  for (var i = 0; i < 100e7; i++) {
    if ((new Date().getTime() - start) > milliseconds) {
      break;
    }
  }
};

function myalert(x) {
  console.log("ALERTALERTALERTALERT");
};

function alert(x) {
  console.log(x);
};

function print_console(x) {
  console.log("VALUE: " + x);
}

function js_read_byte() {
  return read_byte_from_read_shared_arrays(arrs);
}

function js_write_byte(b) {
  return write_byte_to_write_shared_arrays(arrs, b);
}

function js_write_bytes(bytes) {
  return write_bytes_to_write_shared_arrays(arrs, bytes);
}

function js_flush() {
  console.log("ffi flush_webocket");
  self.postMessage({ flush: true });
}


var count = 0;

var instance_bytes_rcv;
var witness_bytes_rcv;
var relation_bytes_rcv;


self.onmessage = (msg) => {
  if (count == 0) {
    console.log("client worker received share memory");

    arrs = msg.data.arrays;

  } else {
    if (msg.data.command == "exec") {
      console.log('module received from main thread');
      var mod = msg.data.content;
      console.log("MODULE");
      console.log(mod);
      var witnessJSONString = msg.data.witness;
      // It is a little weird that we have to redo the imports!!! Oh well
      const imports = __wbg_get_imports();
      WebAssembly.instantiate(mod, imports)
        .then(function (instance) {
          console.log("EXPORTS")
          console.log(instance.exports);
          wasm = instance.exports;

          let r;

          /*test_web_macandcheese(instance_bytes_rcv, relation_bytes_rcv, witness_bytes_rcv);*/
          r = execute_prover(witnessJSONString);
          //r = wasm.test_web_macandcheese(ptr0, len0, ptr1, len1, ptr2, len2);
          let isOk = (r.length === 0);
          console.log(`MAC and CHEESE COMPLETED! result:${isOk}, message:${r}`);
          self.postMessage({
            verif_status: isOk,
            verif_message: r
          });

        });
    }
  }
  count++;
};

