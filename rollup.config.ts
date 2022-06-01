import type { Plugin, RollupOptions, TreeshakingOptions } from "rollup";
import type { RollupAliasOptions } from "@rollup/plugin-alias";

import alias from "@rollup/plugin-alias";
import commonjs from "@rollup/plugin-commonjs";
import json from "@rollup/plugin-json";
import { nodeResolve } from "@rollup/plugin-node-resolve";
import typescript from "@rollup/plugin-typescript";
import { terser } from "rollup-plugin-terser";

const aliases: RollupAliasOptions = {
  entries: {},
};

const nodePlugins: Plugin[] = [
  alias(aliases),
  nodeResolve({ preferBuiltins: true }),
  json(),
  commonjs({
    ignoreTryCatch: false,
    include: "node_modules/**",
    dynamicRequireTargets: "package.json",
  }),
  typescript({ removeComments: true }),
];

const treeshake: TreeshakingOptions = {
  moduleSideEffects: false,
  propertyReadSideEffects: false,
  tryCatchDeoptimization: false,
};

export default async function (
  _command: Record<string, unknown>,
): Promise<RollupOptions | RollupOptions[]> {
  const cjsBuild: RollupOptions = {
    input: "src/node/index.ts",
    output: {
      dir: "dist",
      exports: "auto",
      format: "commonjs",
      interop: "default",
      sourcemap: true,
    },
    plugins: [...nodePlugins],
    treeshake,
  };

  const esmBuild: RollupOptions = {
    ...cjsBuild,
    output: {
      ...cjsBuild.output,
      dir: "dist/esm",
      format: "esm",
      sourcemap: false,
    },
    plugins: [...nodePlugins],
  };

  const browserBuild: RollupOptions = {
    input: "src/browser.ts",
    output: {
      file: "dist/esm/index.browser.js",
      format: "esm",
    },
    plugins: [
      alias(aliases),
      nodeResolve({ browser: true }),
      commonjs(),
      typescript({ removeComments: true }),
      terser({ module: true, ecma: 2020 }),
    ],
    treeshake,
  };

  return [cjsBuild, esmBuild, browserBuild];
}
