# Notice

记录一些踩过的雷

## Lab2 tcp_receiver

*   doc里有说`empty segment`当做长度1处理，但是tests里面没有相关测试。如果不处理的话 lab4会出锅、

> If the window has size zero, then its size should be treated as one byte for the purpose of determining the first and last byte of the window.

* 在`out of window`的时候直接返回，不做处理

## Lab3 tcp_sender

* 在`ack out of range`的时候直接返回，不做处理

## Lab4 tcp_connection

* 握手之前不应该接收非握手数据包

* 收到包时做判断
  * `seqno`不在`receiver window`内，如果收到的包非空，发送空包提示 （e.g. 空`RST`不发空包提示）
  * `ackno`超出`sender seqno`，发送空包提示
  * Confused: 两者行为不同，但是按doc的描述，应该行为一致。

> • On receiving a segment, what should I do if the TCPReceiver complains that the segment didn’t overlap the window and was unacceptable (segment received() returns false)?
>
> ​	 In that situation, the TCPConnection needs to make sure that a segment is sent back to the peer, giving the current ackno and window size. This can help correct a confused peer. 
>
> • Okay, fine. How about if the TCPConnection received a segment, and the TCPSender complains that an ackno was invalid (ack received() returns false)? 
>
> ​	Same answer!

* 建立连接时的判断
  * `sender`未发送却收到`ACK`，应该发送`RST`
  * 未建立连接，包中没有`SYN`，应该等到`SYN`发来
