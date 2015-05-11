#!/bin/bash

touch outputs.txt
echo "" > outputs.txt

echo "***** RCV *****" >> outputs.txt

TRAIN="/afs/andrew.cmu.edu/usr19/wtwood/public/RCV.binary.train.dat"
TEST="/afs/andrew.cmu.edu/usr19/wtwood/public/RCV.binary.test.dat"


echo "Seq"
echo "----- Seq -----" >> outputs.txt

./hogwild_seq -f $TRAIN -t $TEST >> outputs.txt

echo "Naive"
echo "----- Naive -----" >> outputs.txt

echo "2"
./hogwild_naive -p 2 -f $TRAIN -t $TEST >> outputs.txt
echo "4"
./hogwild_naive -p 4 -f $TRAIN -t $TEST >> outputs.txt
echo "8"
./hogwild_naive -p 8 -f $TRAIN -t $TEST >> outputs.txt
echo "12"
./hogwild_naive -p 12 -f $TRAIN -t $TEST >> outputs.txt

echo "HogWild!"
echo "----- HogWild! -----" >> outputs.txt

echo "2"
./hogwild -p 2 -f $TRAIN -t $TEST >> outputs.txt
echo "4"
./hogwild -p 4 -f $TRAIN -t $TEST >> outputs.txt
echo "8"
./hogwild -p 8 -f $TRAIN -t $TEST >> outputs.txt
echo "12"
./hogwild -p 12 -f $TRAIN -t $TEST >> outputs.txt

echo "HogWildChild!"
echo "----- HogWildChild! -----" >> outputs.txt

echo "2"
./hogwildchild -p 2 -f $TRAIN -t $TEST >> outputs.txt
echo "4"
./hogwildchild -p 4 -f $TRAIN -t $TEST >> outputs.txt
echo "8"
./hogwildchild -p 8 -f $TRAIN -t $TEST >> outputs.txt
echo "12"
./hogwildchild -p 12 -f $TRAIN -t $TEST >> outputs.txt

echo "***** WIKI *****" >> outputs.txt

TRAIN="/tmp/wtwood/wiki.binary.train.dat"
TEST="/tmp/wtwood/wiki.binary.test.dat"

echo "Seq"
echo "----- Seq -----" >> outputs.txt

./hogwild_seq -f $TRAIN -t $TEST >> outputs.txt

echo "Naive"
echo "----- Naive -----" >> outputs.txt
echo "2"

./hogwild_naive -p 2 -f $TRAIN -t $TEST >> outputs.txt
echo "4"
./hogwild_naive -p 4 -f $TRAIN -t $TEST >> outputs.txt
echo "8"
./hogwild_naive -p 8 -f $TRAIN -t $TEST >> outputs.txt
echo "12"
./hogwild_naive -p 12 -f $TRAIN -t $TEST >> outputs.txt

echo "HogWild!"
echo "----- HogWild! -----" >> outputs.txt

echo "2"
./hogwild -p 2 -f $TRAIN -t $TEST >> outputs.txt
echo "4"
./hogwild -p 4 -f $TRAIN -t $TEST >> outputs.txt
echo "8"
./hogwild -p 8 -f $TRAIN -t $TEST >> outputs.txt
echo "12"
./hogwild -p 12 -f $TRAIN -t $TEST >> outputs.txt

echo "HogWildChild!"
echo "----- HogWildChild! -----" >> outputs.txt

echo "2"
./hogwildchild -p 2 -f $TRAIN -t $TEST >> outputs.txt
echo "4"
./hogwildchild -p 4 -f $TRAIN -t $TEST >> outputs.txt
echo "8"
./hogwildchild -p 8 -f $TRAIN -t $TEST >> outputs.txt
echo "12"
./hogwildchild -p 12 -f $TRAIN -t $TEST >> outputs.txt
