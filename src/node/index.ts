import { alloc } from "../alloc";
import { loadNative } from "./bindings";
import {
  toBytesLE as fbToBytesLE,
  toBytesBE as fbToBytesBE,
  fromBytesLE as fbFromBytesLE,
  fromBytesBE as fbFromBytesBE,
} from "./fallback";

let toBytesLE = fbToBytesLE;
let toBytesBE = fbToBytesBE;
let fromBytesLE = fbFromBytesLE;
let fromBytesBE = fbFromBytesBE;

try {
  const native = loadNative();

  toBytesLE = function toBytesLE(n) {
    let nWords = native.wordLength(n);
    let buf = alloc(nWords * 8);
    let nBytes = native.toBytesLE(n, buf);
    return buf.subarray(0, nBytes);
  };

  toBytesBE = function toBytesLE(n) {
    let nWords = native.wordLength(n);
    let buf = alloc(nWords * 8);
    let nBytes = native.toBytesBE(n, buf);
    return buf.subarray(0, nBytes);
  };

  fromBytesLE = native.fromBytesLE;
  fromBytesBE = native.fromBytesBE;
} catch (_e) {}

export { toBytesLE, toBytesBE, fromBytesLE, fromBytesBE };
