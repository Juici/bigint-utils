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
  ({ toBytesLE, toBytesBE, fromBytesLE, fromBytesBE } = loadNative());
} catch (_) {}

export { toBytesLE, toBytesBE, fromBytesLE, fromBytesBE };
