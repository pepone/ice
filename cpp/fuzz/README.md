# Ice C++ fuzz targets

These [libFuzzer](https://llvm.org/docs/LibFuzzer.html) targets feed arbitrary bytes to the parts of the C++ runtime
that parse data received from a peer:

| Target | Input |
| --- | --- |
| `ice_http_parser_fuzzer` | A WebSocket upgrade request or response, parsed by `HttpParser` |
| `ice_base64_fuzzer` | A Base64 string, such as the `Sec-WebSocket-Key` header, decoded by `Base64::decode` |
| `ice_distinguished_name_fuzzer` | The subject name of a peer's certificate, parsed as an RFC 2253 distinguished name |

Each target is built from `<Name>Fuzzer.cpp`, and `corpus/<name>` holds its seed inputs. The files in `corpus` are raw
bytes: they are excluded from end-of-line conversion and from the whitespace checks.

## Building and running

`oss-fuzz-build.sh` builds the targets with the compiler and flags in `CC`, `CXX`, `CFLAGS`, `CXXFLAGS` and
`LIB_FUZZING_ENGINE`, and writes them, with their seed corpora, to `OUT`. It is the build [OSS-Fuzz] runs, and it works
the same on a Linux machine with clang and the C++ build dependencies installed:

```shell
export CC=clang CXX=clang++
export CFLAGS="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined,fuzzer-no-link"
export CXXFLAGS="$CFLAGS"
export LIB_FUZZING_ENGINE=-fsanitize=fuzzer
export OUT=/tmp/ice-fuzz
mkdir -p $OUT
cpp/fuzz/oss-fuzz-build.sh
```

Then run a target on a copy of its seed corpus, which libFuzzer extends with the new inputs it finds:

```shell
cd /tmp/ice-fuzz
mkdir -p corpus && unzip -q ice_http_parser_fuzzer_seed_corpus.zip -d corpus
./ice_http_parser_fuzzer -max_total_time=300 corpus
```

When a target finds a crash, it writes the input to a `crash-<hash>` file; pass that file to the target to reproduce
it. Once Ice is enrolled in OSS-Fuzz, its `infra/helper.py` also builds and runs the targets exactly as OSS-Fuzz does.

[OSS-Fuzz]: https://github.com/google/oss-fuzz
