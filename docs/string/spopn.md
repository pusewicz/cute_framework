[](../header.md ':include')

# spopn

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Removes n characters from the back of a string.

```cpp
#define spopn(s, n) (s = cf_string_pop_n(s, n))
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
n | Number of characters to pop.

## Related Pages

[spop](/string/spop.md)  
[slast](/string/slast.md)  
[serase](/string/serase.md)  