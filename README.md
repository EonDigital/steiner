# steiner
A tool for playing around with Steiner systems.

# Usage
build/steiner.out <v> <k> <t>

Attempts to find a steiner system with these properties.

# Approach
This uses bitmasks to manage k-blocks.
It uses the known differences between t and k sizes to quickly step
to new candidate blocks before testing the history to see if a new
block is valid.  If it is, it adds it to the history.  As an example,
if you have a (v,5,3) system, the active candidates are 2 wide and may
be treated differently.

If a quick step is insufficient, it will search by incrementing the 
subset of the field that is active to the next field with the same
number of bits.

If the working space is not successful, it advances the rest of the 
field a step at a time to ensure full discovery.

# Limitations
The current code does not confirm the result is a steiner system.
The system does not know how to backtrack when a result is found to be
invalid.
