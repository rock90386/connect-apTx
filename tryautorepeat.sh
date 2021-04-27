while true ; 
do 
	tshark -x -r fd-server-0-0.pcap | sed -n 's/^[0-9a-f]*\s\(\(\s[0-9a-f][0-9a-f]\)\{1,16\}\).*$/\1/p' > pcapfile.txt ;
sleep 1;
 tshark -x -r ssddf.pcap | sed -n 's/^[0-9a-f]*\s\(\(\s[0-9a-f][0-9a-f]\)\{1,16\}\).*$/\1/p' > pcapfile2.txt;
sleep 1;
 done
