
declare -a pcap_name=(a b c d e f g h i j)
declare -i ii=0
sleep 7s
while true ; 
do 
	editcap -r fd-server-0-0.pcap ${pcap_name[$ii%10]}.pcap $ii
	tshark -x -r ${pcap_name[$ii%10]}.pcap | sed -n 's/^[0-9a-f]*\s\(\(\s[0-9a-f][0-9a-f]\)\{1,16\}\).*$/\1/p' > ${pcap_name[$ii%10]}.txt ;
ii=ii+1
# sleep 0.0083s
echo $ii 
 done
# while true ;
# do
# editcap -c 1 aaaaa.pcap smallfile.pcap
# sleep 1
# done