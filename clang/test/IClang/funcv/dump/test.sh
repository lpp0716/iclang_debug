set -e

BINPATH="${MYBINPATH:-../../../../../llvm/cmake-build-release/bin}"
BINPATH=$(realpath $BINPATH)
CLANG=/usr/bin/clang++
ICLANGFUNCV=${BINPATH}/iclang-funcv
if [ "${WSL:-0}" = "1" ]; then
  ICLANGFUNCV=${BINPATH}/iclang-funcv.exe
fi

echo "Test funcv dump =================================================="

rm -f *.o
rm -f *.log
rm -f a.out

${CLANG} -c -ffunction-sections -fdata-sections -o old.o old.cpp
${CLANG} -c -ffunction-sections -fdata-sections -o new.o new.cpp

${ICLANGFUNCV} old.o new.o merge.o funcx.txt 1 > dump.log 2>&1

echo "FuncV dump pass =================================================="