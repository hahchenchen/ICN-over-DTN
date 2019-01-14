地球上的客户端，负责向火星请求数据。
分为两种客户端。

客户端一：
地球上的转换网关上直接就是客户请求:ccn-lite-peek

客户端(地球上的卫星网关)<———————>火星网关<——————>火星表面传感器

这种情况下，要启动DTN作为底层传输网络，然后开启ccn进行数据传输。

客户端(地球上的卫星网关)装软件:
cient文件夹的ION-DTN和ccn-lite

命令如下：
客户端作为端点1，火星端作为端点2

sudo ionstart -I host1.rc

bpchat ipn:1.1 ipn:2.1

ccn-lite-peek -w 10 -s ndn2013 -x /tmp/my.sock "/ndn/test/mycontent" | ccn-lite-pktdump -f 2

ccn-lite-peek的作用是发送数据到指定接口，其中接口由命令行参数决定，接口可以选择-u：ipv4，也可以选择-x：unix socket。这里选择的是-x my.sock是bpchat绑定的socket路径，也就是通过发送到这个接口可以实现ccn与bp层的通信。
-w 10，是选择重传时间10秒，如果10秒内ccn没收到数据，则重传兴趣包。如果不设定该参数，默认是3秒。
具体的ccn-lite-peek参数列表可以去ccn-lite-peek源文件中查看。

ccn-lite-pktdump作用是对收到的数据包进行解析，-f 2参数表示只显示收到的数据内容，而不是整个包。


客户端二：
地球上的某个用户，通过地球上的卫星网关，将兴趣包发送到火星网关，然后得到数据。

客户端<————>地球上卫星网关<——————>火星网关<———————>火星表面传感器

这种情况下，客户端只需要运行ICN即可，不用运行DTN，ccn-lite-peek向网关发送请求，网关将数据通过DTN传输给火星网关。

客户端安装软件：
client/ccn-lite

地球上卫星网关安装软件：

gateway_on_earth/ccn-lite
gateway_on_earth/ION-DTN

客户端命令如下：
ccn-lite-peek -w 10 -s ndn2013 -u 192.168.1.7/9999 "/ndn/test/mycontent" | ccn-lite-pktdump -f 2

这里-u表示选择ip地址作为目的地址，192.168.1.7就是地球上网关的ip，9999就是网关的监听端口。



一般来说，选择客户端二适用于更为普遍的场景。客户端一将客户端和网关放在一起，实际上并不标准，比较简陋。
ccn-lite的安装博客上有。
DTN的安装步骤：
./configure
sudo make
sudo make install
sudo ldconfig






