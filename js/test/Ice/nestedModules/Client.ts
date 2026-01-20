// Copyright (c) ZeroC, Inc.

import { TestHelper, test } from "../../Common/TestHelper.js";
import { Outer } from "./Test.js";

export class Client extends TestHelper {
    run() {
        const out = this.getWriter();

        out.write("testing nested modules from multiple files are merged correctly... ");

        // Test that types from First.ice are accessible through Outer.Inner
        // This verifies that the nested module merge preserves First from First.ice
        const first = new Outer.Inner.First(42);
        test(first.value === 42);

        // Test that types from Second.ice are accessible through Outer.Inner
        // This verifies that the nested module merge preserves Second from Second.ice
        const second = new Outer.Inner.Second("hello");
        test(second.name === "hello");

        // Test that types from Test.ice (which includes both) are accessible
        // This verifies that Combined can use both First and Second
        const combined = new Outer.Inner.Combined(first, second);
        test(combined.first.value === 42);
        test(combined.second.name === "hello");

        out.writeLine("ok");

        out.write("testing deep nested modules from multiple files are merged correctly... ");

        // Test that deep nested types from First.ice are accessible through Outer.Inner.Deep
        // This verifies that multi-level nested module merge works
        const deepFirst = new Outer.Inner.Deep.DeepFirst(100);
        test(deepFirst.deepValue === 100);

        // Test that deep nested types from Second.ice are accessible through Outer.Inner.Deep
        const deepSecond = new Outer.Inner.Deep.DeepSecond("deep");
        test(deepSecond.deepName === "deep");

        // Test that DeepCombined from Test.ice can use both DeepFirst and DeepSecond
        const deepCombined = new Outer.Inner.Deep.DeepCombined(deepFirst, deepSecond);
        test(deepCombined.deepFirst.deepValue === 100);
        test(deepCombined.deepSecond.deepName === "deep");

        out.writeLine("ok");
    }
}
