PATH="$(pwd -P)"

while true
do

bin/client -f solsock.sk -W files/files1/file1.txt -r ${PATH}/files/files1/file1.txt -W files/files2/file9.txt -r ${PATH}/files/files1/file5.txt &

bin/client -f solsock.sk -W files/files1/file3.txt -l ${PATH}/files/files1/file3.txt -r ${PATH}/files/files1/file3.txt -c ${PATH}/files/files1/file3.txt &

bin/client -f solsock.sk -W files/files2/file7.txt,files/files2/file8.txt -r ${PATH}/files/files1/file1.txt -l ${PATH}/files/files2/file8.txt &

bin/client -f solsock.sk -w files/files2,0 -R 3 -d test3savedfiles/1 -r ${PATH}/files/files1/file2.txt

bin/client -f solsock.sk -c ${PATH}/files/files2/file6.txt -r ${PATH}/files/files1/file1.txt -W files/files2/file9.txt -r ${PATH}/files/files1/file5.txt

bin/client -f solsock.sk -l ${PATH}/files/files2/file5.txt -u ${PATH}/files/files2/file5.txt -R 1 -d test3savedfiles/2

bin/client -f solsock.sk -W files/files1/file2.txt -r ${PATH}/files/files1/file1.txt,${PATH}/files/files1/file2.txt -l ${PATH}/files/files2/file5.txt

bin/client -f solsock.sk -w files/files3,0 -l ${PATH}/files/files3/file14 -u ${PATH}/files/files3/file14

bin/client -f solsock.sk -R 7 -d test3savedfiles/3 -c files/files3/file14,files/files3/file13

done