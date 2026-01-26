set -e

echo "Test All =================================================="

cd driver
./test.sh
cd ..

cd funcx
./test.sh
cd ..

cd funcv
./test.sh
cd ..

cd system
./test.sh
cd ..

echo "All pass =================================================="