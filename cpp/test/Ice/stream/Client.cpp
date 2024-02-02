//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifdef _MSC_VER
#   pragma warning(disable:4244) // '=': conversion from 'int' to 'Ice::Short', possible loss of data
#endif

#include <Ice/Ice.h>
#include <TestHelper.h>
#include <Test.h>

using namespace std;
using namespace Test;
using namespace Test::Sub;
using namespace Test2::Sub2;

class TestObjectWriter : public Ice::ValueHelper<TestObjectWriter, Ice::Value>
{
public:

    TestObjectWriter(const MyClassPtr& p)
    {
        obj = p;
        called = false;
    }

    virtual void _iceWrite(Ice::OutputStream* out) const
    {
        obj->_iceWrite(out);
        const_cast<TestObjectWriter*>(this)->called = true;
    }

    virtual void _iceRead(Ice::InputStream*)
    {
        assert(false);
    }

    MyClassPtr obj;
    bool called;
};

class TestObjectReader : public Ice::ValueHelper<TestObjectReader, Ice::Value>
{
public:

    TestObjectReader()
    {
        called = false;
    }

    virtual void _iceWrite(Ice::OutputStream*) const
    {
        assert(false);
    }

    virtual void _iceRead(Ice::InputStream* in)
    {
        obj = std::make_shared<MyClass>();
        obj->_iceRead(in);
        called = true;
    }

    MyClassPtr obj;
    bool called;
};

// Required for ValueHelper<>'s _iceReadImpl and _iceWriteIpml
namespace Ice
{
template<class S>
struct StreamWriter<TestObjectWriter, S>
{
    static void write(S*, const TestObjectWriter&) { assert(false); }
};
template<class S>
struct StreamReader<TestObjectWriter, S>
{
    static void read(S*, TestObjectWriter&) { assert(false); }
};
template<class S>
struct StreamWriter<TestObjectReader, S>
{
    static void write(S*, const TestObjectReader&) { assert(false); }
};
template<class S>
struct StreamReader<TestObjectReader, S>
{
    static void read(S*, TestObjectReader&) { assert(false); }
};
}

void
patchObject(void* addr, const shared_ptr<Ice::Value>& v)
{
    shared_ptr<Ice::Value>* p = static_cast<shared_ptr<Ice::Value>*>(addr);
    assert(p);
    *p = v;
}

class MyClassFactoryWrapper
{
public:

    MyClassFactoryWrapper()
    {
        clear();
    }

    shared_ptr<Ice::Value> create(const string& type)
    {
        return _factory(type);
    }

    void setFactory(function<shared_ptr<Ice::Value>(const string&)> f)
    {
        _factory = f;
    }

    void clear()
    {
        _factory = [](const string&) { return make_shared<MyClass>(); };
    }

    function<shared_ptr<Ice::Value>(const string&)> _factory;
};

void
allTests(Test::TestHelper* helper)
{
    Ice::CommunicatorPtr communicator = helper->communicator();
    MyClassFactoryWrapper factoryWrapper;
    function<shared_ptr<Ice::Value>(const string&)> f =
        std::bind(&MyClassFactoryWrapper::create, &factoryWrapper, std::placeholders::_1);
    communicator->getValueFactoryManager()->add(f, MyClass::ice_staticId());

    vector<Ice::Byte> data;

    //
    // Test the stream API.
    //
    cout << "testing primitive types... " << flush;

    {
        vector<Ice::Byte> byte;
        Ice::InputStream in(communicator, byte);
    }

    {
        Ice::OutputStream out(communicator);
        out.startEncapsulation();
        out.write(true);
        out.endEncapsulation();
        out.finished(data);
        pair<const Ice::Byte*, const Ice::Byte*> d = out.finished();
        test(d.second - d.first == static_cast<int>(data.size()));
        test(vector<Ice::Byte>(d.first, d.second) == data);

        Ice::InputStream in(communicator, data);
        in.startEncapsulation();
        bool v;
        in.read(v);
        test(v);
        in.endEncapsulation();
    }

    {
        vector<Ice::Byte> byte;
        Ice::InputStream in(communicator, byte);
        try
        {
            bool v;
            in.read(v);
            test(false);
        }
        catch(const Ice::UnmarshalOutOfBoundsException&)
        {
        }
    }

    {
        Ice::OutputStream out(communicator);
        out.write(true);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        bool v;
        in.read(v);
        test(v);
    }

    {
        Ice::OutputStream out(communicator);
        out.write((Ice::Byte)1);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::Byte v;
        in.read(v);
        test(v == 1);
    }

    {
        Ice::OutputStream out(communicator);
        out.write((Ice::Short)2);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::Short v;
        in.read(v);
        test(v == 2);
    }

    {
        Ice::OutputStream out(communicator);
        out.write((Ice::Int)3);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::Int v;
        in.read(v);
        test(v == 3);
    }

    {
        Ice::OutputStream out(communicator);
        out.write((Ice::Long)4);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::Long v;
        in.read(v);
        test(v == 4);
    }

    {
        Ice::OutputStream out(communicator);
        out.write((Ice::Float)5.0);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::Float v;
        in.read(v);
        test(v == 5.0);
    }

    {
        Ice::OutputStream out(communicator);
        out.write((Ice::Double)6.0);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::Double v;
        in.read(v);
        test(v == 6.0);
    }

    {
        Ice::OutputStream out(communicator);
        out.write("hello world");
        out.finished(data);
        Ice::InputStream in(communicator, data);
        string v;
        in.read(v);
        test(v == "hello world");
    }

    cout << "ok" << endl;

    cout << "testing constructed types... " << flush;

    {
        Ice::OutputStream out(communicator);
        out.write(ICE_ENUM(MyEnum, enum3));
        out.finished(data);
        Ice::InputStream in(communicator, data);
        MyEnum e;
        in.read(e);
        test(e == ICE_ENUM(MyEnum, enum3));
    }

    {
        Ice::OutputStream out(communicator);
        SmallStruct s;
        s.bo = true;
        s.by = 1;
        s.sh = 2;
        s.i = 3;
        s.l = 4;
        s.f = 5.0;
        s.d = 6.0;
        s.str = "7";
        s.e = ICE_ENUM(MyEnum, enum2);
        s.p = ICE_UNCHECKED_CAST(MyInterfacePrx, communicator->stringToProxy("test:default"));
        out.write(s);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        SmallStruct s2;
        in.read(s2);
        test(targetEqualTo(s2.p, s.p));
        s2.p = s.p; // otherwise the s2 == s below will fail

        test(s2 == s);
    }

    {
        Ice::OutputStream out(communicator);
        OptionalClassPtr o = std::make_shared<OptionalClass>();
        o->bo = false;
        o->by = 5;
        o->sh = static_cast<Ice::Short>(4);
        o->i = 3;
        out.write(o);
        out.writePendingValues();
        out.finished(data);
        Ice::InputStream in(communicator, data);
        OptionalClassPtr o2;
        in.read(o2);
        in.readPendingValues();
        test(o2->bo == o->bo);
        test(o2->by == o->by);
        if(in.getEncoding() == Ice::Encoding_1_0)
        {
            test(!o2->sh);
            test(!o2->i);
        }
        else
        {
            test(o2->sh == o->sh);
            test(o2->i == o->i);
        }
    }

    {
        Ice::OutputStream out(communicator, Ice::Encoding_1_0);
        OptionalClassPtr o = std::make_shared<OptionalClass>();
        o->bo = false;
        o->by = 5;
        o->sh = static_cast<Ice::Short>(4);
        o->i = 3;
        out.write(o);
        out.writePendingValues();
        out.finished(data);
        Ice::InputStream in(communicator, Ice::Encoding_1_0, data);
        OptionalClassPtr o2;
        in.read(o2);
        in.readPendingValues();
        test(o2->bo == o->bo);
        test(o2->by == o->by);
        test(!o2->sh);
        test(!o2->i);
    }

    {
        Ice::BoolSeq arr;
        arr.push_back(true);
        arr.push_back(false);
        arr.push_back(true);
        arr.push_back(false);

        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);

        Ice::InputStream in(communicator, data);
        Ice::BoolSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        BoolSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::BoolSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        BoolSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        Ice::ByteSeq arr;
        arr.push_back(0x01);
        arr.push_back(0x11);
        arr.push_back(0x12);
        arr.push_back(0x22);

        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::ByteSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        ByteSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::ByteSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        ByteSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        Ice::ShortSeq arr;
        arr.push_back(0x01);
        arr.push_back(0x11);
        arr.push_back(0x12);
        arr.push_back(0x22);
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::ShortSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        ShortSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::ShortSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        ShortSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        Ice::IntSeq arr;
        arr.push_back(0x01);
        arr.push_back(0x11);
        arr.push_back(0x12);
        arr.push_back(0x22);
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::IntSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        IntSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::IntSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        IntSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        Ice::LongSeq arr;
        arr.push_back(0x01);
        arr.push_back(0x11);
        arr.push_back(0x12);
        arr.push_back(0x22);
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::LongSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        LongSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::LongSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        LongSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        Ice::FloatSeq arr;
        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);
        arr.push_back(4);
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::FloatSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        FloatSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::FloatSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        FloatSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        Ice::DoubleSeq arr;
        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);
        arr.push_back(4);
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::DoubleSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        DoubleSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::DoubleSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        DoubleSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        Ice::StringSeq arr;
        arr.push_back("string1");
        arr.push_back("string2");
        arr.push_back("string3");
        arr.push_back("string4");
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        Ice::StringSeq arr2;
        in.read(arr2);
        test(arr2 == arr);

        StringSS arrS;
        arrS.push_back(arr);
        arrS.push_back(Ice::StringSeq());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        StringSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        MyEnumS arr;
        arr.push_back(ICE_ENUM(MyEnum, enum3));
        arr.push_back(ICE_ENUM(MyEnum, enum2));
        arr.push_back(ICE_ENUM(MyEnum, enum1));
        arr.push_back(ICE_ENUM(MyEnum, enum2));

        Ice::OutputStream out(communicator);
        out.write(arr);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        MyEnumS arr2;
        in.read(arr2);
        test(arr2 == arr);

        MyEnumSS arrS;
        arrS.push_back(arr);
        arrS.push_back(MyEnumS());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        MyEnumSS arr2S;
        in2.read(arr2S);
        test(arr2S == arrS);
    }

    {
        SmallStructS arr;
        for(int i = 0; i < 4; ++i)
        {
            SmallStruct s;
            s.bo = true;
            s.by = 1;
            s.sh = 2;
            s.i = 3;
            s.l = 4;
            s.f = 5.0;
            s.d = 6.0;
            s.str = "7";
            s.e = ICE_ENUM(MyEnum, enum2);
            s.p = ICE_UNCHECKED_CAST(MyInterfacePrx, communicator->stringToProxy("test:default"));
            arr.push_back(s);
        }
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.writePendingValues();
        out.finished(data);
        Ice::InputStream in(communicator, data);
        SmallStructS arr2;
        in.read(arr2);
        in.readPendingValues();
        test(arr2.size() == arr.size());

        for(SmallStructS::size_type j = 0; j < arr2.size(); ++j)
        {
            test(targetEqualTo(arr[j].p, arr2[j].p));
            arr2[j].p = arr[j].p;
            test(arr[j] == arr2[j]);
        }

        SmallStructSS arrS;
        arrS.push_back(arr);
        arrS.push_back(SmallStructS());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        SmallStructSS arr2S;
        in2.read(arr2S);
    }

    {
        MyClassS arr;
        for(int i = 0; i < 4; ++i)
        {
            MyClassPtr c = std::make_shared<MyClass>();
            c->c = c;
            c->o = c;
            c->s.e = ICE_ENUM(MyEnum, enum2);

            c->seq1.push_back(true);
            c->seq1.push_back(false);
            c->seq1.push_back(true);
            c->seq1.push_back(false);

            c->seq2.push_back(1);
            c->seq2.push_back(2);
            c->seq2.push_back(3);
            c->seq2.push_back(4);

            c->seq3.push_back(1);
            c->seq3.push_back(2);
            c->seq3.push_back(3);
            c->seq3.push_back(4);

            c->seq4.push_back(1);
            c->seq4.push_back(2);
            c->seq4.push_back(3);
            c->seq4.push_back(4);

            c->seq5.push_back(1);
            c->seq5.push_back(2);
            c->seq5.push_back(3);
            c->seq5.push_back(4);

            c->seq6.push_back(1);
            c->seq6.push_back(2);
            c->seq6.push_back(3);
            c->seq6.push_back(4);

            c->seq7.push_back(1);
            c->seq7.push_back(2);
            c->seq7.push_back(3);
            c->seq7.push_back(4);

            c->seq8.push_back("string1");
            c->seq8.push_back("string2");
            c->seq8.push_back("string3");
            c->seq8.push_back("string4");

            c->seq9.push_back(ICE_ENUM(MyEnum, enum3));
            c->seq9.push_back(ICE_ENUM(MyEnum, enum2));
            c->seq9.push_back(ICE_ENUM(MyEnum, enum1));

            c->d["hi"] = c;
            arr.push_back(c);
        }
        Ice::OutputStream out(communicator);
        out.write(arr);
        out.writePendingValues();
        out.finished(data);

        Ice::InputStream in(communicator, data);
        MyClassS arr2;
        in.read(arr2);
        in.readPendingValues();
        test(arr2.size() == arr.size());
        for(MyClassS::size_type j = 0; j < arr2.size(); ++j)
        {
            test(arr2[j]);
            test(arr2[j]->c == arr2[j]);
            test(arr2[j]->o == arr2[j]);
            test(arr2[j]->s.e == ICE_ENUM(MyEnum, enum2));
            test(arr2[j]->seq1 == arr[j]->seq1);
            test(arr2[j]->seq2 == arr[j]->seq2);
            test(arr2[j]->seq3 == arr[j]->seq3);
            test(arr2[j]->seq4 == arr[j]->seq4);
            test(arr2[j]->seq5 == arr[j]->seq5);
            test(arr2[j]->seq6 == arr[j]->seq6);
            test(arr2[j]->seq7 == arr[j]->seq7);
            test(arr2[j]->seq8 == arr[j]->seq8);
            test(arr2[j]->seq9 == arr[j]->seq9);
            test(arr2[j]->d["hi"] == arr2[j]);
        }

        MyClassSS arrS;
        arrS.push_back(arr);
        arrS.push_back(MyClassS());
        arrS.push_back(arr);

        Ice::OutputStream out2(communicator);
        out2.write(arrS);
        out2.writePendingValues();
        out2.finished(data);

        Ice::InputStream in2(communicator, data);
        MyClassSS arr2S;
        in2.read(arr2S);
        in2.readPendingValues();
        test(arr2S.size() == arrS.size());
        test(arr2S[0].size() == arrS[0].size());
        test(arr2S[1].size() == arrS[1].size());
        test(arr2S[2].size() == arrS[2].size());
        for(size_t j = 0; j < arr2S.size(); ++j)
        {
            for(size_t k = 0; k < arr2S[j].size(); ++k)
            {
                test(arr2S[j][k]->c == arr2S[j][k]);
                test(arr2S[j][k]->o == arr2S[j][k]);
                test(arr2S[j][k]->s.e == ICE_ENUM(MyEnum, enum2));
                test(arr2S[j][k]->seq1 == arr[k]->seq1);
                test(arr2S[j][k]->seq2 == arr[k]->seq2);
                test(arr2S[j][k]->seq3 == arr[k]->seq3);
                test(arr2S[j][k]->seq4 == arr[k]->seq4);
                test(arr2S[j][k]->seq5 == arr[k]->seq5);
                test(arr2S[j][k]->seq6 == arr[k]->seq6);
                test(arr2S[j][k]->seq7 == arr[k]->seq7);
                test(arr2S[j][k]->seq8 == arr[k]->seq8);
                test(arr2S[j][k]->seq9 == arr[k]->seq9);
                test(arr2S[j][k]->d["hi"] == arr2S[j][k]);
            }
        }

        auto clearS = [](MyClassS& arr3) {
            for(MyClassS::iterator p = arr3.begin(); p != arr3.end(); ++p)
            {
                if(*p)
                {
                    (*p)->c = nullptr;
                    (*p)->o = nullptr;
                    (*p)->d["hi"] = nullptr;
                }
            }
        };
        auto clearSS = [clearS](MyClassSS& arr3) {
            for(MyClassSS::iterator p = arr3.begin(); p != arr3.end(); ++p)
            {
                clearS(*p);
            }
        };
        clearS(arr);
        clearS(arr2);
        clearSS(arrS);
        clearSS(arr2S);
    }

    {
        Ice::OutputStream out(communicator);
        MyClassPtr obj = std::make_shared<MyClass>();
        obj->s.e = ICE_ENUM(MyEnum, enum2);
        shared_ptr<TestObjectWriter> writer = make_shared<TestObjectWriter>(obj);
        out.write(writer);
        out.writePendingValues();
        out.finished(data);
        test(writer->called);
    }

    {
        Ice::OutputStream out(communicator);
        MyClassPtr obj = std::make_shared<MyClass>();
        obj->s.e = ICE_ENUM(MyEnum, enum2);
        shared_ptr<TestObjectWriter> writer = make_shared<TestObjectWriter>(obj);
        out.write(writer);
        out.writePendingValues();
        out.finished(data);
        test(writer->called);
        factoryWrapper.setFactory([](const string&) { return make_shared<TestObjectReader>(); });
        Ice::InputStream in(communicator, data);
        shared_ptr<Ice::Value> p;
        in.read(&patchObject, &p);
        in.readPendingValues();
        test(p);
        shared_ptr<TestObjectReader> reader = dynamic_pointer_cast<TestObjectReader>(p);
        test(reader);
        test(reader->called);
        test(reader->obj);
        test(reader->obj->s.e == ICE_ENUM(MyEnum, enum2));
        factoryWrapper.clear();
    }

    {
        Ice::OutputStream out(communicator);
        MyException ex;
        MyClassPtr c = std::make_shared<MyClass>();
        c->c = c;
        c->o = c;
        c->s.e = ICE_ENUM(MyEnum, enum2);

        c->seq1.push_back(true);
        c->seq1.push_back(false);
        c->seq1.push_back(true);
        c->seq1.push_back(false);

        c->seq2.push_back(1);
        c->seq2.push_back(2);
        c->seq2.push_back(3);
        c->seq2.push_back(4);

        c->seq3.push_back(1);
        c->seq3.push_back(2);
        c->seq3.push_back(3);
        c->seq3.push_back(4);

        c->seq4.push_back(1);
        c->seq4.push_back(2);
        c->seq4.push_back(3);
        c->seq4.push_back(4);

        c->seq5.push_back(1);
        c->seq5.push_back(2);
        c->seq5.push_back(3);
        c->seq5.push_back(4);

        c->seq6.push_back(1);
        c->seq6.push_back(2);
        c->seq6.push_back(3);
        c->seq6.push_back(4);

        c->seq7.push_back(1);
        c->seq7.push_back(2);
        c->seq7.push_back(3);
        c->seq7.push_back(4);

        c->seq8.push_back("string1");
        c->seq8.push_back("string2");
        c->seq8.push_back("string3");
        c->seq8.push_back("string4");

        c->seq9.push_back(ICE_ENUM(MyEnum, enum3));
        c->seq9.push_back(ICE_ENUM(MyEnum, enum2));
        c->seq9.push_back(ICE_ENUM(MyEnum, enum1));

        ex.c = c;

        out.write(ex);
        out.finished(data);

        Ice::InputStream in(communicator, data);
        try
        {
            in.throwException();
            test(false);
        }
        catch(const MyException& ex1)
        {
            test(ex1.c->s.e == c->s.e);
            test(ex1.c->seq1 == c->seq1);
            test(ex1.c->seq2 == c->seq2);
            test(ex1.c->seq3 == c->seq3);
            test(ex1.c->seq4 == c->seq4);
            test(ex1.c->seq5 == c->seq5);
            test(ex1.c->seq6 == c->seq6);
            test(ex1.c->seq7 == c->seq7);
            test(ex1.c->seq8 == c->seq8);
            test(ex1.c->seq9 == c->seq9);

            ex1.c->c = nullptr;
            ex1.c->o = nullptr;
        }

        c->c = nullptr;
        c->o = nullptr;
    }

    {
        ByteBoolD dict;
        dict[0x04] = true;
        dict[0x01] = false;
        Ice::OutputStream out(communicator);
        out.write(dict);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        ByteBoolD dict2;
        in.read(dict2);
        test(dict2 == dict);
    }

    {
        ShortIntD dict;
        dict[1] = 9;
        dict[4] = 8;
        Ice::OutputStream out(communicator);
        out.write(dict);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        ShortIntD dict2;
        in.read(dict2);
        test(dict2 == dict);
    }

    {
        LongFloatD dict;
        dict[123809828] = 0.51f;
        dict[123809829] = 0.56f;
        Ice::OutputStream out(communicator);
        out.write(dict);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        LongFloatD dict2;
        in.read(dict2);
        test(dict2 == dict);
    }

    {
        StringStringD dict;
        dict["key1"] = "value1";
        dict["key2"] = "value2";
        Ice::OutputStream out(communicator);
        out.write(dict);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        StringStringD dict2;
        in.read(dict2);
        test(dict2 == dict);
    }

    {
        StringMyClassD dict;
        dict["key1"] = std::make_shared<MyClass>();
        dict["key1"]->s.e = ICE_ENUM(MyEnum, enum2);
        dict["key2"] = std::make_shared<MyClass>();
        dict["key2"]->s.e = ICE_ENUM(MyEnum, enum3);
        Ice::OutputStream out(communicator);
        out.write(dict);
        out.writePendingValues();
        out.finished(data);
        Ice::InputStream in(communicator, data);
        StringMyClassD dict2;
        in.read(dict2);
        in.readPendingValues();
        test(dict2.size() == dict.size());
        test(dict2["key1"] && (dict2["key1"]->s.e == ICE_ENUM(MyEnum, enum2)));
        test(dict2["key2"] && (dict2["key2"]->s.e == ICE_ENUM(MyEnum, enum3)));
    }

    {
        Ice::OutputStream out(communicator);
        out.write(ICE_ENUM(Sub::NestedEnum, nestedEnum3));
        out.finished(data);
        Ice::InputStream in(communicator, data);
        NestedEnum e;
        in.read(e);
        test(e == ICE_ENUM(Sub::NestedEnum, nestedEnum3));
    }

    {
        Ice::OutputStream out(communicator);
        NestedStruct s;
        s.bo = true;
        s.by = 1;
        s.sh = 2;
        s.i = 3;
        s.l = 4;
        s.f = 5.0;
        s.d = 6.0;
        s.str = "7";
        s.e = ICE_ENUM(Sub::NestedEnum, nestedEnum2);
        out.write(s);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        NestedStruct s2;
        in.read(s2);
        test(s2 == s);
    }

    {
        Ice::OutputStream out(communicator);
        NestedException ex;
        ex.str = "str";

        out.write(ex);
        out.finished(data);

        Ice::InputStream in(communicator, data);
        try
        {
            in.throwException();
            test(false);
        }
        catch(const NestedException& ex1)
        {
            test(ex1.str == ex.str);
        }
    }

    {
        Ice::OutputStream out(communicator);
        out.write(ICE_ENUM(NestedEnum2, nestedEnum4));
        out.finished(data);
        Ice::InputStream in(communicator, data);
        NestedEnum2 e;
        in.read(e);
        test(e == ICE_ENUM(NestedEnum2, nestedEnum4));
    }

    {
        Ice::OutputStream out(communicator);
        NestedStruct2 s;
        s.bo = true;
        s.by = 1;
        s.sh = 2;
        s.i = 3;
        s.l = 4;
        s.f = 5.0;
        s.d = 6.0;
        s.str = "7";
        s.e = ICE_ENUM(NestedEnum2, nestedEnum5);
        out.write(s);
        out.finished(data);
        Ice::InputStream in(communicator, data);
        NestedStruct2 s2;
        in.read(s2);
        test(s2 == s);
    }

    {
        Ice::OutputStream out(communicator);
        NestedException2 ex;
        ex.str = "str";

        out.write(ex);
        out.finished(data);

        Ice::InputStream in(communicator, data);
        try
        {
            in.throwException();
            test(false);
        }
        catch(const NestedException2& ex1)
        {
            test(ex1.str == ex.str);
        }
    }

    //
    // Test marshaling to user-supplied buffer.
    //
    {
        Ice::Byte buf[128];
        pair<Ice::Byte*, Ice::Byte*> p(&buf[0], &buf[0] + sizeof(buf));
        Ice::OutputStream out(communicator, Ice::currentEncoding, p);
        vector<Ice::Byte> v;
        v.resize(127);
        out.write(v);
        test(out.pos() == 128); // 127 bytes + leading size (1 byte)
        test(out.b.begin() == buf); // Verify the stream hasn't reallocated.
    }
    {
        Ice::Byte buf[128];
        pair<Ice::Byte*, Ice::Byte*> p(&buf[0], &buf[0] + sizeof(buf));
        Ice::OutputStream out(communicator, Ice::currentEncoding, p);
        vector<Ice::Byte> v;
        v.resize(127);
        ::memset(&v[0], 0xFF, v.size());
        out.write(v);
        out.write(Ice::Byte(0xFF)); // This extra byte should make the stream reallocate.
        test(out.pos() == 129); // 127 bytes + leading size (1 byte) + 1 byte
        test(out.b.begin() != buf); // Verify the stream was reallocated.
        out.finished(data);

        Ice::InputStream in(communicator, data);
        vector<Ice::Byte> v2;
        in.read(v2);
        test(v2.size() == 127);
        test(v == v2); // Make sure the original buffer was preserved.
    }

    cout << "ok" << endl;
}

class Client : public Test::TestHelper
{
public:

    void run(int, char**);
};

void
Client::run(int argc, char** argv)
{
    Ice::PropertiesPtr properties = createTestProperties(argc, argv);
    properties->setProperty("Ice.AcceptClassCycles", "1");
    Ice::CommunicatorHolder communicator = initialize(argc, argv, properties);
    void allTests(Test::TestHelper*);
    allTests(this);
}

DEFINE_TEST(Client)
