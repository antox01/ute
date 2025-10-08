// Possible implementation
// - Using a diff structure
//   - Add a way to denote the start of a diff
//   - Should we use a backbuffer? Probably not needed if we just use the snapshot buffer as a backbuffer
//   - Insert/Delete type for a diff
//   - Store the line start and end
//   - Store the previous value of the string and the current one
//   - In case of the redo, change the types of the diff
//   - Should we implement an undotree or just a simple list? Let's do a simple list
//   - Think of adding an Arena to possibly optimize the allocations
//   - How do we tell when to start a diff in entering insert mode?
//      - When entering insert mode, we start the new history command, and then we just add a new character both on the Gap_Buffer and then on the snapshot buffer
