//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { defineEnum } from "../Ice/EnumBase"

const FormatType = defineEnum(
    [
        ['DefaultFormat', 0],
        ['CompactFormat', 1],
        ['SlicedFormat', 2]
    ]);

export { FormatType };
