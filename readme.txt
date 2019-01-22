Jordan Sybesma
Teddy Willard
Will Knospe

Our interpreter implements the base functionality of R5RS Scheme in C alongside a few extensions. 
The interpreter accomplishes this by tokenizing any input, parsing it into a tree using an LR
grammar, then passing the tree into the interpreter for evaluation. At each step, improper syntax
and values are caught such that any invalid requests will not result in any issues in the interpreter
itself.

All required features are functional and well tested. We have also prepared several of the project
extensions and incorporated them into our repository. These extensions include:
- Functional command line interface
- Mark and Sweep Garbage Collection
- ' as an alias for the quote operator
- + operator handles the proper numerical return type
- Partial lists.scm

All in all, this project went pretty smoothly, and we're all happy with the final result. It was really cool to see everything coming together.