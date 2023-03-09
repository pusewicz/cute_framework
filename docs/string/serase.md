[](../header.md ':include')

# serase

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Deletes a number of characters from the string.

```cpp
#define serase(s, index, count) cf_string_erase(s, index, count)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
index | Index in the string to start deleting from.
count | Number of character to delete.

## Related Pages

[strim](/string/strim.md)  
[sltrim](/string/sltrim.md)  
[srtrim](/string/srtrim.md)  
[slpad](/string/slpad.md)  
[srpad](/string/srpad.md)  
[sdedup](/string/sdedup.md)  
[sreplace](/string/sreplace.md)  