set -e

BINPATH="${MYBINPATH:-../../../../../llvm/cmake-build-release/bin}"
BINPATH=$(realpath $BINPATH)
CLANG=/usr/bin/clang++
ICLANGFUNCV=${BINPATH}/iclang-funcv
if [ "${WSL:-0}" = "1" ]; then
  ICLANGFUNCV=${BINPATH}/iclang-funcv.exe
fi

echo "Test funcv reuse =================================================="

for dir in "."/*/ ; do
    dir_name=$(basename "${dir%/}")
    cd ${dir_name}
    echo "[Test ${dir_name}]"
    rm -f *.o
    rm -f *.log
    rm -f a.out

    ${CLANG} -c -ffunction-sections -fdata-sections -o old.o old.cpp
    ${CLANG} -c -ffunction-sections -fdata-sections -o new.o new.cpp

    ${ICLANGFUNCV} old.o new.o merge.o funcx.txt

    ${CLANG} -o a.out merge.o
    # Must return 0, otherwise set -e will terminate the script
    ./a.out

    echo "${dir_name} pass"

    cd ..
done

echo "FuncV reuse pass =================================================="