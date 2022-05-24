while true
do

bin/client -f solsock.sk -W files/files1/file1.txt -r files/files1/file1.txt -W files/files2/file9.txt -r files/files1/file5.txt

bin/client -f solsock.sk -W files/files1/file3.txt -l files/files1/file3.txt -r files/files1/file3.txt -u files/files1/file3.txt

bin/client -f solsock.sk -W files/files2/file7.txt,files/files2/file8.txt -r files/files1/file1.txt -l files/files2/file8.txt

bin/client -f solsock.sk -w files/files2,0 -R 3 -d test3savedfiles/1 -r files/files1/file2.txt -W files/files1/file5.txt

bin/client -f solsock.sk -r files/files1/file1.txt -W files/files2/file9.txt -r files/files1/file5.txt -l files/files2/file9.txt

bin/client -f solsock.sk -l files/files2/file5.txt -u files/files2/file5.txt -R 1 -d test3savedfiles/2 -r files/files2/file7.txt

bin/client -f solsock.sk -W files/files1/file2.txt -r files/files1/file1.txt,files/files1/file2.txt -l files/files2/file5.txt

bin/client -f solsock.sk -w files/files3,0 -l files/files3/file14.txt -u files/files3/file14.txt -r files/files3/file12.txt

done