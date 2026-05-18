# ToyEngine Project Instructions

## Code Style & Readability Mandates

To ensure the codebase remains clean, maintainable, and easy to scan, all generated or modified code MUST adhere to the following formatting standards:

1.  **Generous Vertical Spacing:**
    *   Use new lines to separate logical blocks of logic within functions.
    *   Add a blank line before and after control flow statements (`if`, `for`, `while`, `switch`) unless they are part of a very short, related sequence.
    *   Separate member variable declarations from method declarations in class headers.

2.  **Logical Grouping:**
    *   Group related variable initializations together, followed by a blank line before the first operation.
    *   Vulkan initialization structures should be separated by blank lines to make the high-volume boilerplate readable.

3.  **No Dense Blocks:**
    *   Avoid long sequences of code without vertical breaks. If a block exceeds 5-7 lines, evaluate if it can be split into smaller, logically grouped sections.

4.  **Header Organization:**
    *   Group includes by category (Standard Library, Third-Party, Project Headers) and separate groups with blank lines.

---

*These rules are foundational and take precedence over default formatting tools if they conflict with the goal of high legibility.*
