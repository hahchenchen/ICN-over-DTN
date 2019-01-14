该文件夹是地球上的网关。

对于client中的两种客户端情况分别进行配置。

客户端一：
只需要按照客户端一的说明配置即可。客户端一也是网关。

客户端二：

网关所需软件：
gateway_on_earth/ccn-lite
gateway_on_earth/ion-dtn

网关命令如下：
sudo ionstart -I host1.rc

bpchat ipn:1.1 ipn:2.1

ccn-lite-relay -v trace -s ndn2013 -u 9999 -x /tmp/mgmt-relay-a.sock -d /home/istin/ccn-lite/test/ndntlv

FACEID=`ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock newUNIXface /tmp/my.sock | ccn-lite-ccnb2xml | grep FACEID | sed -e 's/^[^0-9]*\([0-9]\+\).*/\1/'`


ccn-lite-ctrl -x /tmp/mgmt-relay-a.sock prefixreg /ndn $FACEID ndn2013 | ccn-lite-ccnb2xml


这里的ccn-lite-relay是设置监听和转发的关键程序，ccn-lite-relay的-u  -x 都是设置的监听的端口。-u 9999表示监听ip地址的9999端口，即监听来自客户端二的请求。-x /tmp/mgmt-relay-a.sock表示监听unix socket接口，mgmt-relay-a.sock是对ccn-lite-relay进行下一步设置的接口。

FACEID表示设置转发的接口，这里设置了newUNIXface /tmp/my.sock，表示新创建一个unix转发接口，也就是收到的兴趣包如果在该网关的cs中没有，则会进行转发至该接口，这个接口这里设置为/tmp/my.sock，是bpchat的监听接口，也就是转发给bpchat了。

ccn-lite-ctrl 表示对ccn-lite-relay进行设置转发接口，-x /tmp/mgmt-relay-a.sock表示利用这个接口对ccn-lite-relay进行控制。
