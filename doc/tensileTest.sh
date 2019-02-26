# nohup ./tensileTest.sh > tempout.file 2>&1 &
# jobs
# fg [n]

## vega64 test
#./TensileConv.out -s 7 -c 512 -k 2048 -n 1 --search 0
#./TensileConv.out -s 7 -c 512 -k 2048 -n 4 --search 0
#./TensileConv.out -s 7 -c 512 -k 2048 -n 32 --search 0
#./TensileConv.out -s 7 -c 1024 -k 512 -n 1 --search 0
#./TensileConv.out -s 7 -c 1024 -k 512 -n 4 --search 0
#./TensileConv.out -s 7 -c 1024 -k 512 -n 32 --search 0
#./TensileConv.out -s 7 -c 2048 -k 512 -n 1 --search 0
#./TensileConv.out -s 7 -c 2048 -k 512 -n 8 --search 0
#./TensileConv.out -s 7 -c 2048 -k 512 -n 16 --search 0
#./TensileConv.out -s 14 -c 256 -k 1024 -n 8 --search 0
#./TensileConv.out -s 14 -c 256 -k 1024 -n 16 --search 0
#./TensileConv.out -s 14 -c 512 -k 512 -n 4 --search 0
#./TensileConv.out -s 14 -c 512 -k 512 -n 8 --search 0
#./TensileConv.out -s 14 -c 1024 -k 256 -n 1 --search 0
#./TensileConv.out -s 14 -c 1024 -k 256 -n 4 --search 0
#./TensileConv.out -s 14 -c 1024 -k 256 -n 16 --search 0
#./TensileConv.out -s 28 -c 128 -k 512 -n 1 --search 0
#./TensileConv.out -s 28 -c 128 -k 512 -n 4 --search 0
#./TensileConv.out -s 28 -c 128 -k 512 -n 8 --search 0
#./TensileConv.out -s 28 -c 128 -k 1024 -n 1 --search 0
#./TensileConv.out -s 28 -c 128 -k 1024 -n 4 --search 0
#./TensileConv.out -s 28 -c 128 -k 1024 -n 8 --search 0
#./TensileConv.out -s 28 -c 512 -k 256 -n 1 --search 0
#./TensileConv.out -s 28 -c 512 -k 256 -n 4 --search 0
#./TensileConv.out -s 28 -c 512 -k 256 -n 8 --search 0
#./TensileConv.out -s 28 -c 512 -k 256 -n 16 --search 0
#./TensileConv.out -s 28 -c 512 -k 256 -n 32 --search 0
#./TensileConv.out -s 28 -c 512 -k 1024 -n 1 --search 0
#./TensileConv.out -s 28 -c 512 -k 1024 -n 4 --search 0

# genetic search
./TensileConv.out -i 10 -s 7 -c 512 -k 2048 -n 32 --search 0
./TensileConv.out -i 10 -s 7 -c 512 -k 2048 -n 32 --search 1
./TensileConv.out -i 10 -s 7 -c 1024 -k 512 -n 32 --search 0
./TensileConv.out -i 10 -s 7 -c 1024 -k 512 -n 32 --search 1
./TensileConv.out -i 10 -s 7 -c 2048 -k 512 -n 16 --search 0
./TensileConv.out -i 10 -s 7 -c 2048 -k 512 -n 16 --search 1
./TensileConv.out -i 10 -s 14 -c 256 -k 1024 -n 16 --search 0
./TensileConv.out -i 10 -s 14 -c 256 -k 1024 -n 16 --search 1
./TensileConv.out -i 10 -s 14 -c 1024 -k 256 -n 16 --search 0
./TensileConv.out -i 10 -s 14 -c 1024 -k 256 -n 16 --search 1
./TensileConv.out -i 10 -s 28 -c 128 -k 512 -n 8 --search 0
./TensileConv.out -i 10 -s 28 -c 128 -k 512 -n 8 --search 1
./TensileConv.out -i 10 -s 28 -c 128 -k 1024 -n 8 --search 0
./TensileConv.out -i 10 -s 28 -c 128 -k 1024 -n 8 --search 1
./TensileConv.out -i 10 -s 28 -c 512 -k 256 -n 8 --search 0
./TensileConv.out -i 10 -s 28 -c 512 -k 256 -n 8 --search 1
./TensileConv.out -i 10 -s 28 -c 512 -k 256 -n 32 --search 0
./TensileConv.out -i 10 -s 28 -c 512 -k 256 -n 32 --search 1
./TensileConv.out -i 10 -s 28 -c 512 -k 1024 -n 4 --search 0
./TensileConv.out -i 10 -s 28 -c 512 -k 1024 -n 4 --search 1