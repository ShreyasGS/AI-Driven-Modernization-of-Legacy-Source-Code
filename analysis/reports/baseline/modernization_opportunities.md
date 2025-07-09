# Mozilla 1.0 Codebase Modernization Opportunities

Report generated on: 2025-07-09 11:47:19

## Summary

Analyzed 1569 files with modernization opportunities across 5 components.

## Overall Statistics

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 15486 | Error code return | Use result types or exceptions |
| error_check | 6510 | Error code check | Use result types with explicit success/failure |
| null_check | 5966 | Null pointer check | Use std::optional or result type |
| c_style_cast | 5098 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| out_parameter | 5027 | Out parameter | Return value or result type |
| query_interface | 3772 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| raw_new | 3245 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| multiple_out_params | 2820 | Multiple out parameters | Return std::tuple or custom struct |
| addref_call | 1475 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| release_call | 1357 | Manual Release call | Use RefPtr or nsCOMPtr |
| raw_delete | 957 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| default_opportunity | 370 | Empty constructor/destructor | Use = default |
| array_delete | 363 | Raw array deallocation | Use std::vector or other container |
| manual_deallocation | 286 | Manual memory deallocation | Replace with smart pointers or RAII pattern |
| array_new | 247 | Raw array allocation | Use std::vector or other container |
| manual_allocation | 137 | Manual memory allocation | Replace with smart pointers or RAII pattern |

## Component Breakdown


### content (448 files)

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 6549 | Error code return | Use result types or exceptions |
| error_check | 2832 | Error code check | Use result types with explicit success/failure |
| out_parameter | 2357 | Out parameter | Return value or result type |
| null_check | 2179 | Null pointer check | Use std::optional or result type |
| query_interface | 1812 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| c_style_cast | 1178 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| raw_new | 1008 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| multiple_out_params | 836 | Multiple out parameters | Return std::tuple or custom struct |
| addref_call | 799 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| release_call | 589 | Manual Release call | Use RefPtr or nsCOMPtr |
| raw_delete | 332 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| default_opportunity | 162 | Empty constructor/destructor | Use = default |
| array_delete | 41 | Raw array deallocation | Use std::vector or other container |
| manual_deallocation | 27 | Manual memory deallocation | Replace with smart pointers or RAII pattern |
| array_new | 23 | Raw array allocation | Use std::vector or other container |
| manual_allocation | 1 | Manual memory allocation | Replace with smart pointers or RAII pattern |

### layout (406 files)

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 3960 | Error code return | Use result types or exceptions |
| null_check | 1620 | Null pointer check | Use std::optional or result type |
| error_check | 1556 | Error code check | Use result types with explicit success/failure |
| out_parameter | 1521 | Out parameter | Return value or result type |
| multiple_out_params | 1471 | Multiple out parameters | Return std::tuple or custom struct |
| query_interface | 1252 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| c_style_cast | 1161 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| raw_new | 867 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| release_call | 395 | Manual Release call | Use RefPtr or nsCOMPtr |
| raw_delete | 234 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| addref_call | 214 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| default_opportunity | 110 | Empty constructor/destructor | Use = default |
| array_delete | 86 | Raw array deallocation | Use std::vector or other container |
| array_new | 60 | Raw array allocation | Use std::vector or other container |
| manual_deallocation | 3 | Manual memory deallocation | Replace with smart pointers or RAII pattern |
| manual_allocation | 2 | Manual memory allocation | Replace with smart pointers or RAII pattern |

### xpcom (404 files)

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 2022 | Error code return | Use result types or exceptions |
| c_style_cast | 1744 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| error_check | 1429 | Error code check | Use result types with explicit success/failure |
| null_check | 1074 | Null pointer check | Use std::optional or result type |
| raw_new | 645 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| out_parameter | 587 | Out parameter | Return value or result type |
| query_interface | 393 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| addref_call | 301 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| release_call | 300 | Manual Release call | Use RefPtr or nsCOMPtr |
| manual_deallocation | 142 | Manual memory deallocation | Replace with smart pointers or RAII pattern |
| raw_delete | 135 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| array_delete | 96 | Raw array deallocation | Use std::vector or other container |
| multiple_out_params | 90 | Multiple out parameters | Return std::tuple or custom struct |
| array_new | 72 | Raw array allocation | Use std::vector or other container |
| manual_allocation | 67 | Manual memory allocation | Replace with smart pointers or RAII pattern |
| default_opportunity | 38 | Empty constructor/destructor | Use = default |

### gfx (265 files)

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 2343 | Error code return | Use result types or exceptions |
| c_style_cast | 958 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| null_check | 911 | Null pointer check | Use std::optional or result type |
| raw_new | 642 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| error_check | 483 | Error code check | Use result types with explicit success/failure |
| multiple_out_params | 405 | Multiple out parameters | Return std::tuple or custom struct |
| out_parameter | 261 | Out parameter | Return value or result type |
| raw_delete | 251 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| array_delete | 136 | Raw array deallocation | Use std::vector or other container |
| manual_deallocation | 112 | Manual memory deallocation | Replace with smart pointers or RAII pattern |
| addref_call | 99 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| array_new | 92 | Raw array allocation | Use std::vector or other container |
| query_interface | 84 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| manual_allocation | 67 | Manual memory allocation | Replace with smart pointers or RAII pattern |
| release_call | 63 | Manual Release call | Use RefPtr or nsCOMPtr |
| default_opportunity | 36 | Empty constructor/destructor | Use = default |

### dom (46 files)

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 612 | Error code return | Use result types or exceptions |
| out_parameter | 301 | Out parameter | Return value or result type |
| query_interface | 231 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| error_check | 210 | Error code check | Use result types with explicit success/failure |
| null_check | 182 | Null pointer check | Use std::optional or result type |
| raw_new | 83 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| addref_call | 62 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| c_style_cast | 57 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| default_opportunity | 24 | Empty constructor/destructor | Use = default |
| multiple_out_params | 18 | Multiple out parameters | Return std::tuple or custom struct |
| release_call | 10 | Manual Release call | Use RefPtr or nsCOMPtr |
| raw_delete | 5 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| array_delete | 4 | Raw array deallocation | Use std::vector or other container |
| manual_deallocation | 2 | Manual memory deallocation | Replace with smart pointers or RAII pattern |

## Top 20 Files with Most Modernization Opportunities

| File | Component | Total Opportunities | Top Patterns |
|------|-----------|---------------------|-------------|
| content/base/src/nsSelection.cpp | content | 1204 | error_code_return (372), error_check (355), null_check (211) |
| content/xul/document/src/nsXULDocument.cpp | content | 1115 | error_code_return (354), error_check (301), null_check (125) |
| layout/html/style/src/nsCSSFrameConstructor.cpp | layout | 763 | null_check (137), error_check (136), error_code_return (110) |
| layout/html/base/src/nsPresShell.cpp | layout | 703 | error_code_return (184), error_check (135), c_style_cast (81) |
| content/xul/content/src/nsXULElement.cpp | content | 651 | error_code_return (254), error_check (134), query_interface (74) |
| content/base/src/nsDocumentViewer.cpp | content | 645 | error_code_return (175), error_check (129), query_interface (119) |
| dom/src/base/nsGlobalWindow.cpp | dom | 567 | error_code_return (211), query_interface (73), null_check (71) |
| layout/html/base/src/nsFrame.cpp | layout | 517 | error_check (176), error_code_return (141), null_check (48) |
| content/base/src/nsDocument.cpp | content | 507 | error_code_return (172), c_style_cast (72), out_parameter (72) |
| content/base/src/nsRange.cpp | content | 474 | error_code_return (156), error_check (134), null_check (77) |
| content/html/document/src/nsHTMLDocument.cpp | content | 441 | error_code_return (119), error_check (94), out_parameter (51) |
| xpcom/components/nsComponentManager.cpp | xpcom | 434 | error_check (124), error_code_return (118), null_check (57) |
| content/events/src/nsEventStateManager.cpp | content | 427 | query_interface (96), null_check (90), error_code_return (88) |
| layout/html/table/src/nsTableFrame.cpp | layout | 422 | c_style_cast (137), null_check (103), multiple_out_params (56) |
| layout/html/forms/src/nsGfxTextControlFrame2.cpp | layout | 416 | error_code_return (148), error_check (82), null_check (70) |
| content/base/src/nsGenericElement.cpp | content | 408 | error_code_return (115), query_interface (68), error_check (34) |
| layout/html/base/src/nsTextFrame.cpp | layout | 380 | error_code_return (83), error_check (73), null_check (49) |
| gfx/src/windows/nsFontMetricsWin.cpp | gfx | 378 | error_code_return (90), c_style_cast (77), null_check (77) |
| content/html/content/src/nsGenericHTMLElement.cpp | content | 376 | error_code_return (96), query_interface (55), error_check (54) |
| content/html/style/src/nsCSSStyleSheet.cpp | content | 372 | c_style_cast (95), error_code_return (92), error_check (36) |
