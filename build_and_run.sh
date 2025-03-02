clear

make clean

make

if [ $? -eq 0 ]; then
    ./bin/myProgram

    EXIT_CODE=$?
    
    make clean

    if [ $EXIT_CODE -eq 0 ]; then
        echo "Program executed successfully."
    else
        echo "Program failed with exit code $EXIT_CODE."
    fi
else
    echo "Compiling failed. Check for errors."
fi