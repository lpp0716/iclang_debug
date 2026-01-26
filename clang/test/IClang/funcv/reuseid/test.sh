set -e

BINPATH="${MYBINPATH:-../../../../../llvm/cmake-build-release/bin}"
BINPATH=$(realpath $BINPATH)
CLANG=/usr/bin/clang++
ICLANGFUNCV=${BINPATH}/iclang-funcv
if [ "${WSL:-0}" = "1" ]; then
  ICLANGFUNCV=${BINPATH}/iclang-funcv.exe
fi

echo "Test funcv reuseid =================================================="

rm -f *.o
rm -f *.log
rm -f a.out

${CLANG} -c -ffunction-sections -fdata-sections -o old.o old.cpp
${CLANG} -c -ffunction-sections -fdata-sections -o new.o new.cpp

${ICLANGFUNCV} old.o new.o merge.o funcx.txt

symbol_value=$(readelf -sW merge.o | grep reusev | awk '{print $2}')

if [ "$symbol_value" = "0000000000000001" ]; then
    echo "merge.o pass"
else
    echo "merge.o error: $symbol_value != 0000000000000001"
fi

${CLANG} -o a.out merge.o
./a.out

${ICLANGFUNCV} merge.o new.o merge2.o funcx.txt
${CLANG} -o a.out merge2.o
./a.out

symbol_value=$(readelf -sW merge2.o | grep reusev | awk '{print $2}')

if [ "$symbol_value" = "0000000000000002" ]; then
    echo "merge2.o pass"
else
    echo "merge2.o error: $symbol_value != 0000000000000002"
fi

${ICLANGFUNCV} merge2.o new.o merge3.o funcx.txt
${CLANG} -o a.out merge3.o
./a.out

symbol_value=$(readelf -sW merge3.o | grep reusev | awk '{print $2}')

if [ "$symbol_value" = "0000000000000003" ]; then
    echo "merge3.o pass"
else
    echo "merge3.o error: $symbol_value != 0000000000000003"
fi

echo "FuncV reuse id pass =================================================="