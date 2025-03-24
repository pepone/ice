// Copyright (c) ZeroC, Inc.

class ControllerHelper {
    write(msg) {
        process.stdout.write(msg);
    }

    writeLine(msg) {
        process.stdout.write(msg + "\n");
    }

    serverReady() {
        console.log("server ready");
    }
}

(async function () {
    try {
        process.on("unhandledRejection", (reason, promise) => {
            console.error("Unhandled Rejection at:", promise, "reason:", reason);
            process.exit(1);
        });

        process.on("uncaughtException", err => {
            console.error("Uncaught Exception:", err);
            process.exit(1);
        });

        const path = process.argv[2];
        const name = process.argv[3];
        const module = await import(path);
        const cls = module[name];

        const test = new cls();
        test.setControllerHelper(new ControllerHelper());
        await test.run(process.argv);
    } catch (ex) {
        console.log(ex);
        process.exit(1);
    }
})();
