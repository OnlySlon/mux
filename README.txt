iptables -I OUTPUT -p icmp --icmp-type destination-unreachable -j DROP
ip link set arp off dev 

echo 0 > /proc/sys/net/ipv4/conf/eth1/rp_filter
echo 0 > /proc/sys/net/ipv4/conf/eth2/rp_filter


TODO
- DETECT LINK UP/DOWN
- DETECT LINK LOSS %
- DIFF WORK  MODE on TX/RX


./a.out -i zzz -c 22.22.22.22  -u eth1:eth2:eth3 -m 1
./a.out -s -i zzz -m1
tc qdisc add dev eth3 root netem loss 0.1% delay 5ms



----------------------
CLIENT

                  -----  ETH1 (MASTER)  192.168.0.0/24  
                 / 
 CLIENT - |MUX DEV| ---  ETH2 (SLAVE)   192.168.1.0/24 (TUN)
                 \
		  -----  ETH3 (SLAVE)   192.168.2.0/24 (TUN)



SERVER aka Bridge


--------| MUX SERVER |
