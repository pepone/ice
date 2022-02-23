//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { defineEnum } from "./EnumBase";

const ToStringMode = defineEnum(
    [
        ['Unicode', 0],
        ['ASCII', 1],
        ['Compat', 2]
    ]);

export { ToStringMode };