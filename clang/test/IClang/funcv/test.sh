set -e

echo "Test funcv =================================================="

for dir in "."/*/ ; do
    dir_name=$(basename "${dir%/}")
    cd ${dir_name}

    ./test.sh

    cd ..
done

echo "FuncV pass =================================================="