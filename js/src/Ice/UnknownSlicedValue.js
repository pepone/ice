//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

import { Value } from "./Value";

class SliceInfo
{
    constructor()
    {
        //
        // The Slice type ID for this slice.
        //
        this.typeId = "";

        //
        // The Slice compact type ID for this slice.
        //
        this.compactId = -1;

        //
        // The encoded bytes for this slice, including the leading size integer.
        //
        this.bytes = [];

        //
        // The class instances referenced by this slice.
        //
        this.instances = [];

        //
        // Whether or not the slice contains optional members.
        //
        this.hasOptionalMembers = false;

        //
        // Whether or not this is the last slice.
        //
        this.isLastSlice = false;
    }
}

class SlicedData
{
    constructor(slices)
    {
        this.slices = slices;
    }
}

class UnknownSlicedValue extends Value
{
    constructor(unknownTypeId)
    {
        super();
        this._unknownTypeId = unknownTypeId;
    }

    ice_getSlicedData()
    {
        return this._slicedData;
    }

    ice_id()
    {
        return this._unknownTypeId;
    }

    _iceWrite(os)
    {
        os.startValue(this._slicedData);
        os.endValue();
    }

    _iceRead(is)
    {
        is.startValue();
        this._slicedData = is.endValue(true);
    }
}

export { SliceInfo, SlicedData, UnknownSlicedValue };
