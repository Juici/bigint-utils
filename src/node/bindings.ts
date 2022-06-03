import fs from "fs";
import path from "path";
import process, { arch, platform, versions } from "process";

import { name } from "../../package.json";
import { packageRoot } from "../root";

const libName = name.replace(/-/g, "_");
const root = process.env[`${libName.toUpperCase()}_PREBUILD`] ?? packageRoot;

const abi = versions.modules;
const runtime = isElectron()
  ? "electron"
  : versions.nw
  ? "node-webkit"
  : "node";
const libc = process.env.LIBC || (isAlpine() ? "musl" : "glibc");
const armv =
  process.env.ARM_VERSION ||
  (arch === "arm64"
    ? "8"
    : (process.config.variables as { arm_version?: string }).arm_version || "");
const uv = versions.uv?.split(".")[0] ?? "";

const modulePath = (() => {
  if (!process.env.PREBUILDS_ONLY) {
    const release = findBuild(path.join(root, "build/Release"));
    if (release) {
      return release;
    }

    const debug = findBuild(path.join(root, "build/Debug"));
    if (debug) {
      return debug;
    }
  }

  const prebuild = findPrebuild(root);
  if (prebuild) {
    return prebuild;
  }

  const nearby = findPrebuild(path.dirname(process.execPath));
  if (nearby) {
    return nearby;
  }

  return null;
})();

type NativeModule = {
  toBytesLE(n: bigint): Uint8Array;
  toBytesBE(n: bigint): Uint8Array;
  fromBytesLE(buf: Uint8Array): bigint;
  fromBytesBE(buf: Uint8Array): bigint;
};

export function loadNative(): NativeModule {
  if (!modulePath) {
    throw new Error("Native module not found");
  }
  return require(modulePath);
}

/**
 * Search for a node-gyp build.
 */
function findBuild(dir: string): string | null {
  try {
    return (
      fs
        .readdirSync(dir, { withFileTypes: true })
        .find((f) => f.isFile() && f.name.endsWith(".node"))?.name ?? null
    );
  } catch (_e) {}

  return null;
}

/**
 * Search for a prebuild.
 */
function findPrebuild(dir: string): string | null {
  const prebuilds = findArch(path.join(dir, "prebuilds"));
  if (!prebuilds) {
    return null;
  }

  type Tags = {
    file: string;
    specificity: number;
    runtime?: "node" | "electron" | "node-webkit";
    napi?: true;
    abi?: string;
  };

  let pick: Tags | undefined;

  const entries = fs.readdirSync(prebuilds, { withFileTypes: true });
  files: for (const file of entries) {
    if (!file.isFile()) {
      continue;
    }

    const arr = file.name.split(".");
    if (arr.pop() !== "node") {
      continue;
    }

    const tags: Tags = { file: file.name, specificity: 0 };
    const seen = new Set<string>();

    tags: for (const tag of arr) {
      let tagType: string;

      switch (tag) {
        case "node":
        case "node-webkit":
        case "electron":
          tags.runtime = tag;
          tagType = "runtime";
          break;

        case "napi":
          tags.napi = true;
          tagType = "napi";
          break;

        case "glibc":
        case "musl":
          if (libc !== tag) {
            continue files;
          }
          tagType = "libc";
          break;

        default:
          if (tag.startsWith("uv")) {
            if (uv !== tag.slice(2)) {
              continue files;
            }
            tagType = "uv";
          } else if (tag.startsWith("abi")) {
            tags.abi = tag.slice(3);
            tagType = "abi";
          } else if (tag.startsWith("armv")) {
            if (armv !== tag.slice(4)) {
              continue files;
            }
            tagType = "armv";
          } else {
            continue tags;
          }
      }

      if (!seen.has(tagType)) {
        seen.add(tagType);
        tags.specificity++;
      }
    }

    if (tags.runtime !== runtime && (tags.runtime !== "node" || !tags.napi)) {
      continue files;
    }
    if (tags.abi !== abi && !tags.napi) {
      continue files;
    }

    if (
      !pick ||
      (pick.runtime !== tags.runtime && tags.runtime !== runtime) ||
      (pick.abi !== tags.abi && tags.abi) ||
      pick.specificity < tags.specificity
    ) {
      pick = tags;
    }
  }

  return pick ? path.join(prebuilds, pick.file) : null;
}

/**
 * Search for a directory matching the current platform and architecture.
 *
 * Returns the directory that is the most specific to the currect architecture.
 */
function findArch(dir: string): string | null {
  let entries;
  try {
    entries = fs.readdirSync(dir, { withFileTypes: true });
  } catch (_e) {
    return null;
  }

  let pick: string | undefined;
  let specificity = 0x0fff_ffff;

  files: for (const file of entries) {
    if (!file.isDirectory()) {
      continue;
    }

    const split = file.name.split("-");
    if (split.length !== 2 || split[0] !== platform) {
      continue;
    }

    const archs = (split[1] as string).split("+");
    if (archs.length === 0) {
      continue;
    }

    // Check if the archs contains a match for the current architecture.
    let matches = false;
    for (const s of archs) {
      if (s === arch) {
        matches = true;
      } else if (s.length === 0) {
        // There should be no empty strings in the array.
        continue files;
      }
    }

    // Sort the archs by specificity, ie. prefer a single-arch build over a multi-arch build.
    if (matches && (!pick || archs.length < specificity)) {
      pick = file.name;
      specificity = archs.length;
    }
  }

  return pick ? path.join(dir, pick) : null;
}

function isElectron(): boolean {
  return (
    !!versions.electron ||
    !!process.env.ELECTRON_RUN_AS_NODE ||
    (typeof window !== "undefined" &&
      (window.process as { type?: string } | undefined)?.type === "renderer")
  );
}

function isAlpine() {
  return platform === "linux" && fs.existsSync("/etc/alpine-release");
}
