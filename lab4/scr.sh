#!/bin/bash

for i in {1..3}; do
	printf "%s\n%s\n" "$i" "../../bmp/" | ./prog.sh
	cp p$i.txt $i-bmp.txt

	printf "%s\n%s\n" "$i" "../../data/medic/" | ./prog.sh
	cp p$i.txt $i-med.txt

	printf "%s\n%s\n" "$i" "../../data/textur/" | ./prog.sh
	cp p$i.txt $i-tex.txt
done

for i in {1..3}; do
	printf "%s\n%s\n" "$i" "../2-bmp/" | ./prog.sh
	cp p$i.txt $i-2bmp.txt

	printf "%s\n%s\n" "$i" "../2-med/" | ./prog.sh
	cp p$i.txt $i-2med.txt

	printf "%s\n%s\n" "$i" "../2-tex/" | ./prog.sh
	cp p$i.txt $i-2tex.txt
done

for i in {1..3}; do
	printf "%s\n%s\n" "$i" "../3-bmp/" | ./prog.sh
	cp p$i.txt $i-3bmp.txt

	printf "%s\n%s\n" "$i" "../3-med/" | ./prog.sh
	cp p$i.txt $i-3med.txt

	printf "%s\n%s\n" "$i" "../3-tex/" | ./prog.sh
	cp p$i.txt $i-3tex.txt
done