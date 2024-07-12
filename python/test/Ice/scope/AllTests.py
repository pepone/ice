#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

import sys
import Test
import Inner


def test(b):
    if not b:
        raise RuntimeError("test assertion failed")


def allTests(helper, communicator):
    sys.stdout.write("test using same type name in different Slice modules... ")
    sys.stdout.flush()
    i1 = Test.IPrx(communicator, f"i1:{helper.getTestEndpoint()}")

    s1 = Test.S(0)

    s2, s3 = i1.opS(s1)

    test(s2 == s1)
    test(s3 == s1)

    sseq1 = [s1]

    sseq2, sseq3 = i1.opSSeq(sseq1)

    test(sseq2[0] == s1)
    test(sseq3[0] == s1)

    smap1 = {"a": s1}
    smap2, smap3 = i1.opSMap(smap1)
    test(smap2["a"] == s1)
    test(smap3["a"] == s1)

    c1 = Test.C(s1)

    c2, c3 = i1.opC(c1)

    test(c2.s == s1)
    test(c3.s == s1)

    cseq1 = [c1]
    cseq2, cseq3 = i1.opCSeq(cseq1)

    test(cseq2[0].s == s1)
    test(cseq3[0].s == s1)

    cmap1 = {"a": c1}
    cmap2, cmap3 = i1.opCMap(cmap1)
    test(cmap2["a"].s == s1)
    test(cmap3["a"].s == s1)

    e = i1.opE1(Test.E1.v1)
    test(e == Test.E1.v1)

    s = i1.opS1(Test.S1("S1"))
    test(s.s == "S1")

    c = i1.opC1(Test.C1("C1"))
    test(c.s == "C1")

    i2 = Test.Inner.Inner2.IPrx(communicator, f"i2:{helper.getTestEndpoint()}")

    s1 = Test.Inner.Inner2.S(0)

    s2, s3 = i2.opS(s1)

    test(s2 == s1)
    test(s3 == s1)

    sseq1 = [s1]

    sseq2, sseq3 = i2.opSSeq(sseq1)

    test(sseq2[0] == s1)
    test(sseq3[0] == s1)

    smap1 = {"a": s1}
    smap2, smap3 = i2.opSMap(smap1)
    test(smap2["a"] == s1)
    test(smap3["a"] == s1)

    c1 = Test.Inner.Inner2.C(s1)

    c2, c3 = i2.opC(c1)

    test(c2.s == s1)
    test(c3.s == s1)

    cseq1 = [c1]
    cseq2, cseq3 = i2.opCSeq(cseq1)

    test(cseq2[0].s == s1)
    test(cseq3[0].s == s1)

    cmap1 = {"a": c1}
    cmap2, cmap3 = i2.opCMap(cmap1)
    test(cmap2["a"].s == s1)
    test(cmap3["a"].s == s1)

    i3 = Test.Inner.IPrx(communicator, f"i3:{helper.getTestEndpoint()}")

    s1 = Test.Inner.Inner2.S(0)

    s2, s3 = i3.opS(s1)

    test(s2 == s1)
    test(s3 == s1)

    sseq1 = [s1]

    sseq2, sseq3 = i3.opSSeq(sseq1)

    test(sseq2[0] == s1)
    test(sseq3[0] == s1)

    smap1 = {"a": s1}
    smap2, smap3 = i3.opSMap(smap1)
    test(smap2["a"] == s1)
    test(smap3["a"] == s1)

    c1 = Test.Inner.Inner2.C(s1)

    c2, c3 = i3.opC(c1)

    test(c2.s == s1)
    test(c3.s == s1)

    cseq1 = [c1]
    cseq2, cseq3 = i3.opCSeq(cseq1)

    test(cseq2[0].s == s1)
    test(cseq3[0].s == s1)

    cmap1 = {"a": c1}
    cmap2, cmap3 = i3.opCMap(cmap1)
    test(cmap2["a"].s == s1)
    test(cmap3["a"].s == s1)

    i4 = Inner.Test.Inner2.IPrx(communicator, f"i4:{helper.getTestEndpoint()}")

    s1 = Test.S(0)

    s2, s3 = i4.opS(s1)

    test(s2 == s1)
    test(s3 == s1)

    sseq1 = [s1]

    sseq2, sseq3 = i4.opSSeq(sseq1)

    test(sseq2[0] == s1)
    test(sseq3[0] == s1)

    smap1 = {"a": s1}
    smap2, smap3 = i4.opSMap(smap1)
    test(smap2["a"] == s1)
    test(smap3["a"] == s1)

    c1 = Test.C(s1)

    c2, c3 = i4.opC(c1)

    test(c2.s == s1)
    test(c3.s == s1)

    cseq1 = [c1]
    cseq2, cseq3 = i4.opCSeq(cseq1)

    test(cseq2[0].s == s1)
    test(cseq3[0].s == s1)

    cmap1 = {"a": c1}
    cmap2, cmap3 = i4.opCMap(cmap1)
    test(cmap2["a"].s == s1)
    test(cmap3["a"].s == s1)

    i1.shutdown()
    print("ok")
