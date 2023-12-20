//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

/* eslint no-sync: "off" */
/* eslint no-process-env: "off" */
/* eslint no-process-exit: "off" */

const bundle = require("./gulp/bundle");

const del = require("del");
const extreplace = require("gulp-ext-replace");
const fs = require("fs");
const gulp = require("gulp");
const gzip = require('gulp-gzip');
const iceBuilder = require('gulp-ice-builder');
const tsbundle = require("gulp-ice-builder").tsbundle;
const newer = require('gulp-newer');
const path = require('path');
const paths = require('vinyl-paths');
const pump = require('pump');
const rollup = require('rollup');
const sourcemaps = require('gulp-sourcemaps');
const terser = require('gulp-terser');
const tsc = require('gulp-typescript');
const rename = require('gulp-rename');

const sliceDir = path.resolve(__dirname, '..', 'slice');

const iceBinDist = (process.env.ICE_BIN_DIST || "").split(" ");
const useBinDist = iceBinDist.find(v => v == "js" || v == "all") !== undefined;

function parseArg(argv, key)
{
    for(let i = 0; i < argv.length; ++i)
    {
        const e = argv[i];
        if(e == key)
        {
            return argv[i + 1];
        }
        else if(e.indexOf(key + "=") === 0)
        {
            return e.substr(key.length + 1);
        }
    }
}

const platform = parseArg(process.argv, "--cppPlatform") || process.env.CPP_PLATFORM;
const configuration = parseArg(process.argv, "--cppConfiguration") || process.env.CPP_CONFIGURATION;

function slice2js(options)
{
    const defaults = {};
    const opts = options || {};
    if(!useBinDist)
    {
        if(process.platform == "win32")
        {
            if(!platform || (platform.toLowerCase() != "win32" && platform.toLowerCase() != "x64"))
            {
                console.log("Error: --cppPlatform must be set to `Win32' or `x64', in order to locate slice2js.exe");
                process.exit(1);
            }

            if(!configuration || (configuration.toLowerCase() != "debug" && configuration.toLowerCase() != "release"))
            {
                console.log("Error: --cppConfiguration must be set to `Debug' or `Release', in order to locate slice2js.exe");
                process.exit(1);
            }
            defaults.iceToolsPath = path.resolve("../cpp/bin", platform, configuration);
        }
        defaults.iceHome = path.resolve(__dirname, '..');
    }
    else if(process.env.ICE_HOME)
    {
        defaults.iceHome = process.env.ICE_HOME;
    }
    defaults.include = opts.include || [];
    defaults.args = opts.args || [];
    defaults.jsbundle = opts.jsbundle;
    defaults.tsbundle = opts.tsbundle;
    defaults.jsbundleFormat = opts.jsbundleFormat;
    return iceBuilder(defaults);
}

//
// Tasks to build IceJS Distribution
//
const root = path.resolve(__dirname);
const libs = ["Ice", "Glacier2", "IceStorm", "IceGrid"];

const generateTask = name => name.toLowerCase() + ":generate";
const libTask = name => name.toLowerCase() + ":lib";
const minLibTask = name => name.toLowerCase() + ":lib-min";
const libDistTask = name => name.toLowerCase() + ":dist";

const libFile = name => path.join(root, "lib", name + ".js");
const libFileMin = name => path.join(root, "lib", name + ".min.js");

const srcDir = name => path.join(root, "src", name);
const libCleanTask = lib => lib + ":clean";

function libFiles(name)
{
    return [
        path.join(root, "lib", name + ".js"),
        path.join(root, "lib", name + ".js.gz"),
        path.join(root, "lib", name + ".min.js"),
        path.join(root, "lib", name + ".min.js.gz")];
}

function mapFiles(name)
{
    return [
        path.join(root, "lib", name + ".js.map"),
        path.join(root, "lib", name + ".js.map.gz"),
        path.join(root, "lib", name + ".min.js.map"),
        path.join(root, "lib", name + ".min.js.map.gz")];
}

function libSources(lib, sources)
{
    let srcs = sources.common || [];

    srcs = srcs.map(f => path.join(srcDir(lib), f));

    if(sources.slice)
    {
        srcs = srcs.concat(sources.slice.map(f => path.join(srcDir(lib), path.basename(f, ".ice") + ".js")));
    }
    return srcs;
}

function libGeneratedFiles(lib, sources)
{
    const tsSliceSources = sources.typescriptSlice || sources.slice;

    return sources.slice.map(f => path.join(srcDir(lib), path.basename(f, ".ice") + ".js"))
        .concat(tsSliceSources.map(f => path.join(srcDir(lib), path.basename(f, ".ice") + ".d.ts")))
        .concat(libFiles(lib))
        .concat(mapFiles(lib))
        .concat([path.join(srcDir(lib), ".depend", "*")]);
}

const sliceFile = f => path.join(sliceDir, f);

for(const lib of libs)
{
    const sources = JSON.parse(fs.readFileSync(path.join(srcDir(lib), "sources.json"), {encoding: "utf8"}));

    gulp.task(generateTask(lib),
              cb =>
              {
                  pump([gulp.src(sources.slice.map(sliceFile)),
                        slice2js(
                            {
                                jsbundle: false,
                                tsbundle: false,
                                args: ["--typescript"]
                            }),
                        gulp.dest(srcDir(lib))], cb);
              });

    gulp.task(libTask(lib),
              cb =>
              {
                  pump([gulp.src(libSources(lib, sources)),
                        sourcemaps.init(),
                        bundle(
                            {
                                srcDir: srcDir(lib),
                                modules: sources.modules,
                                target: libFile(lib)
                            }),
                        sourcemaps.write("../lib", {sourceRoot: "/src", addComment: false}),
                        gulp.dest("lib"),
                        gzip(),
                        gulp.dest("lib")], cb);
              });

    gulp.task(minLibTask(lib),
              cb =>
              {
                  pump([gulp.src(libFile(lib)),
                        newer(libFileMin(lib)),
                        sourcemaps.init({loadMaps: false}),
                        terser(),
                        extreplace(".min.js"),
                        sourcemaps.write(".", {includeContent: false, addComment: false}),
                        gulp.dest(path.join(root, "lib")),
                        gzip(),
                        gulp.dest(path.join(root, "lib"))], cb);
              });

    gulp.task(libCleanTask(lib),
              cb =>
              {
                  del(libGeneratedFiles(lib, sources));
                  cb();
              });

    gulp.task(libDistTask(lib),
              gulp.series(
                  generateTask(lib),
                  libTask(lib),
                  minLibTask(lib)));
}

gulp.task("ts:bundle",
          cb =>
          {
              pump([gulp.src(libs.map(lib => path.join(root, "src", lib, "*.d.ts"))),
                    tsbundle(),
                    rename("index.d.ts"),
                    gulp.dest("src")], cb);
          });

gulp.task("ts:bundle:clean", () => del("./src/index.d.ts"));

if(useBinDist)
{
    gulp.task("ice:module", cb => cb());
    gulp.task("ice:module:clean", cb => cb());
    gulp.task("dist", cb => cb());
    gulp.task("dist:clean", cb => cb());
}
else
{
    gulp.task("dist", gulp.series(gulp.parallel(libs.map(libDistTask)), "ts:bundle"));

    gulp.task("dist:clean", gulp.parallel(libs.map(libCleanTask).concat("ts:bundle:clean")));

    gulp.task("ice:module:package",
              () => gulp.src(['package.json']).pipe(gulp.dest(path.join("node_modules", "ice"))));

    gulp.task("ice:module",
              gulp.series("ice:module:package",
                          cb =>
                          {
                              pump([
                                  gulp.src([path.join(root, 'src/**/*')]),
                                  gulp.dest(path.join(root, "node_modules", "ice", "src"))], cb);
                          }));

    gulp.task("ice:module:clean", () => gulp.src(['node_modules/ice'], {allowEmpty: true}).pipe(paths(del)));
}

const tests = [
    "test/Ice/acm",
    "test/Ice/adapterDeactivation",
    "test/Ice/ami",
    "test/Ice/binding",
    "test/Ice/defaultValue",
    "test/Ice/enums",
    "test/Ice/exceptions",
    "test/Ice/facets",
    "test/Ice/hold",
    "test/Ice/info",
    "test/Ice/inheritance",
    "test/Ice/location",
    "test/Ice/objects",
    "test/Ice/operations",
    "test/Ice/optional",
    "test/Ice/promise",
    "test/Ice/properties",
    "test/Ice/proxy",
    "test/Ice/retry",
    "test/Ice/servantLocator",
    "test/Ice/slicing/exceptions",
    "test/Ice/slicing/objects",
    "test/Ice/stream",
    "test/Ice/timeout",
    "test/Ice/number",
    "test/Ice/scope",
    "test/Glacier2/router",
    "test/Slice/escape",
    "test/Slice/macros"
];

gulp.task("test:common:generate",
          cb =>
          {
              pump([gulp.src(["../scripts/Controller.ice"]),
                    slice2js(),
                    gulp.dest("test/Common")], cb);
          });

gulp.task("test:common:babel",
          cb =>
          {
              pump([gulp.src(["test/Common/Controller.js",
                              "test/Common/ControllerI.js",
                              "test/Common/ControllerWorker.js",
                              "test/Common/TestHelper.js",
                              "test/Common/run.js"]),
                    babel({compact: false})], cb);
          });

gulp.task("test:common:clean",
          cb =>
          {
              del(["test/Common/Controller.js",
                   "test/Common/.depend"]);
              cb();
          });

gulp.task("test:import:generate",
          cb =>
          {
              pump([gulp.src(["test/Ice/import/Demo/Point.ice",
                              "test/Ice/import/Demo/Circle.ice",
                              "test/Ice/import/Demo/Square.ice",
                              "test/Ice/import/Demo/Canvas.ice"]),
                    slice2js(
                        {
                            include: ["test/Ice/import"]
                        }),
                    gulp.dest("test/Ice/import/Demo")], cb);
          });

gulp.task("test:import:bundle",
          () =>
          {
              const p = rollup.rollup(
                  {
                      input: "test/Ice/import/main.js",
                      external: ["ice"]
                  }).then(bundle => bundle.write(
                      {
                          file: "test/Ice/import/bundle.js",
                          format: "cjs"
                      }));
              return p;
          });

gulp.task("test:import:clean",
          cb =>
          {
              del(["test/Ice/import/Demo/Point.js",
                   "test/Ice/import/Demo/Circle.js",
                   "test/Ice/import/Demo/Square.js",
                   "test/Ice/import/Demo/Canvas.js",
                   "test/Ice/import/bundle.js"]);
              cb();
          });

const testTask = name => name.replace(/\//g, "_");
const testBabelTask = name => testTask(name) + ":babel";
const testCleanTask = name => testTask(name) + ":clean";
const testBuildTask = name => testTask(name) + ":build";

for(const name of tests)
{
    gulp.task(testBuildTask(name),
              cb =>
              {
                  const outdir = path.join(root, name);
                  pump([gulp.src(path.join(outdir, "*.ice")),
                        slice2js(
                            {
                                include: [outdir]
                            }),
                        gulp.dest(outdir)], cb);
              });

    gulp.task(testCleanTask(name),
              cb =>
              {
                  pump([gulp.src(path.join(name, "*.ice")),
                        extreplace(".js"),
                        gulp.src(path.join(name, ".depend"), {allowEmpty: true}),
                        paths(del)], cb);
              });
}

gulp.task(
    "test",
    gulp.series("test:common:generate", "test:import:generate", "test:import:bundle",
        gulp.parallel(tests.map(testBuildTask))));

gulp.task(
    "test:clean",
    gulp.parallel("test:common:clean", "test:import:clean", tests.map(testCleanTask)));

//
// TypeScript tests
//
const tstests = [
    "test/typescript/Ice/acm",
    "test/typescript/Ice/adapterDeactivation",
    "test/typescript/Ice/ami",
    "test/typescript/Ice/binding",
    "test/typescript/Ice/defaultValue",
    "test/typescript/Ice/enums",
    "test/typescript/Ice/exceptions",
    "test/typescript/Ice/facets",
    "test/typescript/Ice/hold",
    "test/typescript/Ice/info",
    "test/typescript/Ice/inheritance",
    "test/typescript/Ice/location",
    "test/typescript/Ice/number",
    "test/typescript/Ice/objects",
    "test/typescript/Ice/operations",
    "test/typescript/Ice/optional",
    "test/typescript/Ice/properties",
    "test/typescript/Ice/proxy",
    "test/typescript/Ice/retry",
    "test/typescript/Ice/scope",
    "test/typescript/Ice/servantLocator",
    "test/typescript/Ice/slicing/exceptions",
    "test/typescript/Ice/slicing/objects",
    "test/typescript/Ice/stream",
    "test/typescript/Ice/timeout",
    "test/typescript/Glacier2/router",
    "test/typescript/Slice/macros"
];

const testTypeScriptSliceCompileJsTask = name => testTask(name) + ":ts:slice-compile-js";
const testTypeScriptCompileTask = name => testTask(name) + ":ts:compile";
const testTypeScriptBuildTask = name => testTask(name) + ":ts:build";
const testTypeScriptCleanTask = name => testTask(name) + ":ts:clean";

for(const name of tstests)
{
    gulp.task(testTypeScriptSliceCompileJsTask(name),
              cb =>
              {
                  const outdir = path.join(root, name);
                  pump([gulp.src(path.join(outdir, "*.ice")),
                        slice2js(
                            {
                                include: [outdir],
                                args: ["--typescript"],
                                jsbundleFormat: "cjs"
                            }),
                        gulp.dest(outdir)], cb);
              });

    gulp.task(testTypeScriptCompileTask(name),
              cb =>
              {
                  pump([gulp.src(path.join(root, name, "*.ts")),
                        tsc(
                            {
                                lib: ["dom", "es2017"],
                                target: "es2017",
                                module: "commonjs",
                                noImplicitAny: true
                            }),
                        gulp.dest(path.join(root, name))
                       ], cb);
              });

    gulp.task(testTypeScriptBuildTask(name),
              gulp.series(
                  testTypeScriptSliceCompileJsTask(name),
                  testTypeScriptCompileTask(name)));

    gulp.task(testTypeScriptCleanTask(name),
              cb =>
              {
                  pump([gulp.src([path.join(root, name, "**/*.js"),
                                  path.join(root, name, "**/*.d.ts"),
                                  path.join(root, name, "**/*.js.map")]),
                        paths(del)], cb);
              });
}

gulp.task("test:ts", gulp.series(tstests.map(testTypeScriptBuildTask)));
gulp.task("test:ts:clean", gulp.parallel(tstests.map(testTypeScriptCleanTask)));

gulp.task("build", gulp.series("dist", "ice:module", "test", "test:ts"));
gulp.task("clean", gulp.series("dist:clean", "ice:module:clean", "test:clean", "test:ts:clean"));
gulp.task("default", gulp.series("build"));
