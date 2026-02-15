# Running the dotnet test suite

We can run tests using a custom xunit runner. This is based on the android standalone one.

For example, build the common tests project:

```
./build.sh --cross -a arm64 --os libnx -projects $MONO_NX_ROOT/src/libraries/Common/tests/Common.Tests.csproj
```

Then you can just run it with the test_runner project
