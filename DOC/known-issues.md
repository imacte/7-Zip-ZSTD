# Known issues

## LZ5 does not round-trip on Windows ARM64

Reported 2026-09-29 (CI run 36522834035, job `windows (arm64)`), still open.

The LZ5 codec of the Windows ARM64 build is not usable: an archive written with
`-tlz5` or `-m0=lz5` is reported as damaged when it is read back by the same
binary. Avoid LZ5 on Windows ARM64 until this is fixed.

Symptoms (from the tcl suite on `windows-11-arm`):

* `7z a -tlz5 -mx3 -si -so | 7z e -tlz5 -si -so` → `ERROR: Data Error`
* `7z a -t7z -m0=lz5:x0 -sitest.txt -- test.lz5.7z .` followed by `7z t` →
  `Archives with Errors: 1 ... ERROR: The data is invalid.` (checksum mismatch)
* Failing cases: `main--pipe-a-to-e`, `main--pipe-a-to-e-mx0`,
  `main--compress-mx0`; the remaining 42 tests pass with the same binary.

What is known about the scope:

* Windows ARM64 only. Linux ARM64 and Windows x86/x64 pass the same tests.
* The payload is small and arrives via stdin in all three failing cases; the
  larger file based `-m0=lz5:x0` case passes.
* The compressed size equals the x64 result (an `lz5.7z` of 203 bytes, i.e. 122
  header bytes plus an 81 byte frame for the 42 byte test string), so decoding or
  frame verification looks more likely than the encoder.

Not reproducible on x86/x64 so far: plain `char` and `/J` (the ARM64 default),
1 and 4 threads, codec levels 1 and 3, the exact `CFLAGS_O1` of the makefiles
(`/O1 /Gy /GR- /Gw /GS-`), `/O2 /GL`, and an AddressSanitizer build of
`C/lz5` plus `C/zstdmt/lz5-mt_*` all round-trip correctly.

The failures are visible without the authenticated job log: the `Test` steps
publish failing tcltest cases as check annotations, see
`tests/publish-test-failures.ps1`.

Next step: collect codec diagnostics on an ARM64 host (or from the arm64 CI job)
to attribute the defect to encoding, decoding or frame verification.
