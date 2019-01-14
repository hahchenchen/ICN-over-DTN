这是火星上的网关，负责与地球上的网关以及火星表面网络通信。

需要安装的软件：
gateway_on_mars/ccn-lite
gateway_on_mars/ion-dtn
gateway_on_mars/RIOT

命令如下：

sudo ionstart -I host2.rc

bpchat ipn:2.1 ipn:1.1

ccn-lite-relay -v trace -s ndn2013 -u 9998 -x /tmp/ccn.sock -x /tmp/mgmt-relay-b.sock


FACEID=`ccn-lite-ctrl -x /tmp/mgmt-relay-b.sock newUNIXface /tmp/ccn.sock | ccn-lite-ccnb2xml | grep FACEID | sed -e 's/^[^0-9]*\([0-9]\+\).*/\1/'`

ccn-lite-ctrl -x /tmp/mgmt-relay-b.sock prefixreg /ndn $FACEID ndn2013 | ccn-lite-ccnb2xml


还要设置火星上的板子通信。

板子要连接该网关的串口。板子烧写程序:gateway_on_mars/RIOT/examples/ccn-lite-relay

上电后。对串口通信进行初始化：
init 2 115200，2指的是串口2，接口为PD5(tx),PD6(RX). 115200是波特率

然后就设置另一块板子的仓库。

这里的ccn-lite-relay监听了三个接口，第一个-u 9998，其实这个接口现在并用不到，这是用来监听火星上的用户对地球数据的请求的，而目前整个架构实现的是地球到火星的单向请求。
第二个接口，-x /tmp/ccn.sock，这个很明显，是监听DTN的数据，也就是从地球发送过来的请求通过DTN发过来，然后DTN通过ccn.sock发送给ccn。ccn再将这个请求通过uart转发给板子。
第三个接口：-x /tmp/mgmt-relay-b.sock，是监听ccn-lite-ctrl发送的控制数据。

这里FACEID也是设置的是转发端口，这里设置了一个newUNIXface /tmp/ccn.sock，这个设置其实和ccn-lite-relay的第一个-u接口一样，是用来转发火星上的用户对地球数据的请求的，如果火星请求在这个网关的cs中没有找到，则通过这个接口转发给底层dtn，然后传输至地球网关。这个接口目前用不到。
也就是说，对于地球发来的请求，这里其实并不需要设置转发接口，因为，在源码里进行了改动，从DTN获得的数据都会发送给串口。



从上面设置可以看出，ccn-lite-relay 负责监听来自不同接口的请求(-x,-u等)，ccn-lite-ctrl负责添加转发的接口(newUNIXface,newUDPface等)。只要弄清楚这两者的逻辑关系和接口设置，其实就可以很简单的设置单向或者双向通信。火星到地球的通信其实也可以进行设置了。只要添加转发接口和监听接口即可。



