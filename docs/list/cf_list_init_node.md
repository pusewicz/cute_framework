[](../header.md ':include')

# cf_list_init_node

Category: [list](/api_reference?id=list)  
GitHub: [cute_doubly_list.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_doubly_list.h)  
---

Intializes a node.

```cpp
void cf_list_init_node(CF_ListNode* node)
```

Parameters | Description
--- | ---
node | The node.

## Remarks

As this list is circular, each node is initialized to point to itself.

## Related Pages

[CF_ListNode](/list/cf_listnode.md)  
[CF_List](/list/cf_list.md)  
[CUTE_LIST_NODE](/list/cute_list_node.md)  
[CUTE_LIST_HOST](/list/cute_list_host.md)  
[cf_list_back](/list/cf_list_back.md)  
[cf_list_init](/list/cf_list_init.md)  
[cf_list_push_front](/list/cf_list_push_front.md)  
[cf_list_push_back](/list/cf_list_push_back.md)  
[cf_list_remove](/list/cf_list_remove.md)  
[cf_list_pop_front](/list/cf_list_pop_front.md)  
[cf_list_pop_back](/list/cf_list_pop_back.md)  
[cf_list_empty](/list/cf_list_empty.md)  
[cf_list_begin](/list/cf_list_begin.md)  
[cf_list_end](/list/cf_list_end.md)  
[cf_list_front](/list/cf_list_front.md)  