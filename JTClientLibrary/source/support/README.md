# Support library

We need all kind of little helpers for ensure ABI compatibility, accessing existing structures at given addresses or 
even hooking into existing functions. This library has it all. 

## ABI checking

To check the layout (offset of members) in any class, simply add the following structure to your class declaration.
This is a compile-time check. If the offsets are wrong, your code will not compile.


\include abitest.example.h


### ABI checking under the hood

The main goal is to check the offset at compile time and throw a compiler error in case of a wrong offset. If an offset is wrong, the code
will most likely not work, there is no point in running it.

The offset checking is built around `offsetof`. This macro returns the offset of a member variable in a class. We compare that offset to the
expected offset and if they are different, the offset is wrong.

We don't have static_assert, so we need a poor mans static_assert:

```cpp
int assert_me[expected == actual ? 1 : -1];
```

If `expected` equals `actual`, we get an array of size `1`. If these are not equal, the array size will be `-1`, causing a compile time error.


We need to declare that array in a separate class because `offsetof` can not work on incomplete types. And we must make it a child (or friend)
of the class we want to test. Otherwise we can only access the public interface of the class, which usually does not include member variables.

```cpp
class CIFlol {

  int x;

  template<typename T>
  class TestFixture {
    int check_x[offsetof(T, x) == 0 ? 1 : -1];

  }

  static void _RunFixture() {
      static CIFlol::TestFixture<CIFlol> testFixture;
  }

};
```

To avoid the incomplete type, we use a template class. The template is constructed after the class to test is complete. `TestFixture` is a
child of the class to test, so we have no issues accessing the private members.

This works fine, however it does not produce useful error messages.

```
error C2118: negative subscript
```

We want to know which member is at at an incorrect offset, and what that offset is.

```cpp
class CIFlol {

  int x;

  template<typename T>
  class TestFixture {

      class Check_x {};

      template<typename T, int Want, int Have>
      class CheckHelper {
          char checker[Want == Have ? 1 : -1];
      };

      CheckHelper<Check_x, 0, offsetof(T, x)> checkit;

  }

  static void _RunFixture() {
      static CIFlol::TestFixture<CIFlol> testFixture;
  }

};
```

We use another template to trigger a template error. Aside from expected and actual offset, the template also accepts a typename. That typename
is used to also print the name of the member at fault. We just make an empty dummy class with a matching name.

This will produce a template error like this:

```
with
[
    T=CIFlol,
    U=CIFlol::TestFixture<CIFlol>::Check_x,
    Want=1,
    Have=0
]
```

However, MSVC claims that offsetof is not compile-time static, since it uses reinterpret_cast. It still compiles, so I dont really understand
what the deal is. But we can do better.

Meet: Poor Mans constexpr.

```cpp
enum {
    actual_x_offset = offsetof(T, x)
};
```

All together it now looks like this:

```cpp
class CIFlol {

  int x;

  template<typename T>
  class TestFixture {

      class Check_x {};
      template<typename T, int Want, int Have>
      class CheckHelper {
          char checker[Want == Have ? 1 : -1];
      };

      enum {
          actual_x_offset = offsetof(T, x)
      };

      CheckHelper<Check_x, 0, actual_x_offset> checkit;

  }

  static void _RunFixture() {
      static CIFlol::CIFlol> testFixture;
  }

};
```

This is very hard to use, so all the details are hidden inside functionlike macros. In order to get the IDEs highlight the line of `ENSURE_OFFSET` that has failed, we have to generate the CheckHelper for each member aswell.



