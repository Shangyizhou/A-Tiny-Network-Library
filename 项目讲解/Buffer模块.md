# 为什么要有缓冲区的设计
TcpConnection 类负责处理一个新连接的事件，包括从客户端读取数据和向客户端写数据。但是在这之前，需要先设计好缓冲区。

1. 非阻塞网络编程中应用层buffer是必须的：非阻塞IO的核心思想是避免阻塞在`read()`或`write()`或其他`I/O`系统调用上，这样可以最大限度复用`thread-of-control`，让一个线程能服务于多个`socket`连接。`I/O`线程只能阻塞在`IO-multiplexing`函数上，如`select()/poll()/epoll_wait()`。这样一来，应用层的缓冲是必须的，每个`TCP socket`都要有`inputBuffer`和`outputBuffer`。
2. TcpConnection必须有output buffer：使程序在`write()`操作上不会产生阻塞，当`write()`操作后，操作系统一次性没有接受完时，网络库把剩余数据则放入`outputBuffer`中，然后注册`POLLOUT`事件，一旦`socket`变得可写，则立刻调用`write()`进行写入数据。——应用层`buffer`到操作系统`buffer`
3. TcpConnection必须有input buffer：当发送方`send`数据后，接收方收到数据不一定是整个的数据，网络库在处理`socket`可读事件的时候，必须一次性把`socket`里的数据读完，否则会反复触发`POLLIN`事件，造成`busy-loop`。所以网路库为了应对数据不完整的情况，收到的数据先放到`inputBuffer`里。——操作系统`buffer`到应用层`buffer`。
# Buffer缓冲区设计
muduo 的 Buffer 类作为网络通信的缓冲区，像是 TcpConnection 就拥有 inputBuffer 和 outputBuffer 两个缓冲区成员。而缓冲区的设计特点：

1. 其内部使用`std::vector<char>`保存数据，并提供许多访问方法。并且`std::vector`拥有扩容空间的操作，可以适应数据的不断添加。
2. `std::vector<char>`内部分为三块，头部预留空间，可读空间，可写空间。内部使用索引标注每个空间的起始位置。每次往里面写入数据，就移动`writeIndex`；从里面读取数据，就移动`readIndex`。

## Buffer基本成员
![1663491010(1).png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663491023445-5cef6048-a343-43c3-a2e5-05f8d53d6a73.png#averageHue=%236b90a1&clientId=u6f718134-6646-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=290&id=LjDhv&margin=%5Bobject%20Object%5D&name=1663491010%281%29.png&originHeight=362&originWidth=595&originalType=binary&ratio=1&rotation=0&showTitle=false&size=16428&status=done&style=none&taskId=ufbbe00b4-76b7-4e3b-8832-02d9e3da6f8&title=&width=476)
```cpp
class Buffer : public muduo::copyable
{
public:
    static const size_t kCheapPrepend = 8; // 头部预留8个字节
    static const size_t kInitialSize = 1024; // 缓冲区初始化大小 1KB

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), // buffer分配大小 8 + 1KB
            readerIndex_(kCheapPrepend), // 可读索引和可写索引最开始位置都在预留字节后
            writerIndex_(kCheapPrepend) 
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

	/*......*/

	// 可读空间大小
	size_t readableBytes() const
	{ return writerIndex_ - readerIndex_; }
	
	// 可写空间大小
	size_t writableBytes() const
	{ return buffer_.size() - writerIndex_; }
	
	// 预留空间大小
	size_t prependableBytes() const
	{ return readerIndex_; }
	
	// 返回可读空间地址
	const char* peek() const
	{ return begin() + readerIndex_; }

	/*......*/
	
private:
	std::vector<char> buffer_; // 缓冲区其实就是vector<char>
	size_t readerIndex_; // 可读区域开始索引
	size_t writerIndex_; // 可写区域开始索引
};
```
## 读写数据时对Buffer的操作
![42056ba58ecff7fc0b921f751a77dbf.png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663493611175-2d79f2ee-1282-47f3-a79d-ccb294aa8413.png#averageHue=%23cae3fd&clientId=u6f718134-6646-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=255&id=TcHF0&margin=%5Bobject%20Object%5D&name=42056ba58ecff7fc0b921f751a77dbf.png&originHeight=319&originWidth=921&originalType=binary&ratio=1&rotation=0&showTitle=false&size=17200&status=done&style=none&taskId=u223fc9a9-132c-4d13-b7fa-143d5b84337&title=&width=736.8)

![b28b5599095a2af065df34c25784786.png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663493615698-fc035ca5-b55a-4930-bc47-9e88909798ca.png#averageHue=%23cae4fe&clientId=u6f718134-6646-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=264&id=vrUwu&margin=%5Bobject%20Object%5D&name=b28b5599095a2af065df34c25784786.png&originHeight=330&originWidth=995&originalType=binary&ratio=1&rotation=0&showTitle=false&size=17963&status=done&style=none&taskId=uec3922b2-d9b9-49a5-88ef-6a57e4af1cc&title=&width=796)

![4153384dbba933245c1ab9fff69d3ed.png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663493619587-deb0d8c0-8cea-47e2-b2d5-68a0755c537e.png#averageHue=%23cae3fd&clientId=u6f718134-6646-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=294&id=of7ef&margin=%5Bobject%20Object%5D&name=4153384dbba933245c1ab9fff69d3ed.png&originHeight=367&originWidth=1037&originalType=binary&ratio=1&rotation=0&showTitle=false&size=17926&status=done&style=none&taskId=uab3d23a8-0dee-469b-ba53-c2b7725f854&title=&width=829.6)

## 向Buffer写入数据：readFd
`ssize_t Buffer::readFd(int fd, int* savedErrno)`：表示从 fd 中读取数据到 buffer_ 中。对于 buffer 来说这是写入数据的操作，会改变`writeIndex`。

1. 考虑到 buffer_ 的 writableBytes 空间大小，不能够一次性读完数据，于是内部还在栈上创建了一个临时缓冲区 `char extrabuf[65536];`。如果有多余的数据，就将其读入到临时缓冲区中。
2. 因为可能要写入两个缓冲区，所以使用了更加高效`readv`函数，可以向多个地址写入数据。刚开始会判断需要写入的大小。
   1. 如果一个缓冲区足够，就不必再往临时缓冲区`extrabuf`写入数据了。写入后需要更新`writeIndex`位置，`writerIndex_ += n;`。
   2. 如果一个缓冲区不够，则还需往临时缓冲区`extrabuf`写入数据。原缓冲区直接写满，`writeIndex_ = buffer_.size()`。然后往临时缓冲区写入数据，`append(extrabuf, n - writable);`。
```cpp
/**
 * inputBuffer:：TcpConnection 从 socket 读取数据，然后写入 inputBuffer
 * 这个对于buffer_来说是将数据写入的操作，所以数据在writeIndex之后
 * 客户端从 inputBuffer 读取数据。
 */
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    /**
     * 从fd读取数据到两个地方
     * 1.writeIndex
     * 2.stack上的临时数组变量（防止不能一次性读完fd上的数据）
     */
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    // 判断需要写入几个缓冲区
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (implicit_cast<size_t>(n) <= writable)
    {
        // 如果从fd读取数据长度小于buffer可写数据空间，则直接更改writerIndex索引即可
        writerIndex_ += n;
    }
    else
    {
        // buffer可写数据空间不够，还需写入extrabuf
        // writerIndex直接到尾部
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_30;
    // }
    return n;
}
```
其中的 append 函数真正向 buffer_ 内部添加数据。调用方将数据的首地址和长度给出，其内部将数据拷贝到指定位置。
```cpp
// 向buffer_添加数据
void append(const char* /*restrict*/ data, size_t len)
{
	// 确保可写空间足够
	ensureWritableBytes(len);
	// 将这段数据拷贝到可写位置之后
	std::copy(data, data+len, beginWrite());
	hasWritten(len);
}
```
## 空间不够怎么办？
如果写入空间不够，Buffer 内部会有两个方案来应付

1. 将数据往前移动：因为每次读取数据，`readIndex`索引都会往后移动，从而导致前面预留的空间逐渐增大。我们需要将后面的元素重新移动到前面。
2. 如果第一种方案的空间仍然不够，那么我们就直接对 buffer_ 进行扩容（`buffer_.resize(len)`）操作。

**如图所示：现在的写入空间不够，但是前面的预留空间加上现在的写空间是足够的。因此，我们需要将后面的数据拷贝到前面，腾出足够的写入空间。**

![652d05b2afe60d2c3939e1ea6fb64b6.png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663496328441-e7d2d445-82e5-4f08-9fc9-62621c0f0908.png#averageHue=%23c9e3fd&clientId=u6f718134-6646-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=285&id=u9f3cb530&margin=%5Bobject%20Object%5D&name=652d05b2afe60d2c3939e1ea6fb64b6.png&originHeight=356&originWidth=1028&originalType=binary&ratio=1&rotation=0&showTitle=false&size=20451&status=done&style=none&taskId=u494bcbb0-913c-4cce-90e4-bc639a73b85&title=&width=822.4)
![797797d7067e41e1f3e1ff62c20b3a5.png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663496332935-5f064f9c-8c55-4ca1-8038-9d1f638de652.png#averageHue=%23c9e2fc&clientId=u6f718134-6646-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=269&id=u0113ae2d&margin=%5Bobject%20Object%5D&name=797797d7067e41e1f3e1ff62c20b3a5.png&originHeight=336&originWidth=1004&originalType=binary&ratio=1&rotation=0&showTitle=false&size=23098&status=done&style=none&taskId=u52056cdc-e550-4f14-af16-34f4e9473e7&title=&width=803.2)

muduo 的代码实现：

```cpp
// 保证写空间足够len，如果不够则扩容
void ensureWritableBytes(size_t len)
{
	if (writableBytes() < len)
	{
		makeSpace(len);
	}
	assert(writableBytes() >= len);
}

// 扩容空间
void makeSpace(size_t len)
{
	// prependIndex -------------readIndex---writeIndex-
	// 
	// 因为readIndex一直往后，之前的空间没有被利用，我们可以将后面数据复制到前面
	// 如果挪位置都不够用，则只能重新分配buffer_大小
	if (writableBytes() + prependableBytes() < len + kCheapPrepend)
	{
		// FIXME: move readable data
		buffer_.resize(writerIndex_+len);
	}
	else
	{
		// move readable data to the front, make space inside buffer
		assert(kCheapPrepend < readerIndex_);
		size_t readable = readableBytes();
		std::copy(begin()+readerIndex_,
				begin()+writerIndex_,
				begin()+kCheapPrepend);
		// 读取空间地址回归最开始状态
		readerIndex_ = kCheapPrepend;
		// 可以看到这一步，写空间位置前移了
		writerIndex_ = readerIndex_ + readable;
		assert(readable == readableBytes());
	}
}
```
## 从Buffer中读取数据
就如回声服务器的例子一样：
```cpp
void EchoServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)
{
	// 从 buf 中读取所有数据，返回 string 类型
    muduo::string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
            << "data received at " << time.toString();
    conn->send(msg);
}
```
读取数据会调用`void retrieve(size_t len)`函数，在这之前会判断读取长度是否大于可读取空间

1. 如果小于，则直接后移`readIndex`即可，`readerIndex_ += len;`。
2. 如果大于等于，说明全部数据都读取出来。此时会将buffer置为初始状态：
   1. `readerIndex_ = kCheapPrepend;`
   2. `writerIndex_ = kCheapPrepend;`
```cpp
// 将可读取的数据按照string类型全部取出
string retrieveAllAsString()
{
	return retrieveAsString(readableBytes());
}

// string(peek(), len)
string retrieveAsString(size_t len)
{
	assert(len <= readableBytes());
	string result(peek(), len);
	retrieve(len); // 重新置位
	return result;
}

// retrieve returns void, to prevent
// string str(retrieve(readableBytes()), readableBytes());
// the evaluation of two functions are unspecified
// 读取len长度数据
void retrieve(size_t len)
{
	assert(len <= readableBytes());
	if (len < readableBytes())
	{
		// 读取长度小于可读取空间，直接更新索引
		readerIndex_ += len;
	}
	// 读取长度大于等于可读取空间
	else
	{
		retrieveAll();
	}
}

// 读取所有数据
void retrieveAll()
{
	// 全部置为初始状态
	readerIndex_ = kCheapPrepend;
	writerIndex_ = kCheapPrepend;
}

```
# TcpConnection使用Buffer
TcpConnection 拥有 inputBuffer 和 outputBuffer 两个缓冲区成员。

1. 当服务端接收客户端数据，EventLoop 返回活跃的 Channel，并调用对应的读事件处理函数，即 TcpConnection 调用 handleRead 方法从相应的 fd 中读取数据到 inputBuffer 中。在 Buffer 内部 inputBuffer 中的 writeIndex 向后移动。
2. 当服务端向客户端发送数据，TcpConnection 调用 handleWrite 方法将 outputBuffer 的数据写入到 TCP 发送缓冲区。outputBuffer 内部调用 `retrieve` 方法移动 readIndex 索引。

![1663491937(1).png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663491940665-8107d2ec-afc4-4a24-bacc-c8e63fdb32ce.png#averageHue=%2394bdce&clientId=u6f718134-6646-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=590&id=uc28a2661&margin=%5Bobject%20Object%5D&name=1663491937%281%29.png&originHeight=737&originWidth=1194&originalType=binary&ratio=1&rotation=0&showTitle=false&size=64587&status=done&style=none&taskId=ud738a601-2f43-41a8-a5f7-1a524ec1f43&title=&width=955.2)

## TcpConnection接收客户端数据（从客户端sock读取数据到inputBuffer）

1. 调用`inputBuffer_.readFd(channel_->fd(), &savedErrno);`将对端`fd`数据读取到`inputBuffer`中。
   1. 如果读取成功，调用「可读事件发生回调函数」
   2. 如果读取数据长度为`0`，说明对端关闭连接。调用`handleCose()`
   3. 出错，则保存`errno`，调用`handleError()`
```cpp
/**
 * 消息读取，TcpConnection从客户端读取数据
 * 调用Buffer.readFd(fd, errno) -> 内部调用readv将数据从fd读取到缓冲区 -> inputBuffer
 */
void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    // 将 channel_->fd() 数据读取到 inputBuffer_ 中，出错信息保存到 savedErrno 中
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读事件发生，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    // 读取不到数据，关闭此连接
    else if (n == 0)
    {
        handleClose();
    }
    // 出错
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}
```
## TcpConnection向客户端发送数据（将ouputBuffer数据输出到socket中）
```cpp
// 此行代码的用意何在
if (channel_->isWriting())
```

1. 要在`channel_`确实关注写事件的前提下正常发送数据：因为一般有一个`send`函数发送数据，如果TCP接收缓冲区不够接收ouputBuffer的数据，就需要多次写入。需要重新注册写事件，因此是在注册了写事件的情况下调用的`handleWrite`。
2. 向`channel->fd()`发送outputBuffer中的可读取数据。成功发送数据则移动`readIndex`，并且如果一次性成功写完数据，就不再让此`channel`关注写事件了，并调用写事件完成回调函数没写完则继续关注！
```cpp
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    // channel关注了写事件
    if (channel_->isWriting())
    {
        // 向客户端fd写数据，[peek, peek + readable)
        ssize_t n = sockets::write(channel_->fd(),
                                    outputBuffer_.peek(),
                                    outputBuffer_.readableBytes());
        // 成功写入数据
        if (n > 0)
        {
            // 重置readIndex位置，向后移动n，表示这n个字节的数据都被读取出来了
            outputBuffer_.retrieve(n);
            // 缓冲区可读空间为0，说明 writeIndex - readIndex = 0
            // 我们一次性将数据写完了
            if (outputBuffer_.readableBytes() == 0)
            {
                // channel不再关注写事件
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // 调用用户自定义的写完成事件函数
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    // TcpCOnnection关闭写端
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
        }
    }
    else
    {
        LOG_TRACE << "Connection fd = " << channel_->fd()
                    << " is down, no more writing";
    }
}
```
# 参考
[Muduo库中的Buffer设计_烊萌的博客-CSDN博客](https://blog.csdn.net/qq_36417014/article/details/106190964#:~:text=Muduo%E5%BA%93%E4%B8%AD%E7%9A%84Buffer%E7%B1%BB%E8%AE%BE%E8%AE%A1%20%E9%9D%9E%E9%98%BB%E5%A1%9E%E7%BD%91%E7%BB%9C%E7%BC%96%E7%A8%8B%E4%B8%AD%E5%BA%94%E7%94%A8%E5%B1%82buffer%E6%98%AF%E5%BF%85%E9%A1%BB%E7%9A%84%20%E5%8E%9F%E5%9B%A0%EF%BC%9A%E9%9D%9E%E9%98%BB%E5%A1%9EIO%E7%9A%84%E6%A0%B8%E5%BF%83%E6%80%9D%E6%83%B3%E6%98%AF%E9%81%BF%E5%85%8D%E9%98%BB%E5%A1%9E%E5%9C%A8read%20%28%29%E6%88%96write%20%28%29%E6%88%96%E5%85%B6%E4%BB%96IO%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8%E4%B8%8A%EF%BC%8C%E8%BF%99%E6%A0%B7%E5%8F%AF%E4%BB%A5%E6%9C%80%E5%A4%A7%E9%99%90%E5%BA%A6%E5%A4%8D%E7%94%A8thread-of-control%EF%BC%8C%E8%AE%A9%E4%B8%80%E4%B8%AA%E7%BA%BF%E7%A8%8B%E8%83%BD%E6%9C%8D%E5%8A%A1%E4%BA%8E%E5%A4%9A%E4%B8%AAsocket%E8%BF%9E%E6%8E%A5%E3%80%82%20IO%E7%BA%BF%E7%A8%8B%E5%8F%AA%E8%83%BD%E9%98%BB%E5%A1%9E%E5%9C%A8IO-multiplexing%E5%87%BD%E6%95%B0%E4%B8%8A%EF%BC%8C%E5%A6%82select%20%28%29%2Fpoll,%28%29%2Fepoll_wait%20%28%29%E3%80%82%20%E8%BF%99%E6%A0%B7%E4%B8%80%E6%9D%A5%EF%BC%8C%E5%BA%94%E7%94%A8%E5%B1%82%E7%9A%84%E7%BC%93%E5%86%B2%E6%98%AF%E5%BF%85%E9%A1%BB%E7%9A%84%EF%BC%8C%E6%AF%8F%E4%B8%AATCP%20socket%E9%83%BD%E8%A6%81%E6%9C%89input%20buffer%E5%92%8Coutput%20buffer%E3%80%82%20TcpConnection%E5%BF%85%E9%A1%BB%E6%9C%89output%20buffer)
