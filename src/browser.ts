import { alloc } from "./alloc";

export function toBytesLE(n: bigint): Uint8Array {
  if (n < 0n) {
    n = -n;
  }

  const len = (n.toString(16).length + 1) >>> 1;
  const buf = alloc(len);
  const view = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);

  let offset = 0;
  while (offset < len - 7) {
    view.setBigUint64(offset, n, true);
    offset += 8;
    n >>= 64n;
  }

  let remaining = len & 7;
  if (remaining > 3) {
    view.setUint32(offset, Number(n) & 0xffff_ffff, true);
    offset += 4;
    remaining -= 4;
    n >>= 32n;
  }
  if (remaining > 1) {
    view.setUint16(offset, Number(n) & 0xffff, true);
    offset += 2;
    remaining -= 2;
    n >>= 16n;
  }
  if (remaining > 0) {
    view.setUint8(offset, Number(n));
  }

  return buf;
}

export function toBytesBE(n: bigint): Uint8Array {
  if (n < 0n) {
    n = -n;
  }

  const len = (n.toString(16).length + 1) >>> 1;
  const buf = alloc(len);
  const view = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);

  let remaining = len;
  while (remaining > 7) {
    remaining -= 8;
    view.setBigUint64(remaining, n, false);
    n >>= 64n;
  }

  if (remaining > 3) {
    remaining -= 4;
    view.setUint32(remaining, Number(n) & 0xffff_ffff, false);
    n >>= 32n;
  }
  if (remaining > 1) {
    remaining -= 2;
    view.setUint16(remaining, Number(n) & 0xffff, false);
    n >>= 16n;
  }
  if (remaining > 0) {
    view.setUint8(0, Number(n));
  }

  return buf;
}

export function fromBytesLE(buf: Uint8Array): bigint {
  const view = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);

  let n = 0n;

  let remaining = buf.length;
  while (remaining > 7) {
    remaining -= 8;
    n = (n << 64n) | view.getBigUint64(remaining, true);
  }

  if (remaining > 3) {
    remaining -= 4;
    n = (n << 32n) | BigInt(view.getUint32(remaining, true));
  }
  if (remaining > 1) {
    remaining -= 2;
    n = (n << 16n) | BigInt(view.getUint16(remaining, true));
  }
  if (remaining > 0) {
    n = (n << 8n) | BigInt(view.getUint8(0));
  }

  return n;
}

export function fromBytesBE(buf: Uint8Array): bigint {
  const view = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  const len = buf.length;

  let n = 0n;

  let offset = 0;
  while (offset < len - 7) {
    n = (n << 64n) | view.getBigUint64(offset, false);
    offset += 8;
  }

  let remaining = len & 7;
  if (remaining > 3) {
    n = (n << 32n) | BigInt(view.getUint32(offset, false));
    offset += 4;
    remaining -= 4;
  }
  if (remaining > 1) {
    n = (n << 16n) | BigInt(view.getUint16(offset, false));
    offset += 2;
    remaining -= 2;
  }
  if (remaining > 0) {
    n = (n << 8n) | BigInt(view.getUint8(offset));
  }

  return n;
}
