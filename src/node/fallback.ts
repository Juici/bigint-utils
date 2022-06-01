import { Buffer } from "buffer";

import { alloc } from "../alloc";

const hexWrite:
  | ((this: Uint8Array, hex: string, offset: number, len: number) => number)
  | undefined = Buffer.prototype.hexWrite;

const hexSlice:
  | ((this: Uint8Array, start: number, end: number) => string)
  | undefined = Buffer.prototype.hexSlice;

const toString: ((this: Uint8Array, encoding: "hex") => string) | undefined =
  Buffer.prototype.toString;

const fromHex: (hex: string) => Uint8Array =
  typeof hexWrite === "function"
    ? (hex) => {
        let buf = alloc(hex.length >>> 1);
        hexWrite.call(buf, hex, 0, hex.length);
        return buf;
      }
    : (hex) => Buffer.from(hex, "hex");

const toHex: (buf: Uint8Array) => string =
  typeof hexSlice === "function"
    ? (buf) => hexSlice.call(buf, 0, buf.length)
    : typeof toString === "function"
    ? (buf) => toString.call(buf, "hex")
    : (buf) =>
        Buffer.from(buf.buffer, buf.byteOffset, buf.byteLength).toString("hex");

export function toBytesLE(n: bigint): Uint8Array {
  if (n < 0n) {
    n = -n;
  }

  let hex = n.toString(16);
  if (hex.length & 1) {
    hex = "0" + hex;
  }

  return fromHex(hex).reverse();
}

export function toBytesBE(n: bigint): Uint8Array {
  if (n < 0n) {
    n = -n;
  }

  let hex = n.toString(16);
  if (hex.length & 1) {
    hex = "0" + hex;
  }

  return fromHex(hex);
}

export function fromBytesLE(buf: Uint8Array): bigint {
  const hex = toHex(buf.reverse());
  return BigInt(`0x${hex}`);
}

export function fromBytesBE(buf: Uint8Array): bigint {
  const hex = toHex(buf);
  return BigInt(`0x${hex}`);
}
