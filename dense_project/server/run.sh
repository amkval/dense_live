# Find how many threads we can use for faster compilation
cores=$(threads)

printf "We have $threads threads!\nUsing all of them for compilation...\n"

# Make sure that we have the latest program
make -B -j$threads

# Run the program in GDB automaticaly
gdb -x commands.txt --args ./testProgs/server ./extras/chunks/ 60