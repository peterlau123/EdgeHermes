#!/bin/bash

# Define the directories to search for C++ files
DIRECTORIES=("include" "source" "standalone/source" "test/source")

# Find all C++ source and header files in the specified directories
FILES=$(find "${DIRECTORIES[@]}" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cu" -o -name "*.cuh" \) 2>/dev/null)

# Check if any files are found
if [ -z "$FILES" ]; then
    echo "No C++ files found to format in the specified directories."
    exit 0
fi

# Run clang-format on each file
for FILE in $FILES; do
    echo "Formatting $FILE"
    clang-format -i "$FILE"

    if [ $? -ne 0 ]; then
        echo "Error formatting $FILE"
    fi
done

echo "All C++ files in the specified directories have been formatted."


